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
        Serial.println("[fNET] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        Connection = c;
        comm_tunnel = new fNETTunnel(c, "fNET_Comm");

        comm_tunnel->OnMessageReceived.AddHandler(new EventHandlerFunc([](void* d) {OnReceiveMessage(*(DynamicJsonDocument*)d); }));
        comm_tunnel->OnConnect.AddHandler(new EventHandlerFunc([](void* d) {OnReconnect(); }));
        comm_tunnel->OnDisconnect.AddHandler(new EventHandlerFunc([](void* d) {OnDisconnect(); }));

        comm_tunnel->Init();
        comm_tunnel->AcceptIncoming();

        SetPins();
        ReadConfig();

        if (IsValidMACAddress(data["controllerMac"])){
            fNET_ESPNOW::AddPeer(ToMACAddress(data["controllerMac"]));

            ControllerMAC = data["controllerMac"].as<String>();
            SendConfig();
        }
        else {
            Scan();
        }


        Connection->MessageReceived = OnReceiveMessage;

        xTaskCreate(MainTask, "fNET_MainTask", 8192, nullptr, 0, nullptr);

        return Connection;
    }

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fNET] Saving...");
        SendConfig();

        String data_serialized;

        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fNET_ModuleData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fNET] Saved.");
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

private:
    static bool err, fatal_err;
    static String ControllerMAC, ConnectionMAC;
    static long LastConnectionAttemptMS, LastOKMS;

    static void MainTask(void* param) {
        while (true) {
            UpdateSignalState();

            if (comm_tunnel->IsConnected)
                LastOKMS = millis();

            if (!comm_tunnel->IsConnected && millis() - LastOKMS > 5000 && IsValidMACAddress(ControllerMAC) && millis() - LastConnectionAttemptMS > 1000)
                SendConnectionRequest(ControllerMAC);

            if(!comm_tunnel->IsConnected && millis() - LastOKMS > 10000 && millis() - LastConnectionAttemptMS > 1000)
                Scan();

            if (comm_tunnel->IsConnected && data["controllerMac"] != ControllerMAC) {
                data["controllerMac"] = ControllerMAC;
                Save();
            }

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
        Serial.println("[fNET] Read config: " + data_rawJson);
    }

    static void Reset() {
        Serial.println("[fNET] Reconfiguring...");

        data = DynamicJsonDocument(1024);

        Save();
    }

    static void Error() {
        err = true;
    }

    static void OnReceiveMessage(DynamicJsonDocument d) {
        if (d.containsKey("command")) {
            Serial.println("Received command.");
            Serial.println("Received command.");
            if (d["command"] == "connected")
                OnReceiveConnect(d);

            if (d["command"] == "available")
                OnReceiveDiscovery(d);

            if (d["source"] != ControllerMAC)
                return;

            if (d["command"] == "getConfig")
                SendConfig();
        }
    }

    static void OnReceiveConnect(DynamicJsonDocument d) {
        if (ControllerMAC != "" && d["source"] != ControllerMAC)
            return;

        Serial.println("[fNET Module] Connected to controller: " + ControllerMAC);

        ControllerMAC = d["source"].as<String>();
        LastConnectionAttemptMS = millis();
        LastOKMS = millis();

        //comm_tunnel->TryConnect(ControllerMAC);

        SendConfig();
        ConnectionMAC = "";
    }

    static void OnReceiveDiscovery(DynamicJsonDocument r) {
        Serial.println("[fNET Module] Received available controller: " + ControllerMAC);

        if ((ControllerMAC != "" && r["source"] != ControllerMAC) || ConnectionMAC != "")
            return;

        SendConnectionRequest(r["source"]);
    }

    static void Scan() {
        Serial.println("[fNET Module] Scanning for available controllers.");

        DynamicJsonDocument d(4096);
        d["tag"] = "command";
        d["command"] = "request_available";
        d["recipient"] = "FF:FF:FF:FF:FF:FF";

        Connection->Send(d);
        LastConnectionAttemptMS = millis();

        ConnectionMAC = "";
        ControllerMAC = "";
    }

    static void SendConfig() {
        Serial.println("[fNET Module] Sending config to controller.");

        String str_data;
        String str_msg;
        DynamicJsonDocument d(4096);

        data["macAddress"] = WiFi.macAddress();

        serializeJson(data, str_data);

        d["tag"] = "config";
        d["data"] = str_data;

        comm_tunnel->Send(d);
    }

    static void SendConnectionRequest(String MAC) {
        Serial.println("Sending connection request to: " + MAC);
        DynamicJsonDocument d(4096);
        d["tag"] = "command";
        d["command"] = "request_connect";
        d["recipient"] = MAC;

        Connection->Send(d);

        ConnectionMAC = MAC;
        ControllerMAC = "";

        //comm_tunnel->Close(ConnectionMAC);
        LastConnectionAttemptMS = millis();
    }

    static void OnDisconnect() {
        Serial.println("[fNET fNet] Disconencted from controller!");
        digitalWrite(2, LOW);
    }

    static void OnReconnect() {
        Serial.println("[fNET fNet] Connected to controller!");
        digitalWrite(2, HIGH);
    }
};

#endif