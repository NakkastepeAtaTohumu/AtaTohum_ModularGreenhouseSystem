#pragma once

#ifndef fNETController_h
#define fNETController_h

#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <CircularBuffer.h>
#include <esp_now.h>

#include "fNETStringFunctions.h"
#include "fNETMessages.h"
#include "fNETLib.h"

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
        }

        void Update() {
        }

        void Remove() {
            int index = GetModuleIndex(this);

            Serial.println("Removing " + String(index));

            for (int i = index; i < ModuleCount - 1; i++) {
                Serial.println("Shift " + String(i));
                Modules[i] = Modules[i + 1];
            }

            Modules[ModuleCount] = nullptr;
            ModuleCount--;

            valid = false;
        }

        DynamicJsonDocument Config;

    private:
        void OnReceiveJSON(DynamicJsonDocument* dp) {
            DynamicJsonDocument d = *dp;

            if (d["tag"] == "config")
                OnReceiveConfig(d["data"]);
        }

        void OnReceiveConfig(String d) {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received config.");

            String c_str;
            serializeJson(Config, c_str);

            if (c_str != d)
            {
                Serial.println("[fNET fNET, Module: " + MAC_Address + "] Save new config: " + d);
                deserializeJson(Config, d);

                MAC_Address = Config["macAddress"].as<String>();

                //Save();
            }
        }

        void Disconnected() {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Disconnected!");
            isOnline = false;
            digitalWrite(2, LOW);
        }

        void Reconnected() {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Reconnected!");
            isOnline = true;
            digitalWrite(2, HIGH);
        }
    };

public:
    static fNETConnection* Connection;
    static void Init(fNETConnection* c) {
        status_d = "init";
        Connection = c;
        Connection->MessageReceived = OnMessageReceived;

        Serial.println("[fNET] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        status_d = "read_cfg";
        ReadConfig();

        Serial.println("[fNET] Loading modules...");

        Serial.println("[fNET] Adding peers...");
        for (int i = 0; i < ModuleCount; i++)
            if (IsValidMACAddress(Modules[i]->MAC_Address))
                fNET_ESPNOW::AddPeer(ToMACAddress(Modules[i]->MAC_Address));

        Serial.println("[fNET] Done.");

        Connection->IsConnected = true;
        status_d = "ok";
    }

    static String SessionID;

    static void Save() {
        Serial.println("[fNET] Saving...");
        DynamicJsonDocument data(32767);
        JsonObject modulesObject = data.createNestedObject("moduleData");

        JsonArray modulesArray = modulesObject.createNestedArray("modules");

        for (int i = 0; i < ModuleCount; i++) {
            Module* mod = Modules[i];
            Serial.println("[fNET Controller] Saving module " + String(mod->MAC_Address));

            JsonObject object = modulesArray.createNestedObject();

            object["macAddress"] = mod->MAC_Address;
            object["data"] = mod->Config.as<JsonObject>();
            object["isOnline"] = mod->isOnline;
        }

        String data_serialized;
        serializeJson(data, data_serialized);

        Serial.println("[fNET Controller] New serialized data: \n" + data_serialized);

        if (data_serialized.length() < 8) {
            Serial.println("[fNET Controller] Failed to save!");
            return;
        }

        File data_file = LittleFS.open("/fNET_SystemData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fNET] Saved.");
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
        Serial.println("[fNET Controller] Scanning for modules.");
        SendDiscovery("FF:FF:FF:FF:FF:FF");
    }

    static Module* Modules[32];
    static int ModuleCount;

    static String status_d;

private:
    static void ReadConfig() {
        bool mounted = LittleFS.begin(true);

        File data_file = LittleFS.open("/fNET_SystemData.json", "r");

        if (!data_file)
            return;

        DynamicJsonDocument data(65535);

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fNET Controller] Read config: " + data_rawJson);

        status_d = "load_mdl";
        LoadModules(data["moduleData"]);
    }

    static void LoadModules(DynamicJsonDocument data) {
        JsonArray modulesArray = data["modules"];
        ModuleCount = 0;

        for (JsonObject o : modulesArray) {
            String mac = o["macAddress"];
            JsonObject config_o = o["data"];

            Serial.println("[fNET Controller] Loaded module " + String(mac));

            esp_now_peer_info_t peer_t = {};

            Module* d = new Module(mac);
            d->Config = config_o;

            Modules[ModuleCount] = d;
            ModuleCount++;
        }
    }

    static void UpdateModules() {
        for (int i = 0; i < ModuleCount; i++)
            Modules[i]->Update();
    }

    static void AddNewModule(String mac) {
        Serial.println("[fNET Controller] Add new module: " + mac);

        Module* dat = new Module(mac);
        //dat->SetI2CAddr(I2C_GetEmptyAddr(0));

        fNET_ESPNOW::AddPeer(ToMACAddress(mac));

        Modules[ModuleCount] = dat;
        ModuleCount++;
        //Save();
    }

    static void SendDiscovery(String recipient) {
        Serial.println("[fNET Controller] Sending availability status to " + recipient);

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
        Serial.println("[fNET Controller] Connecting module: " + mac);

        Module* m = GetModuleByMAC(mac);
        if (m != nullptr)
            m->comm_tunnel->TryConnect(m->MAC_Address);
        else
            AddNewModule(mac);

        DynamicJsonDocument d(512);
        d["recipient"] = mac;
        d["tag"] = "command";
        d["command"] = "connected";

        Connection->Send(d);
    }

    static void OnMessageReceived(DynamicJsonDocument d) {
        if (d["tag"] == "command") {
            if (d["command"] == "request_connect")
                ConnectModule(d["source"]);
            else if (d["command"] == "request_available")
                SendDiscovery(d["source"]);
        }
    }
};
#endif