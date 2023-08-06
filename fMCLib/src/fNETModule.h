#pragma once

#ifndef fNETModule_h
#define fNETModule_h

//#define I2C_BUFFER_LENGTH 4096

#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <CircularBuffer.h>
#include <esp_now.h>

#include "fNETStringFunctions.h"
#include "fNETMessages.h"
#include "fNET.h"

#define fNET_ModuleTimeoutMS 2000


enum fNETModuleState {
    CONNECTED_WORKING = 10,
    CONNECTED_IDLE = 9,
    CONNECTED_ERR = 8,

    DISCONNECTED = 0,
    DISCONNECTED_ERROR = -2,
    FATAL_ERROR = -3
};

class fNETModule {
public:
    static fNETConnection* Connection;
    static fNETTunnel* comm_tunnel;

    static fNETConnection* Init(fNETConnection* c) {
        ESP_LOGI("fNET Module", "Build date / time: % s / % s", __DATE__, __TIME__);

        Connection = c;
        comm_tunnel = new fNETTunnel(c, "fNET_Comm");

        comm_tunnel->OnMessageReceived.AddHandler(new EventHandlerFunc([](void* d) {OnReceiveMessage((DynamicJsonDocument*)d, true); }));
        comm_tunnel->OnConnect.AddHandler(new EventHandlerFunc([](void* d) {OnReconnect(); }));
        comm_tunnel->OnDisconnect.AddHandler(new EventHandlerFunc([](void* d) {OnDisconnect(); }));

        comm_tunnel->Init();
        comm_tunnel->AcceptIncoming();

        SetPins();
        ReadConfig();

        if (Connection->IsAddressValid(data["controllerMac"])) {
            ESP_LOGI("fNET Module", "Controller MAC:", data["controllerMac"].as<String>().c_str());

            ControllerMAC = data["controllerMac"].as<String>();
            SendConfig();
        }
        else {
            ESP_LOGI("fNET Module", "Scanning for controllers.");
            Scan();
        }


        Connection->MessageReceived = OnReceiveMessage;

        xTaskCreate(MainTask, "fNET_MainTask", 8192, nullptr, 0, nullptr);
        return Connection;
    }

    static DynamicJsonDocument data;

    static void Save() {
        ESP_LOGI("fNET Module", "Saving.");
        SendConfig();

        String data_serialized;

        serializeJson(data, data_serialized);

        ESP_LOGI("fNET Module", "New serialized data: %s", data_serialized.c_str());

        File data_file = LittleFS.open("/fNET_ModuleData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        ESP_LOGI("fNET Module", "Saved.");
    }

    static void SetErrorState() {
        err = true;
    }

    static void SetFatalErrorState() {
        fatal_err = true;
    }

    static void SetName(String name) {
        data["name"] = name;
    }

    static fNETModuleState State;
    static bool working;

    static void Idle_blink(int ms) {
        int ms_start = millis();
        while (millis() - ms_start < ms) {
            long m = millis() % 1000;
            digitalWrite(fNET_PIN_INDICATOR_G, !(m < 250 && m > 000));
            digitalWrite(fNET_PIN_INDICATOR_Y, !(m < 500 && m > 250));
            digitalWrite(fNET_PIN_INDICATOR_R, !(m < 750 && m > 500));

            delay(125);
        }
    }

    static void Update() {
        UpdateSignalState();

        if (comm_tunnel->IsConnected)
            LastOKMS = millis();

        if (!comm_tunnel->IsConnected && millis() - LastOKMS > 5000 && Connection->IsAddressValid(ControllerMAC) && millis() - LastConnectionAttemptMS > 1000)
            SendConnectionRequest(ControllerMAC);

        if (!comm_tunnel->IsConnected && millis() - LastOKMS > 10000 && millis() - LastConnectionAttemptMS > 1000)
            Scan();

        if (comm_tunnel->IsConnected) {
            if (data["controllerMac"] != ControllerMAC) {
                data["controllerMac"] = ControllerMAC;
                Save();
            }

            if (!config_sent && millis() - LastConfigSentMS > 2000)
                SendConfig();

        }
    }

private:
    static bool err, fatal_err, config_sent;
    static String ControllerMAC, ConnectionMAC;
    static long LastConnectionAttemptMS, LastOKMS, LastConfigSentMS;

    static void MainTask(void* param) {
        while (true) {
            Update();
            delay(250);
        }
    }

    static void SetPins() {
        pinMode(fNET_PIN_INDICATOR_G, OUTPUT);
        pinMode(fNET_PIN_INDICATOR_Y, OUTPUT);
        pinMode(fNET_PIN_INDICATOR_R, OUTPUT);

        //pinMode(2, OUTPUT);

        digitalWrite(fNET_PIN_INDICATOR_G, HIGH);
        digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
        digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

        delay(250);

        digitalWrite(fNET_PIN_INDICATOR_G, LOW);
        digitalWrite(fNET_PIN_INDICATOR_Y, LOW);
        digitalWrite(fNET_PIN_INDICATOR_R, LOW);
    }

    static void UpdateSignalState() {
        /*if (!comm_tunnel->IsConnected && millis() - LastCommsMillis < 10000) {
            long m = millis() % 1000;
            digitalWrite(fNET_PIN_INDICATOR_G, (m < 250 && m > 000));
            digitalWrite(fNET_PIN_INDICATOR_Y, (m < 500 && m > 250) || m > 750);
            digitalWrite(fNET_PIN_INDICATOR_R, (m < 750 && m > 500));

            return;
        }*/

        bool blink = (millis() % 1000) < 500;

        if (!comm_tunnel->IsConnected) {
            State = CONNECTED_IDLE;

            if (working)
                State = CONNECTED_WORKING;

            if (err)
                State = CONNECTED_ERR;

            if (fatal_err)
                State = FATAL_ERROR;
        }
        else {
            State = DISCONNECTED;

            if (err || ControllerMAC == "")
                State = DISCONNECTED_ERROR;

            if (fatal_err)
                State = FATAL_ERROR;
        }

        switch (State) {
        case DISCONNECTED:
            digitalWrite(fNET_PIN_INDICATOR_G, LOW);
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

            break;

        case DISCONNECTED_ERROR:
            digitalWrite(fNET_PIN_INDICATOR_G, LOW);
            digitalWrite(fNET_PIN_INDICATOR_Y, blink);
            digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

            break;

        case FATAL_ERROR:
            digitalWrite(fNET_PIN_INDICATOR_G, LOW);
            digitalWrite(fNET_PIN_INDICATOR_Y, blink);
            digitalWrite(fNET_PIN_INDICATOR_R, !blink);
            break;

        case CONNECTED_WORKING:
            digitalWrite(fNET_PIN_INDICATOR_G, blink);
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

            break;

        case CONNECTED_IDLE:
            digitalWrite(fNET_PIN_INDICATOR_G, HIGH);
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

            break;

        case CONNECTED_ERR:
            digitalWrite(fNET_PIN_INDICATOR_G, HIGH);
            digitalWrite(fNET_PIN_INDICATOR_Y, blink);
            digitalWrite(fNET_PIN_INDICATOR_R, HIGH);

            break;
        }
    }

    static void ReadConfig() {
        LittleFS.begin(true);

        File data_file = LittleFS.open("/fNET_ModuleData.json", "r");

        if (!data_file) {
            Reset();
        }

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        ESP_LOGI("fNET Module", "Read config: %s", data_rawJson.c_str());
    }

    static void Reset() {
        ESP_LOGE("fNET Module", "Resetting.");

        data = DynamicJsonDocument(1024);

        Save();
    }

    static void Error() {
        err = true;
    }

    static void OnReceiveMessage(DynamicJsonDocument* dat) {
        OnReceiveMessage(dat, false);
    }

    static void OnReceiveMessage(DynamicJsonDocument* dat, bool secure = false) {
        DynamicJsonDocument& d = *dat;

        if (d.containsKey("command")) {
            ESP_LOGV("fNET Module", "Received Command.");

            if (d["command"] == "connected")
                OnReceiveConnect(d);

            if (d["command"] == "available")
                OnReceiveDiscovery(d);

            if (!secure && d["source"] != ControllerMAC)
                return;

            if (d["command"] == "getConfig")
                SendConfig();

            if (d["command"] == "restart")
                ESP.restart();
        }
    }

    static void OnReceiveConnect(DynamicJsonDocument d) {
        if (ControllerMAC != "" && d["source"] != ControllerMAC)
            return;

        ESP_LOGD("fNET Module", "Connected to controller: %s", ControllerMAC.c_str());

        ControllerMAC = d["source"].as<String>();
        LastConnectionAttemptMS = millis();
        LastOKMS = millis();

        //comm_tunnel->TryConnect(ControllerMAC);

        SendConfig();
        ConnectionMAC = "";
    }

    static void OnReceiveDiscovery(DynamicJsonDocument r) {
        ESP_LOGD("fNET Module", "Received available controller: %s", ControllerMAC.c_str());

        if ((ControllerMAC != "" && r["source"] != ControllerMAC) || ConnectionMAC != "")
            return;

        SendConnectionRequest(r["source"]);
    }

    static void Scan() {
        ESP_LOGI("fNET Module", "Scanning for available controllers.");

        DynamicJsonDocument d(4096);
        d["tag"] = "command";
        d["command"] = "request_available";
        d["recipient"] = "broadcast";

        Connection->Send(d);
        LastConnectionAttemptMS = millis();

        ConnectionMAC = "";
        ControllerMAC = "";
    }

    static void SendConfig() {
        ESP_LOGD("fNET Module", "Sending config to controller.");

        String str_data;
        String str_msg;
        DynamicJsonDocument d(4096);

        data["macAddress"] = Connection->mac;

        serializeJson(data, str_data);

        d["type"] = "config";
        d["data"] = str_data;

        if (comm_tunnel->Send(d))
            config_sent = true;

        LastConfigSentMS = millis();
    }

    static void SendConnectionRequest(String MAC) {
        ESP_LOGD("fNET Module", "Sending connection request to: %s", MAC.c_str());
        DynamicJsonDocument d(4096);
        d["tag"] = "command";
        d["command"] = "request_connect";
        d["recipient"] = MAC;

        Connection->Send(d);

        ConnectionMAC = MAC;
        ControllerMAC = "";
        config_sent = false;

        //comm_tunnel->Close(ConnectionMAC);
        LastConnectionAttemptMS = millis();
    }

    static void OnDisconnect() {
        ESP_LOGW("fNET Module", "Disconencted from controller!");
        digitalWrite(2, LOW);
    }

    static void OnReconnect() {
        ESP_LOGW("fNET Module", "Connected to controller!");
        digitalWrite(2, HIGH);
    }
};

#endif