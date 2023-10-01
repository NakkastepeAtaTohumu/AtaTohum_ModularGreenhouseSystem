#pragma once

#ifndef fNETController_h
#define fNETController_h

#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <CircularBuffer.h>
#include <esp_now.h>
#include <RemoteUDPLogging.h>

#include "fNETStringFunctions.h"
#include "fNETMessages.h"
#include "fNETLib.h"
#include "fEvents.h"


class fNETController {
public:
    class Module {
    public:
        String MAC_Address = "00:00:00:00:00:00";

        bool isOnline = false;
        bool valid = false;
        fNETTunnel* comm_tunnel;

        Module(String mac_address) : Config(1024) {
            MAC_Address = mac_address;
            valid = true;
            isOnline = false;

            comm_tunnel = new fNETTunnel(Connection, mac_address, "fNET_Comm");
            comm_tunnel->OnMessageReceived.AddHandler(new EventHandler<Module>(this, [](Module* m, void* d) { m->OnReceiveJSON((DynamicJsonDocument*)d); }));
            comm_tunnel->OnConnect.AddHandler(new EventHandler<Module>(this, [](Module* m, void* d) { m->Reconnected(); }));
            comm_tunnel->OnDisconnect.AddHandler(new EventHandler<Module>(this, [](Module* m, void* d) { m->Disconnected(); }));

            comm_tunnel->Init();

            if (!ping_handler_added) {
                Connection->OnPingReceived.AddHandler(new EventHandlerFunc(OnPingReceived));
                ping_handler_added = true;
            }
        }

        void Update() {
            if (!isOnline)
                return;

            if (millis() - last_pinged_ms > 1000 && (!waiting_for_ping || millis() - last_pinged_ms > 10000)) {
                Connection->Ping(MAC_Address);
                waiting_for_ping = true;
                last_pinged_ms = millis();
            }

            if (millis() - last_config_requested_ms > 10000 && millis() - connected_ms > 4000 && (last_config_requested_ms == 0 || Config.isNull())) {
                RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Module: %s, Sent config request.", MAC_Address.c_str());

                SendCommand("getConfig");
                last_config_requested_ms = millis();
            }
        }

        void Remove() {
            int index = GetModuleIndex(this);

            //Serial.println("Removing " + String(index));

            for (int i = index; i < ModuleCount - 1; i++) {
                //Serial.println("Shift " + String(i));
                Modules[i] = Modules[i + 1];
            }

            Modules[ModuleCount] = nullptr;
            ModuleCount--;

            valid = false;
        }

        void Restart() {
            RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Module: %s, Sent restart request.", MAC_Address.c_str());
            SendCommand("restart");
        }

        void SetName(String name) {
            RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Module: %s, Sent set name request.", MAC_Address.c_str());

            DynamicJsonDocument d(256);
            d["command"] = "setName";
            d["name"] = name;

            comm_tunnel->Send(d);
        }

        void SendCommand(String command) {
            DynamicJsonDocument d(256);
            d["command"] = command;
            comm_tunnel->Send(d);
        }

        DynamicJsonDocument Config;
        int32_t Ping = -1;

    private:
        void OnReceiveJSON(DynamicJsonDocument* dp) {
            DynamicJsonDocument d = *dp;

            String c_str;
            serializeJson(d, c_str);

            RemoteLog.log(ESP_LOG_VERBOSE, "fNET Controller", "Module: %s, Received message: %s", MAC_Address.c_str(), c_str.c_str());

            if (d["type"] == "config")
                OnReceiveConfig(d["data"]);
        }

        void OnReceiveConfig(String d) {
            RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Module: %s, Received config.", MAC_Address.c_str());

            String c_str;
            serializeJson(Config, c_str);

            if (c_str != d)
            {
                RemoteLog.log(ESP_LOG_DEBUG, "fNET Controller", "Module: %s, Saved new config: %s", MAC_Address.c_str(), d.c_str());
                deserializeJson(Config, d);

                MAC_Address = Config["macAddress"].as<String>();

                //Save();
            }
        }

        void Disconnected() {
            RemoteLog.log(ESP_LOG_WARN, "fNET Controller", "Module: %s, Disconnected.", MAC_Address.c_str());
            isOnline = false;

            OnModuleDisconnected.Invoke((void*)this);
        }

        void Reconnected() {
            RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Module: %s, Connected.", MAC_Address.c_str());
            isOnline = true;
            connected_ms = millis();

            SendCommand("getConfig");

            OnModuleConnected.Invoke((void*)this);
        }

        static void OnPingReceived(void* pd) {
            PingData d = *(PingData*)pd;

            Module* m = GetModuleByMAC(d.mac);
            if (m != nullptr)
            {
                m->Ping = d.ping;
                m->last_pinged_ms = millis();
                m->waiting_for_ping = false;
            }
        }

        long last_pinged_ms = 0;
        long connected_ms = 0;
        long last_config_requested_ms = 0;
        bool waiting_for_ping = false;

        static bool ping_handler_added;
    };

public:
    static fNETConnection* Connection;
    static void Init(fNETConnection* c) {
        status_d = "init";
        Connection = c;
        Connection->MessageReceived = OnMessageReceived;

        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Build date / time: % s / % s", __DATE__, __TIME__);

        status_d = "read_cfg";
        ReadConfig();

        xTaskCreate(UpdateModules, "update_mdl", 4096, NULL, 0, NULL);

        //Serial.println("[fNET] Loading modules...");

        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Done initializing.");

        status_d = "ok";
    }

    static String SessionID;

    static DynamicJsonDocument* GetDataJSON() {
        DynamicJsonDocument* data = new DynamicJsonDocument(16384);
        JsonObject modulesObject = data->createNestedObject("moduleData");

        JsonArray modulesArray = modulesObject.createNestedArray("modules");

        for (int i = 0; i < ModuleCount; i++) {
            Module* mod = Modules[i];
            ESP_LOGV("fNET Controller", "Saving module: %s", String(mod->MAC_Address).c_str());

            JsonObject object = modulesArray.createNestedObject();

            object["macAddress"] = mod->MAC_Address;
            object["data"] = mod->Config.as<JsonObject>();
            object["isOnline"] = mod->isOnline;
        }

        data->shrinkToFit();

        return data;
    }

    static void Save() {
        ESP_LOGI("fNET Controller", "Saving.");
        DynamicJsonDocument& data = *GetDataJSON();

        String data_serialized;
        serializeJson(data, data_serialized);

        ESP_LOGD("fNET Controller", "New data: %s", data_serialized.c_str());

        if (data_serialized.length() < 8) {
            RemoteLog.log(ESP_LOG_ERROR, "fNET Controller", "Failed to save!");
            return;
        }

        File data_file = LittleFS.open("/fNET_SystemData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Saved.");

        delete& data;
    }

    static Module* GetModuleByMAC(String mac) {
        //Serial.println("[fNET] Get module: " + mac);
        for (int i = 0; i < ModuleCount; i++) {
            Module* d = Modules[i];

            if (d->MAC_Address == mac)
                return d;
        }
        //Serial.println("[fNET] Module not found");
        return nullptr;
    }

    static int GetModuleIndex(Module* c) {
        for (int i = 0; i < ModuleCount; i++) {
            if (Modules[i] == c) {
                return i;
            }
        }

        return -1;
    }

    static void Scan() {
        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Scanning for modules.");
        SendDiscovery("broadcast");
    }

    static Module* Modules[32];
    static int ModuleCount;

    static String status_d;

    static Event OnModuleConnected;
    static Event OnModuleDisconnected;
private:
    static void ReadConfig() {
        bool mounted = LittleFS.begin(true);

        File data_file = LittleFS.open("/fNET_SystemData.json", "r");

        if (!data_file)
            return;

        DynamicJsonDocument data(16384);

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Read config: %s", data_rawJson.c_str());

        status_d = "load_mdl";
        LoadModules(data["moduleData"]);
    }

    static void LoadModules(DynamicJsonDocument data) {
        JsonArray modulesArray = data["modules"];
        ModuleCount = 0;

        for (JsonObject o : modulesArray) {
            String mac = o["macAddress"];
            JsonObject config_o = o["data"];

            RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Loaded module: %s", mac.c_str());

            esp_now_peer_info_t peer_t = {};
            if (!Connection->IsAddressValid(mac))
                continue;

            Module* d = new Module(mac);
            d->Config = config_o;

            Modules[ModuleCount] = d;
            ModuleCount++;
        }
    }

    static void UpdateModules(void* param) {
        while (true) {
            for (int i = 0; i < ModuleCount; i++)
                Modules[i]->Update();

            delay(100);
        }
    }

    static void AddNewModule(String mac) {
        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Add new module: %s", mac.c_str());

        Module* dat = new Module(mac);
        //dat->SetI2CAddr(I2C_GetEmptyAddr(0));

        Modules[ModuleCount] = dat;
        ModuleCount++;
        //Save();
    }

    static void SendDiscovery(String recipient) {
        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Sending availability status to: %s", recipient.c_str());

        DynamicJsonDocument d(512);
        d["recipient"] = recipient;
        d["tag"] = "command";
        d["command"] = "available";

        Connection->Send(d);

        Module* m = GetModuleByMAC(recipient);
        if (m != nullptr)
        {
            //m->comm_tunnel->Close(recipient);
            m->isOnline = false;
        }
    }

    static void ConnectModule(String mac) {
        RemoteLog.log(ESP_LOG_INFO, "fNET Controller", "Connecting module: %s", mac.c_str());

        Module* m = GetModuleByMAC(mac);

        if (m == nullptr)
            AddNewModule(mac);

        DynamicJsonDocument d(512);
        d["recipient"] = mac;
        d["tag"] = "command";
        d["command"] = "connected";

        Connection->Send(d);
    }

    static void OnMessageReceived(DynamicJsonDocument* dat) {
        DynamicJsonDocument& d = *dat;

        if (d["tag"] == "command") {
            if (d["command"] == "request_connect")
                ConnectModule(d["source"]);
            else if (d["command"] == "request_available")
                SendDiscovery(d["source"]);
        }
    }
};
#endif