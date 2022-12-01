#pragma once

#ifndef fGMS_h
#define fGMS_h

#include "fNETLib.h"
#include "ArduinoJson.h"

class fGMS {
public:
    class Hygrometer {
    public:
        float x = -1;
        float y = -1;

        int id = 0;

        bool ok = false;

        float value = -1;
    };

    class HygrometerGroup {
    public:
        int hygrometers[128];
        int numHygrometers = 0;

        int color = 0x404040;

        void AddHygrometer(int h) {
            hygrometers[numHygrometers++] = h;
        }
    };

    class HygrometerModule {
    public:
        HygrometerModule(fNETController::fNETSlaveConnection* m) {
            mdl = m;
            module_mac = m->MAC_Address;
        }

        HygrometerModule(JsonObject o) {
            module_mac = o["mac"].as<String>();
            mdl = fNETController::GetModuleByMAC(module_mac);

            map_min = o["mapMin"];
            map_max = o["mapMax"];

            for (int i : o["hygrometers"].as<JsonArray>())
                hygrometers[hygrometerCount++] = i;
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument d = DynamicJsonDocument(256);
            d["mac"] = module_mac;
            d["mapMin"] = map_min;
            d["mapMax"] = map_max;

            JsonArray a = d.createNestedArray("hygrometers");
            for (int i = 0; i < hygrometerCount; i++)
                a.add(hygrometers[i]);

            return d;
        }


        int map_min = 0;
        int map_max = 16384;

        String module_mac = "";
        fNETController::fNETSlaveConnection* mdl = nullptr;

        int Channels = 4;
        int hygrometerCount;
        int hygrometers[8];

        float Values[8];

        bool ok;

        void Update() {
            if (mdl == nullptr || !mdl->isOnline) {
                ok = false;
                lastDataID = 0;
                return;
            }

            if (millis() - lastMessageReceivedMillis > 2000)
                setInterval();
            else
                ok = true;

            checkMessages();
        }

    private:
        void setInterval() {
            Serial.println("[fGMS Hygrometer Controller] Setting value interval of module " + module_mac);

            DynamicJsonDocument d(256);
            d["interval"] = 250;
            JsonObject* result = fNET->Query(module_mac, "setValueInterval", &d);
            if (result == nullptr) {
                Serial.println("[fGMS Hygrometer Controller] Setting value interval of module " + module_mac + " failed!");
                ok = false;
            }
            delay(100);
            lastDataID = 0;
        }

        void checkMessages() {
            for (int i = 0; i < fNET->ReceivedJSONBuffer.size(); i++) {
                if (i > fNET->ReceivedJSONBuffer.size() - 1)
                    return;

                DynamicJsonDocument r = *fNET->ReceivedJSONBuffer[i];

                if (r["tag"].as<String>() == "values" && r["source"].as<String>() == module_mac && r["dataID"].as<int>() > lastDataID) {
                    int i = 0;

                    for (float f : r["values"].as<JsonArray>())
                        Values[i++] = f;

                    lastDataID = r["dataID"];
                    lastMessageReceivedMillis = millis();

                    break;
                }
            }
        }

        long lastMessageReceivedMillis = 0;
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

    static fNETConnection* fNET;

    static Hygrometer* Hygrometers[1024];
    static int HygrometerCount;

    static HygrometerGroup* HygrometerGroups[16];
    static int HygrometerGroupCount;

    static HygrometerModule* HygrometerModules[128];
    static int HygrometerModuleCount;

    static ValveModule* ValveModules[128];
    static int ValveModuleCount;

    static Hygrometer* GetHygrometer(int id) {
        if (id < HygrometerCount)
            return Hygrometers[id];

        return nullptr;
    }

    static Hygrometer* CreateHygrometer() {
        Hygrometer* h = new Hygrometer();
        Hygrometers[HygrometerCount++] = h;

        return h;
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
        HygrometerModule* m = new HygrometerModule(mdl);
        HygrometerModules[HygrometerModuleCount++] = m;

        return m;
    }

    static ValveModule* AddValveModule(fNETController::fNETSlaveConnection* mdl) {
        Serial.println("[fGMS] Add valve module: " + mdl->MAC_Address);
        ValveModule* m = new ValveModule(mdl);
        ValveModules[ValveModuleCount++] = m;

        return m;
    }

    static DynamicJsonDocument data;

    static void Init(fNETConnection* c) {
        Serial.println("[fGMS] Init...");

        fNET = c;

        ReadConfig();

        xTaskCreate(hygrometerModuleUpdateTask, "fGMS_hygrometers", 4096, nullptr, 0, nullptr);
    }

    static void Save() {
        Serial.println("[fGMS] Saving...");
        JsonArray hygrometersArray = data.createNestedArray("hygrometers");

        for (int i = 0; i < HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();
            Hygrometer* h = Hygrometers[i];

            o["x"] = h->x;
            o["y"] = h->y;
            o["ok"] = h->ok;
        }

        JsonArray groupsArray = data.createNestedArray("groups");

        for (int i = 0; i < HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();
            HygrometerGroup* g = HygrometerGroups[i];

            JsonArray hygros = o.createNestedArray("hygrometers");

            for (int j = 0; j < g->numHygrometers; j++)
                hygros.add(g->hygrometers[j]);

            o["color"] = g->color;
        }

        JsonArray hygrometerModulesArray = data.createNestedArray("hygrometerModules");

        for (int i = 0; i < HygrometerModuleCount; i++) {
            JsonObject o = hygrometerModulesArray.createNestedObject();
            o.set(HygrometerModules[i]->Save().as<JsonObject>());
        }

        String data_serialized;
        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fGMS_Data.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fGMS] Saved.");
    }

    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            return;

        File data_file = LittleFS.open("/fGMS_Data.json", "r");

        if (!data_file)
            return;

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fGMS] Read config: " + data_rawJson);


        JsonArray hygrometersArray = data["hygrometers"];
        HygrometerCount = 0;

        for (JsonObject o : hygrometersArray) {
            Hygrometer* h = CreateHygrometer();
            h->x = o["x"].as<float>();
            h->y = o["y"].as<float>();
            h->ok = o["ok"].as<bool>();
        }

        JsonArray groupsArray = data["groups"];
        HygrometerGroupCount = 0;

        for (JsonObject o : groupsArray) {
            HygrometerGroup* h = CreateHygrometerGroup();
            JsonArray idsArray = data["hygrometers"];

            for (int i : idsArray)
                h->hygrometers[h->numHygrometers++] = i;

            h->color = o["color"].as<int>();
        }

        JsonArray hygrometerModulesArray = data["hygrometerModules"];
        HygrometerModuleCount = 0;

        for (JsonObject o : hygrometerModulesArray)
            HygrometerModules[HygrometerModuleCount++] = new HygrometerModule(o);

        JsonArray valveModulesArray = data["modules"];
        ValveModuleCount = 0;

        for (JsonObject o : valveModulesArray)
            ValveModules[ValveModuleCount++] = new ValveModule(o);

        Serial.println("[fGMS] Loaded");
    }

private:
    static void hygrometerModuleUpdateTask(void* param) {
        while (true) {
            delay(75);

            for (int i = 0; i < HygrometerModuleCount; i++)
                HygrometerModules[i]->Update();

            updateHygrometerModules();
            updateValveModules();
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
        }
    }
};

#endif // !fGMS_h
