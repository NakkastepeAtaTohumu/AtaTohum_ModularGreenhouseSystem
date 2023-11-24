#pragma once

#ifndef fGMS_h
#define fGMS_h

#include "fNETLib.h"
#include "ArduinoJson.h"
#include "FS.h"

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RemoteUDPLogging.h>

using namespace fs;

class fGMS {
public:
    class Greenhouse {
    public:
        float x_size;
        float y_size;
    };

    class HygrometerModule;
    class ValveModule;

    class Hygrometer {
    public:
        Hygrometer() {
            id = HygrometerCount;
        }

        Hygrometer(JsonObject o) {
            id = HygrometerCount;

            Load(o);
        }

        void Load(JsonObject o) {
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
        HygrometerGroup(JsonObject o) {
            Load(o);

            id = HygrometerGroupCount;
        }

        HygrometerGroup() {
            id = HygrometerGroupCount;
        }

        void Load(JsonObject o) {
            JsonArray idsArray = o["hygrometers"];

            numHygrometers = 0;
            for (int i : idsArray)
                hygrometers[numHygrometers++] = GetHygrometer(i);

            color = o["color"].as<int>();
            mod = GetValveModuleByMAC(o["module"]);
            if (mod == nullptr)
                Serial.println("Can't find valve module!");

            channel = o["channel"];

            map_min = o["mapMin"];
            map_max = o["mapMax"];
        }

        Hygrometer* hygrometers[128];
        int numHygrometers = 0;

        int color = 0x404040;

        ValveModule* mod = nullptr;
        int channel = -1;

        int id = 0;

        float map_min;
        float map_max;

        void AddHygrometer(Hygrometer* h) {
            hygrometers[numHygrometers++] = h;
        }

        void RemoveHygrometer(Hygrometer* h) {
            int i = -1;
            for (int index = 0; index < numHygrometers; index++) {
                if (hygrometers[index] == h)
                {
                    i = index;
                    break;
                }
            }

            if (i == -1)
                return;


            for (int index = i; index < numHygrometers - 1; index++)
                hygrometers[index] = hygrometers[index + 1];

            numHygrometers--;
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument o(512);

            JsonArray hygros = o.createNestedArray("hygrometers");

            for (int j = 0; j < numHygrometers; j++)
                hygros.add(hygrometers[j]->id);

            o["color"] = color;

            if (mod != nullptr)
                o["module"] = mod->module_mac;

            o["channel"] = channel;

            o["mapMin"] = map_min;
            o["mapMax"] = map_max;

            return o;
        }

        float GetAverage() {
            float value = 0;

            for (int i = 0; i < numHygrometers; i++)
                value += hygrometers[i]->GetValue();

            return min(1.0f, max(value / numHygrometers, 0.0f));
        }

        bool isWatering = false;

        bool UpdateWateringState() {
            if (isOverride)
                return overrideWater;

            if (isWatering && GetAverage() > map_max)
                isWatering = false;

            if (!isWatering && GetAverage() < map_min)
                isWatering = true;

            return isWatering;
        }

        void SetOverride(bool enable, bool water) {
            isOverride = enable;
            overrideWater = water;
        }

        bool isOverride = false;
        bool overrideWater = false;
    };

    class Module {
    public:
        Module(fNETController::Module* m) {
            mdl = m;
            module_mac = m->MAC_Address;

            SetupTunnel();
        }

        Module(JsonObject o) {
            module_mac = o["mac"].as<String>();
            mdl = fNETController::GetModuleByMAC(module_mac);

            LoadData(o["data"].as<JsonObject>());
            SetupTunnel();
        }

        DynamicJsonDocument Save() {
            DynamicJsonDocument d = DynamicJsonDocument(256);
            d["mac"] = module_mac;
            d["data"] = SaveData();

            return d;
        }

        String module_mac = "";
        fNETController::Module* mdl = nullptr;
        fNETTunnel* tunnel;

        bool ok = false;
        bool value_received = false;

        virtual void Update() {
            if (mdl == nullptr || !mdl->isOnline) {
                ok = false;
                value_received = false;

                tunnel->SetRemoteMAC("");
                return;
            }

            if (tunnel->remoteMAC == "") {
                ESP_LOGE("fGMS", "Module: %s : Connected to network, beginning.", module_mac);
                tunnel->SetRemoteMAC(module_mac);
            }

            if (!tunnel->IsConnected) {
                ok = false;
                return;
            }

            if (millis() - lastMessageReceivedMillis > 10000 && (ok || value_received)) {
                value_received = false;
                ok = false;

                ESP_LOGE("fGMS", "Module: %s : Message timeout!", module_mac);
            }

            if (value_received)
                ok = true;
        }

    protected:
        virtual void OnMessage(DynamicJsonDocument* p) {

        }

        virtual DynamicJsonDocument SaveData() {
            return DynamicJsonDocument(0);
        }

        virtual void LoadData(JsonObject o) {

        }

    private:
        void SetupTunnel() {
            tunnel = new fNETTunnel(fNETController::Connection, "data");
            tunnel->Init();

            tunnel->OnMessageReceived.AddHandler(new EventHandler<Module>(this, [](Module* m, void* d) { m->message_received((DynamicJsonDocument*)d); }));

            lastMessageReceivedMillis = millis();
        }

        void message_received(DynamicJsonDocument* p) {
            OnMessage(p);

            lastMessageReceivedMillis = millis();
            value_received = true;
        }

        long lastMessageReceivedMillis = 0;
        long lastConnectionAttempt = -10000;
    };

    class HygrometerModule : public Module {
    public:
        HygrometerModule(fNETController::Module* m) : Module(m) { }
        HygrometerModule(JsonObject o) : Module(o) { }

        int Channels = -1;
        float Values[16];

    private:
        void OnMessage(DynamicJsonDocument* p) override {
            DynamicJsonDocument r = *p;
            int i = 0;

            for (float f : r["hygrometers"].as<JsonArray>())
                Values[i++] = f / 100.0;

            Channels = i;
        }
    };

    class ValveModule : public Module {
    public:
        ValveModule(fNETController::Module* m) : Module(m) { }
        ValveModule(JsonObject o) : Module(o) { }

        int State = 0b0000;
        int NextState = 0b0000;

        long LastStateSent = 0;

        void Update() {
            Module::Update();

            if (!ok)
                return;

            if (State != NextState && millis() - LastStateSent > 10000)
                SetState(NextState);
        }

        bool SetState(int state) {
            Serial.println("[fGMS Valve Controller] Setting valve state of module " + module_mac + ", state: " + String(state));

            DynamicJsonDocument d(512);
            DynamicJsonDocument result(256);
            d["state"] = state;
            bool success = fNET->Query(module_mac, "setState", &d, &result);

            if (!success) {
                Serial.println("[fGMS Valve Controller] Setting valve state of module " + module_mac + " failed!");
                ok = false;
                return false;
            }
            else {
                State = result["state"].as<int>();
                NextState = State;
            }

            LastStateSent = millis();
            Serial.println("[fGMS Valve Controller] Set valve state of module: " + module_mac + ", new state: " + String(state));
            return true;
        }

        int SetState(int valve, bool state) {
            uint8_t prev_state = State;
            uint8_t bitmask = (int)pow(2, valve);

            bool state_different = ((bitmask & prev_state) > 0) != state;

            if (state_different) {
                uint8_t new_state = prev_state ^ ((int)pow(2, valve));
                return SetState(new_state) ? 1 : -1;
            }
            else
                return 0;
        }

        bool QueueState(int valve, bool state) {
            uint8_t prev_state = State;
            uint8_t bitmask = (int)pow(2, valve);

            bool state_different = ((bitmask & prev_state) > 0) != state;

            //Serial.println("[fGMS Valve Controller] Queuing valve state of module " + module_mac + ", valve: " + String(valve) + ", state: " + String(NextState));

            if (state_different) {
                uint8_t new_state = prev_state ^ ((int)pow(2, valve));
                NextState = new_state;
                LastStateSent = 0;

                Serial.println("[fGMS Valve Controller] Queuing valve state of module " + module_mac + ", state: " + String(NextState));
                return true;
            }
            else
                return false;
        }
    };

    class SensorModule : public Module {
    public:
        SensorModule(fNETController::Module* m) : Module(m) { }
        SensorModule(JsonObject o) : Module(o) { }

        bool FanState = false;

        float ppm, temp, humidity;

        bool SetFan(bool on) {
            Serial.println("[fGMS Air Sensor] Setting fan state of module " + module_mac + ", state: " + (on ? "on" : "off"));

            DynamicJsonDocument d(512);
            DynamicJsonDocument result(256);
            d["enabled"] = on;
            bool success = fNET->Query(module_mac, "setFan", &d, &result);

            if (!success) {
                Serial.println("[fGMS Air Sensor] Setting fan state of module " + module_mac + " failed!");
                ok = false;
                return false;
            }
            else
                FanState = result["enabled"].as<String>() != "0";

            return true;
        }

    private:
        void OnMessage(DynamicJsonDocument* p) override {
            DynamicJsonDocument r = *p;
            ppm = r["co2PPM"];
            temp = r["temp"];
            humidity = r["humidity"];
        }
    };

    static fNETConnection* fNET;

    static Hygrometer* Hygrometers[32];
    static int HygrometerCount;

    static HygrometerGroup* HygrometerGroups[16];
    static int HygrometerGroupCount;

    static HygrometerModule* HygrometerModules[4];
    static int HygrometerModuleCount;

    static ValveModule* ValveModules[4];
    static int ValveModuleCount;

    static SensorModule* SensorModules[4];
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
        for (int idx = 0; idx < HygrometerGroupCount; idx++) {
            HygrometerGroups[idx]->RemoveHygrometer(Hygrometers[i]);
        }

        delete Hygrometers[i];
        for (int index = i; index < HygrometerCount - 1; index++) {
            Hygrometers[index]->id--;
            Hygrometers[index] = Hygrometers[index + 1];
        }

        HygrometerCount--;
    }

    static void RemoveHygrometerGroup(int i) {
        delete HygrometerGroups[i];
        for (int index = i; index < HygrometerGroupCount - 1; index++)
            HygrometerGroups[index] = HygrometerGroups[index + 1];

        HygrometerGroupCount--;
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

    static HygrometerGroup* GetHygrometerGroupByHygrometer(Hygrometer* m) {
        for (int i = 0; i < HygrometerGroupCount; i++) {
            HygrometerGroup* gr = HygrometerGroups[i];
            for (int j = 0; j < gr->numHygrometers; j++)
            {
                Hygrometer* meter = gr->hygrometers[j];

                if (meter == m)
                    return gr;
            }
        }

        return nullptr;
    }

    static HygrometerModule* AddHygrometerModule(fNETController::Module* mdl) {
        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Add hygrometer module: %s", mdl->MAC_Address.c_str());

        for (int i = 0; i < HygrometerModuleCount; i++)
            if (HygrometerModules[i]->module_mac == mdl->MAC_Address)
                return nullptr;

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

    static ValveModule* GetValveModuleByMAC(String m) {
        Serial.println(m);
        for (int i = 0; i < ValveModuleCount; i++) {
            Serial.println(ValveModules[i]->module_mac);
            if (ValveModules[i]->module_mac == m)
                return ValveModules[i];
        }

        return nullptr;
    }

    static ValveModule* AddValveModule(fNETController::Module* mdl) {
        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Add valve module: %s", mdl->MAC_Address.c_str());

        for (int i = 0; i < ValveModuleCount; i++)
            if (ValveModules[i]->module_mac == mdl->MAC_Address)
                return nullptr;

        ValveModule* m = new ValveModule(mdl);
        ValveModules[ValveModuleCount++] = m;

        return m;
    }

    static SensorModule* AddSensorModule(fNETController::Module* mdl) {
        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Add sensor module: %s", mdl->MAC_Address.c_str());

        for (int i = 0; i < SensorModuleCount; i++)
            if (SensorModules[i]->module_mac == mdl->MAC_Address)
                return nullptr;

        SensorModule* m = new SensorModule(mdl);
        SensorModules[SensorModuleCount++] = m;

        return m;
    }

    static void SetWatering(int period, int active) {
        watering_period = period;
        watering_active_period = active;
    }

    static void Init(fNETConnection* c) {
        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Init...");

        fNET = c;

        ReadConfig();

        //ntp_client.begin();

        xTaskCreate(update_task, "fGMS", 4096, nullptr, 0, nullptr);
        xTaskCreate(watering_task, "fGMS_Watering", 4096, nullptr, 0, nullptr);
    }

    static void Save() {
        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Saving...");
        DynamicJsonDocument data(16384);

        JsonArray hygrometersArray = data.createNestedArray("hygrometers");


        for (int i = 0; i < HygrometerCount; i++) {
            JsonObject o = Hygrometers[i]->Save().as<JsonObject>();
            hygrometersArray.add(o);
        }

        JsonArray groupsArray = data.createNestedArray("groups");

        for (int i = 0; i < HygrometerGroupCount; i++) {
            JsonObject o = HygrometerGroups[i]->Save().as<JsonObject>();
            groupsArray.add(o);
        }
        JsonArray hygrometerModulesArray = data.createNestedArray("hygrometerModules");

        for (int i = 0; i < HygrometerModuleCount; i++)
            hygrometerModulesArray.add(HygrometerModules[i]->Save().as<JsonObject>());

        JsonArray valveModulesArray = data.createNestedArray("valveModules");

        for (int i = 0; i < ValveModuleCount; i++)
            valveModulesArray.add(ValveModules[i]->Save().as<JsonObject>());

        JsonArray sensorModulesArray = data.createNestedArray("sensorModules");

        for (int i = 0; i < SensorModuleCount; i++)
            sensorModulesArray.add(SensorModules[i]->Save().as<JsonObject>());

        data["greenhouseSizeX"] = greenhouse.x_size;
        data["greenhouseSizeY"] = greenhouse.y_size;

        data["serverEnabled"] = serverEnabled;

        data["wateringPeriod"] = watering_period;
        data["wateringActivePeriod"] = watering_active_period;

        data["auto"] = AutomaticWatering;

        String data_serialized;
        serializeJsonPretty(data, data_serialized);

        Serial.println("[fGMS] Saved data: " + data_serialized);

        if (data.overflowed()) {
            RemoteLog.log(ESP_LOG_ERROR, "fGMS", "Failed to save!");
            return;
        }

        File data_file = LittleFS.open("/fGMS_Data.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Saved.");
    }

    static bool serverEnabled;

    static WiFiUDP ntp_udp;
    static NTPClient ntp_client;

    static bool AutomaticWatering;

    static int watering_period;
    static int watering_active_period;

    static bool IsWatering() {
        if (!AutomaticWatering)
            return false;

        unsigned long epoch;
        if (WiFi.isConnected())
            epoch = ntp_client.getEpochTime();
        else
            epoch = millis() / 1000;

        int phase = epoch % watering_period;
        return phase < watering_active_period;
    }

    static int GetSecondsUntilWatering() {
        if (IsWatering())
            return 0;

        if (watering_period == 0)
            return 0;

        unsigned long epoch;
        if (WiFi.isConnected())
            epoch = ntp_client.getEpochTime();
        else
            epoch = millis() / 1000;

        int phase = epoch % watering_period;
        return watering_period - phase;
    }

    static int GetSecondsToWater() {
        if (!IsWatering())
            return 0;

        if (watering_period == 0)
            return 0;

        unsigned long epoch;
        if (WiFi.isConnected())
            epoch = ntp_client.getEpochTime();
        else
            epoch = millis() / 1000;

        int phase = epoch % watering_period;
        return watering_active_period - phase;
    }

private:
    static void update_task(void* param) {
        long last_refreshed = 0;
        while (true) {
            delay(75);

            for (int i = 0; i < HygrometerModuleCount; i++)
                HygrometerModules[i]->Update();

            for (int i = 0; i < ValveModuleCount; i++)
                ValveModules[i]->Update();

            for (int i = 0; i < SensorModuleCount; i++)
                SensorModules[i]->Update();


            if(WiFi.isConnected())
                ntp_client.update();

            if (millis() - last_refreshed > 5000) {
                updateHygrometerModules();
                updateValveModules();
                updateSensorModules();

                last_refreshed = millis();
            }
        }
    }

    static void watering_task(void* param) {
        while (true) {
            delay(100);

            if (AutomaticWatering)
                update_watering();
        }
    }

    static void updateHygrometerModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++) {
            fNETController::Module* c = fNETController::Modules[i];
            if (c->Config["ModuleType"] != "HygroCtl")
                continue;

            if (!c->isOnline)
                continue;

            bool skip = false;
            for (int i = 0; i < HygrometerModuleCount; i++)
                if (HygrometerModules[i]->module_mac == c->MAC_Address) {
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
            fNETController::Module* c = fNETController::Modules[i];
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

    static void updateSensorModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++) {
            fNETController::Module* c = fNETController::Modules[i];
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
        }
    }

    static void LoadHygrometerModule(JsonObject o) {
        HygrometerModule* m = new HygrometerModule(o);

        if (!fNET->IsAddressValid(m->module_mac)) {
            delete m;
            return;
        }

        if (m->mdl == nullptr) {
            delete m;
            return;
        }

        if (m->mdl->Config["ModuleType"] != "HygroCtl") {
            delete m;
            return;
        }

        for (int i = 0; i < HygrometerModuleCount; i++) {
            if (HygrometerModules[i]->module_mac == m->module_mac) {
                delete m;
                return;
            }
        }

        HygrometerModules[HygrometerModuleCount++] = m;
    }

    static void LoadValveModule(JsonObject o) {
        ValveModule* m = new ValveModule(o);

        if (!fNET->IsAddressValid(m->module_mac)) {
            delete m;
            return;
        }

        if (m->mdl == nullptr) {
            delete m;
            return;
        }

        if (m->mdl->Config["ModuleType"] != "ValveCtl") {
            delete m;
            return;
        }

        for (int i = 0; i < ValveModuleCount; i++) {
            if (ValveModules[i]->module_mac == m->module_mac) {
                delete m;
                return;
            }
        }

        ValveModules[ValveModuleCount++] = m;
    }

    static void LoadSensorModule(JsonObject o) {
        SensorModule* m = new SensorModule(o);

        if (!IsValidMACAddress(m->module_mac)) {
            delete m;
            return;
        }

        if (m->mdl == nullptr) {
            delete m;
            return;
        }

        if (m->mdl->Config["ModuleType"] != "AirSensor") {
            delete m;
            return;
        }

        for (int i = 0; i < SensorModuleCount; i++) {
            if (SensorModules[i]->module_mac == m->module_mac) {
                delete m;
                return;
            }
        }

        SensorModules[SensorModuleCount++] = m;
    }

    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            return;

        fs::File data_file = LittleFS.open("/fGMS_Data.json", "r");

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

        for (JsonObject o : data["hygrometerModules"].as<JsonArray>())
            LoadHygrometerModule(o);

        for (JsonObject o : data["valveModules"].as<JsonArray>())
            LoadValveModule(o);

        for (JsonObject o : data["sensorModules"].as<JsonArray>())
            LoadSensorModule(o);

        for (JsonObject o : data["hygrometers"].as<JsonArray>())
            Hygrometers[HygrometerCount++] = new Hygrometer(o);

        for (JsonObject o : data["groups"].as<JsonArray>())
            HygrometerGroups[HygrometerGroupCount++] = new HygrometerGroup(o);

        greenhouse.x_size = data["greenhouseSizeX"];
        greenhouse.y_size = data["greenhouseSizeY"];

        serverEnabled = data["serverEnabled"];

        watering_period = data["wateringPeriod"];
        watering_active_period = data["wateringActivePeriod"];

        AutomaticWatering = data["auto"];

        RemoteLog.log(ESP_LOG_INFO, "fGMS", "Loaded");
    }

    static void update_watering() {
        if (WiFi.isConnected() && ntp_client.getEpochTime() < 1600000000 /*around 13 sep 2020*/)
            return; //NTP not set up.

        bool watering = IsWatering();


        for (int i = 0; i < HygrometerModuleCount; i++) {
            HygrometerModule* m = HygrometerModules[i];
            if (!m->ok)
                watering = false;
        }

        if (!watering) {
            for (int i = 0; i < ValveModuleCount; i++)
            {
                ValveModule* v = ValveModules[i];
                if (v->State > 0)
                    v->NextState = 0b0000;
            }

            return;
        }

        for (int i = 0; i < HygrometerGroupCount; i++) {
            HygrometerGroup* g = HygrometerGroups[i];
            bool shouldWater = g->UpdateWateringState();

            if (g->mod != nullptr)
                g->mod->QueueState(g->channel, shouldWater);
        }
    }
};

#endif // !fGMS_h
