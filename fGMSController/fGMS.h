#pragma once

#ifndef fGMS_h
#define fGMS_h

#include "fNETLib.h"
#include "ArduinoJson.h"

class fGMS {
public:
    class Greenhouse {
    public:
        float x_size;
        float y_size;
    };

    class HygrometerModule;

    class Hygrometer {
    public:
        Hygrometer() {
            id = HygrometerCount;
        }

        Hygrometer(JsonObject o) {
            id = HygrometerCount;

            x = o["x"];
            y = o["y"];

            Module = GetHygrometerModuleByMAC(o["module"]);

            if (Module != nullptr)
                Channel = o["channel"];
            else
                Channel = -1;

            map_min = o["mapMin"];
            map_max = o["mapMax"];

            Setpoint = o["setpoint"];
        }

        float x = -1;
        float y = -1;

        int id = 0;

        bool ok = false;

        HygrometerModule* Module = nullptr;
        int Channel = -1;

        float map_min = 0.00;
        float map_max = 3.30;

        float Setpoint;

        float value = -1;

        bool invalid = false;

        DynamicJsonDocument Save() {
            DynamicJsonDocument o(128);

            o["x"] = x;
            o["y"] = y;

            if (Module != nullptr)
                o["module"] = Module->module_mac;

            o["channel"] = Channel;

            o["mapMin"] = map_min;
            o["mapMax"] = map_max;

            o["setpoint"] = Setpoint;

            return o;
        }

        float GetRaw() {
            if (Module == nullptr || Channel <= -1)
                return -1;

            if (!Module->ok)
                return -1;

            if (Channel >= Module->Channels)
                return -1;

            return Module->Values[Channel];
        }

        float GetValue() {
            float volts = GetRaw();

            if (volts == -1)
                return -1;

            return min(1.0f, max((volts - map_min) / (map_max - map_min), 0.0f));
        }
    };

    class HygrometerGroup {
    public:
        Hygrometer* hygrometers[128];
        int numHygrometers = 0;

        int color = 0x404040;

        void AddHygrometer(Hygrometer* h) {
            hygrometers[numHygrometers++] = h;
        }
    };

    class HygrometerModule {
    public:
        HygrometerModule(fNETController::fNETSlaveConnection* m) {
            mdl = m;
            module_mac = m->MAC_Address;

            tunnel = new fNETTunnel(fNETController::Connection, "data");
            tunnel->Init();

            tunnel->OnMessageReceived.AddHandler(new EventHandler<HygrometerModule>(this, [](HygrometerModule* m, void* d) { m->messageReceived(*(DynamicJsonDocument*)d); }));
        }

        HygrometerModule(JsonObject o) {
            module_mac = o["mac"].as<String>();
            mdl = fNETController::GetModuleByMAC(module_mac);

            tunnel = new fNETTunnel(fNETController::Connection, "data");
            tunnel->Init();

            tunnel->OnMessageReceived.AddHandler(new EventHandler<HygrometerModule>(this, [](HygrometerModule* m, void* d) { m->messageReceived(*(DynamicJsonDocument*)d); }));
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument d = DynamicJsonDocument(256);
            d["mac"] = module_mac;

            return d;
        }

        String module_mac = "";
        fNETController::fNETSlaveConnection* mdl = nullptr;
        fNETTunnel* tunnel;

        int Channels = -1;

        float Values[16];

        bool ok;

        bool value_received = false;

        void Update() {
            if (mdl == nullptr || !mdl->isOnline) {
                ok = false;
                lastDataID = 0;
                return;
            }

            if (!tunnel->IsConnected && millis() - lastConnectionAttempt > 10000) {
                connect();
                ok = false;
                return;
            }

            if (millis() - lastMessageReceivedMillis > 2000) {
                setInterval();
                value_received = false;
                ok = false;
            }

            if (value_received)
                ok = true;
        }

    private:
        void setInterval() {
            Serial.println("[fGMS Hygrometer Controller] Setting value interval of module " + module_mac);

            DynamicJsonDocument d(256);
            d["interval"] = 250;
            JsonObject* result = fNET->Query(module_mac, "setValueInterval", &d);

            lastDataID = 0;

            if (result == nullptr) {
                Serial.println("[fGMS Hygrometer Controller] Setting value interval of module " + module_mac + " failed!");
                ok = false;
                return;
            }

            lastMessageReceivedMillis = millis();
        }

        void connect() {
            Serial.println("[fGMS Hygrometer Controller] Trying to connect to " + module_mac + ".");

            bool res = tunnel->TryConnect(module_mac);

            if (!res)
                Serial.println("[fGMS Hygrometer Controller] Connection to " + module_mac + " failed!");

            lastConnectionAttempt = millis();
            return;
        }

        void messageReceived(DynamicJsonDocument r) {
            int i = 0;

            for (float f : r["hygrometers"].as<JsonArray>())
                Values[i++] = f;

            Channels = i;

            //Serial.println(Channels);

            lastMessageReceivedMillis = millis();
            value_received = true;
        }

        void checkMessages() {
            /*
            for (int i = 0; i < fNET->ReceivedJSONBuffer.size(); i++) {
                if (i > fNET->ReceivedJSONBuffer.size() - 1)
                    return;

                DynamicJsonDocument r = *fNET->ReceivedJSONBuffer[i];

                //String json_str;
                //serializeJsonPretty(r, json_str);

                //Serial.println("Chk message");
                //Serial.println(r["tag"].as<String>() == "values");
                //Serial.println(r["source"].as<String>() == module_mac);
                //Serial.println(r["dataID"].as<int>() > lastDataID);

                //Serial.println(json_str);


                if (r["tag"].as<String>() == "values" && r["source"].as<String>() == module_mac && r["dataID"].as<int>() > lastDataID) {
                    int i = 0;

                    for (float f : r["hygrometers"].as<JsonArray>())
                        Values[i++] = f;

                    Channels = i;

                    //Serial.println(Channels);

                    lastDataID = r["dataID"];
                    lastMessageReceivedMillis = millis();

                    break;
                }
            }
            */
        }

        long lastMessageReceivedMillis = 0;
        long lastConnectionAttempt = -10000;
        long lastDataID = 0;
    };

    class ValveModule {
    public:
        ValveModule(fNETController::fNETSlaveConnection* m) {
            mdl = m;
            module_mac = m->MAC_Address;
        }

        ValveModule(JsonObject o) {
            module_mac = o["mac"].as<String>();
            mdl = fNETController::GetModuleByMAC(module_mac);
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument d = DynamicJsonDocument(256);
            d["mac"] = module_mac;

            return d;
        }


        String module_mac = "";
        fNETController::fNETSlaveConnection* mdl = nullptr;

        int State = 0b0000;

        bool ok;

        void Update() {
            ok = !(mdl == nullptr || !mdl->isOnline);
        }

        void SetState(int state) {
            Serial.println("[fGMS Hygrometer Controller] Setting valve state of module " + module_mac + ", state: " + String(state));

            DynamicJsonDocument d(256);
            d["state"] = state;
            JsonObject* result = fNET->Query(module_mac, "setState", &d);

            if (result == nullptr) {
                Serial.println("[fGMS Hygrometer Controller] Setting valve state of module " + module_mac + " failed!");
                ok = false;
            }
            else
                State = (*result)["state"].as<int>();

            delay(100);
        }
    };

    class SensorModule {
    public:
        SensorModule(fNETController::fNETSlaveConnection* m) {
            mdl = m;
            module_mac = m->MAC_Address;
        }

        SensorModule(JsonObject o) {
            module_mac = o["mac"].as<String>();
            mdl = fNETController::GetModuleByMAC(module_mac);
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument d = DynamicJsonDocument(256);
            d["mac"] = module_mac;

            return d;
        }


        String module_mac = "";
        fNETController::fNETSlaveConnection* mdl = nullptr;

        int State = 0b0000;

        bool ok;

        void Update() {
            ok = !(mdl == nullptr || !mdl->isOnline);
        }

        void SetFan(bool on) {
            Serial.println("[fGMS Hygrometer Controller] Setting fan state of module " + module_mac + ", state: " + (on ? "on" : "off"));

            DynamicJsonDocument d(256);
            d["enabled"] = on;
            JsonObject* result = fNET->Query(module_mac, "setFan", &d);

            if (result == nullptr) {
                Serial.println("[fGMS Hygrometer Controller] Setting fan state of module " + module_mac + " failed!");
                ok = false;
            }
            else
                State = (*result)["enabled"].as<int>();

            delay(100);
        }
    };

    static fNETConnection* fNET;

    static Hygrometer* Hygrometers[1024];
    static int HygrometerCount;

    static HygrometerGroup* HygrometerGroups[64];
    static int HygrometerGroupCount;

    static HygrometerModule* HygrometerModules[128];
    static int HygrometerModuleCount;

    static ValveModule* ValveModules[16];
    static int ValveModuleCount;

    static SensorModule* SensorModules[16];
    static int SensorModuleCount;

    static Greenhouse greenhouse;

    static Hygrometer* GetHygrometer(int id) {
        if (id < HygrometerCount)
            return Hygrometers[id];

        return nullptr;
    }

    static int GetHygrometerID(Hygrometer* h) {
        for (int i = 0; i < HygrometerCount; i++) {
            if (Hygrometers[i] == h)
                return i;
        }

        return -1;
    }

    static Hygrometer* CreateHygrometer() {
        Hygrometer* h = new Hygrometer();
        Hygrometers[HygrometerCount++] = h;

        h->id = GetHygrometerID(h);

        return h;
    }

    static void RemoveHygrometer(int i) {
        delete Hygrometers[i];
        for (int index = i; index < HygrometerCount - 1; index++) {
            Hygrometers[index]->id--;
            Hygrometers[index] = Hygrometers[index + 1];
        }

        HygrometerCount--;
    }

    static HygrometerGroup* CreateHygrometerGroup() {
        HygrometerGroup* g = new HygrometerGroup();
        HygrometerGroups[HygrometerGroupCount++] = g;

        return g;
    }

    static HygrometerGroup* GetHygrometerGroup(int id) {
        if (id < HygrometerGroupCount)
            return HygrometerGroups[id];

        return nullptr;
    }

    static HygrometerModule* AddHygrometerModule(fNETController::fNETSlaveConnection* mdl) {
        Serial.println("[fGMS] Add hygrometer module: " + mdl->MAC_Address);

        for (int i = 0; i < HygrometerModuleCount; i++)
            if (HygrometerModules[i]->module_mac == mdl->MAC_Address)
                return;

        HygrometerModule* m = new HygrometerModule(mdl);
        HygrometerModules[HygrometerModuleCount++] = m;

        return m;
    }

    static HygrometerModule* GetHygrometerModuleByMAC(String m) {
        for (int i = 0; i < HygrometerModuleCount; i++) {
            if (HygrometerModules[i]->module_mac == m)
                return HygrometerModules[i];
        }

        return nullptr;
    }

    static ValveModule* AddValveModule(fNETController::fNETSlaveConnection* mdl) {
        Serial.println("[fGMS] Add valve module: " + mdl->MAC_Address);

        for (int i = 0; i < ValveModuleCount; i++)
            if (ValveModules[i]->module_mac == mdl->MAC_Address)
                return;

        ValveModule* m = new ValveModule(mdl);
        ValveModules[ValveModuleCount++] = m;

        return m;
    }

    static SensorModule* AddSensorModule(fNETController::fNETSlaveConnection* mdl) {
        Serial.println("[fGMS] Add sensor module: " + mdl->MAC_Address);

        for (int i = 0; i < SensorModuleCount; i++)
            if (SensorModules[i]->module_mac == mdl->MAC_Address)
                return;

        SensorModule* m = new SensorModule(mdl);
        SensorModules[SensorModuleCount++] = m;

        return m;
    }

    static void Init(fNETConnection* c) {
        Serial.println("[fGMS] Init...");

        fNET = c;

        ReadConfig();

        xTaskCreate(moduleUpdateTask, "fGMS_hygrometers", 4096, nullptr, 0, nullptr);
    }

    static void Save() {
        Serial.println("[fGMS] Saving...");
        DynamicJsonDocument* data = new DynamicJsonDocument(65535);

        JsonArray hygrometersArray = data->createNestedArray("hygrometers");

        for (int i = 0; i < HygrometerCount; i++)
            hygrometersArray.add(Hygrometers[i]->Save().as<JsonObject>());

        JsonArray groupsArray = data->createNestedArray("groups");

        for (int i = 0; i < HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();
            HygrometerGroup* g = HygrometerGroups[i];

            JsonArray hygros = o.createNestedArray("hygrometers");

            for (int j = 0; j < g->numHygrometers; j++)
                hygros.add(g->hygrometers[j]->id);

            o["color"] = g->color;
        }

        JsonArray hygrometerModulesArray = data->createNestedArray("hygrometerModules");

        for (int i = 0; i < HygrometerModuleCount; i++)
            hygrometerModulesArray.add(HygrometerModules[i]->Save().as<JsonObject>());

        JsonArray valveModulesArray = data->createNestedArray("valveModules");

        for (int i = 0; i < ValveModuleCount; i++)
            valveModulesArray.add(ValveModules[i]->Save().as<JsonObject>());

        JsonArray sensorModulesArray = data->createNestedArray("sensorModules");

        for (int i = 0; i < SensorModuleCount; i++)
            sensorModulesArray.add(SensorModules[i]->Save().as<JsonObject>());

        (*data)["greenhouseSizeX"] = greenhouse.x_size;
        (*data)["greenhouseSizeY"] = greenhouse.y_size;

        String data_serialized;
        serializeJson(*data, data_serialized);

        Serial.println("[fGMS] New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fGMS_Data.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        delete data;
        Serial.println("[fGMS] Saved.");
    }

    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            return;

        File data_file = LittleFS.open("/fGMS_Data.json", "r");

        if (!data_file)
            return;

        DynamicJsonDocument data(32767);

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fGMS] Read config: " + data_rawJson);

        HygrometerCount = 0;
        HygrometerGroupCount = 0;

        HygrometerModuleCount = 0;
        ValveModuleCount = 0;
        SensorModuleCount = 0;

        for (JsonObject o : data["groups"].as<JsonArray>()) {
            HygrometerGroup* h = CreateHygrometerGroup();
            JsonArray idsArray = data["hygrometers"];

            for (int i : idsArray)
                h->hygrometers[h->numHygrometers++] = GetHygrometer(i);

            h->color = o["color"].as<int>();
        }

        for (JsonObject o : data["hygrometerModules"].as<JsonArray>())
            HygrometerModules[HygrometerModuleCount++] = new HygrometerModule(o);

        for (JsonObject o : data["valveModules"].as<JsonArray>())
            ValveModules[ValveModuleCount++] = new ValveModule(o);

        for (JsonObject o : data["sensorModules"].as<JsonArray>())
            SensorModules[SensorModuleCount++] = new SensorModule(o);

        for (JsonObject o : data["hygrometers"].as<JsonArray>())
            Hygrometers[HygrometerCount++] = new Hygrometer(o);

        greenhouse.x_size = data["greenhouseSizeX"];
        greenhouse.y_size = data["greenhouseSizeY"];

        Serial.println("[fGMS] Loaded");
    }

private:
    static void moduleUpdateTask(void* param) {
        long last_refreshed = 0;
        while (true) {
            delay(75);

            for (int i = 0; i < HygrometerModuleCount; i++)
                HygrometerModules[i]->Update();

            for (int i = 0; i < ValveModuleCount; i++)
                ValveModules[i]->Update();

            for (int i = 0; i < SensorModuleCount; i++)
                SensorModules[i]->Update();

            if (millis() - last_refreshed > 5000) {
                updateHygrometerModules();
                updateValveModules();
                updateSensorModules();

                last_refreshed = millis();
            }
        }
    }

    static void updateHygrometerModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++) {
            fNETController::fNETSlaveConnection* c = fNETController::Modules[i];
            if (c->Config["ModuleType"] != "HygroCtl")
                continue;

            if (!c->isOnline)
                continue;

            bool skip = false;
            for (int i = 0; i < HygrometerModuleCount; i++)
                if (HygrometerModules[i]->mdl->MAC_Address == c->MAC_Address) {
                    skip = true;
                    break;
                }


            if (skip)
                continue;

            AddHygrometerModule(c);
            Save();
        }
    }

    static void updateValveModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++) {
            fNETController::fNETSlaveConnection* c = fNETController::Modules[i];
            if (c->Config["ModuleType"] != "ValveCtl")
                continue;

            if (!c->isOnline)
                continue;

            bool skip = false;
            for (int i = 0; i < ValveModuleCount; i++)
                if (ValveModules[i]->mdl->MAC_Address == c->MAC_Address) {
                    skip = true;
                    break;
                }


            if (skip)
                continue;

            AddValveModule(c);
            Save();
        }
    }

    static void updateSensorModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++) {
            fNETController::fNETSlaveConnection* c = fNETController::Modules[i];
            if (c->Config["ModuleType"] != "AirSensor")
                continue;

            if (!c->isOnline)
                continue;

            bool skip = false;
            for (int i = 0; i < SensorModuleCount; i++)
                if (SensorModules[i]->mdl->MAC_Address == c->MAC_Address) {
                    skip = true;
                    break;
                }


            if (skip)
                continue;

            AddSensorModule(c);
            Save();
        }
    }
};

#endif // !fGMS_h
