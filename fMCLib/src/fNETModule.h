#pragma once

#ifndef fNETModule_h
#define fNETModule_h

//#define I2C_BUFFER_LENGTH 4096

#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

#include "fNETStringFunctions.h"
#include "fNETMessages.h"
#include "fNET.h"

#define fNET_ModuleTimeoutMS 10000

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
    class ModuleConnection : public fNETConnection {
    public:
        ModuleConnection(String m) : fNETConnection(m) {

        }

        void QueueMessage(String d, String r) override {
            I2C_Queue(d);
        }

        void OnReceiveMessage(DynamicJsonDocument d) {
            fNETConnection::OnReceiveMessageService(d);
        }

        int GetQueuedMessageCount() override {
            return I2C_SendBuffer.size();
        }
    };

    static fNETConnection* Init() {
        Serial.println("[fNET] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        SetPins();
        ReadConfig();

        SetupI2C();

        xTaskCreate(MainTask, "fNET_MainTask", 8192, nullptr, 0, nullptr);

        Connection = new ModuleConnection(WiFi.macAddress());

        Connection->AddQueryResponder("getConfig", [](DynamicJsonDocument d) {
            return data;
            });

        return Connection;
    }

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fNET] Saving...");
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

    static int QueuedMessageCount() {
        return I2C_SendBuffer.size();
    }

    static void SetName(String name) {
        data["name"] = name;
    }

    static fNETModuleState State;
    static bool working;

private:
    static TwoWire I2C;

    static uint8_t I2C_Address;

    static bool I2C_Enabled;

    static bool err, fatal_err;

    static ModuleConnection* Connection;

    static void MainTask(void* param) {
        while (true) {
            I2C_CheckConnection();
            UpdateSignalState();

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
        if (!I2C_IsConnected && millis() - I2C_LastCommsMillis < 10000) {
            long m = millis() % 1000;
            digitalWrite(fNET_PIN_INDICATOR_G, (m < 250 && m > 000));
            digitalWrite(fNET_PIN_INDICATOR_Y, (m < 500 && m > 250) || m > 750);
            digitalWrite(fNET_PIN_INDICATOR_R, (m < 750 && m > 500));

            return;
        }

        bool blink = (millis() % 1000) < 500;

        if (I2C_IsConnected) {
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

            if (err)
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
        bool mounted = LittleFS.begin();

        if (!mounted) {
            xTaskCreate(err_LittleFSMountError, "fgms_err_mount", 4096, nullptr, 0, nullptr);
            return;
        }

        File data_file = LittleFS.open("/fNET_ModuleData.json", "r");

        if (!data_file) {
            xTaskCreate(err_DataFileNotFound, "fgms_err_config", 4096, nullptr, 0, nullptr);
            return;
        }

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fNET] Read config: " + data_rawJson);

        I2C_Address = data["i2c_address"];
    }

    static void err_LittleFSMountError(void* param) {
        Serial.println("[fNET LittleFS] LittleFS Mount error!");
        Serial.println("[fNET LittleFS] Attempt flash? (y/n)");

        delay(100);
        fatal_err = true;
        Error();

        while (!Serial.available()) {
            delay(100);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fNET LittleFS] Flash cancelled.");
            delay(500);
            ESP.restart();
        }

        Serial.println("[fNET LittleFS] Attempting to flash LittleFS.");
        Serial.println("[fNET LittleFS] This should not take more than 30 seconds.");

        LittleFS.format();

        Serial.println("[fNET LittleFS] LittleFS reflash done.");
        Serial.println("[fNET LittleFS] Rebooting.");

        delay(5000);

        ESP.restart();
    }

    static void err_DataFileNotFound(void* param) {
        Serial.println("[fNET] No data found!");
        Serial.println("[fNET] Attempt reconfiguration?");

        delay(100);
        Error();

        while (!Serial.available()) {
            delay(100);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fNET] Reconfiguration cancelled.");
            delay(500);
            ESP.restart();
        }

        Reconfigure();
    }

    static void Reconfigure() {
        Serial.println("[fNET] Reconfiguring...");

        data = DynamicJsonDocument(1024);
        data["i2c_address"] = 0;

        Save();
    }

    static void Error() {
        I2C_Enabled = false;
        err = true;
    }

    static long I2C_LastCommsMillis;
    static CircularBuffer<String, 100> I2C_SendBuffer;
    static String I2C_ToSend;

    static fNETMessage* I2C_CurrentMessage;
    static int I2C_LastMsgID;


    static fNETMessage* I2C_MessageFromMaster;

    static void SetupI2C() {
        if (I2C_Address < 0x01) {
            Serial.println("[fNET fNET] No I2C Addres configured!");
            Serial.println("[fNET fNET] Requesting new one from master.");

            I2C_Address = 0x01;
        }

        if (I2C_Address >= 128) {
            Serial.println("[fNET fNET] I2C Addres invalid!");
            Serial.println("[fNET fNET] Requesting new one from master.");

            I2C_Address = 0x01;
        }


        Serial.println("[fNET fNET] Beginnig I2C as slave with address: " + String(I2C_Address));

        bool success = I2C.begin(I2C_Address, fNET_SDA, fNET_SCK, (uint32_t)800000);

        if (!success) {
            Serial.println("[fNET fNET] I2C Failed to begin!");
            SetFatalErrorState();
            return;
        }

        Serial.println("[fNET fNET] I2C ok!");

        I2C.onReceive(I2C_TransactionReceive);

        I2C_Enabled = true;
    }

    static void I2C_Queue(String msg) {
        //Serial.println("[fNET I2C] Queue data: " + msg);
        I2C_SendBuffer.push(msg);
    }

    static void I2C_Queue(String msg, String destMAC) {
        //Serial.println("[fNET I2C] Queue data: " + msg);
        I2C_SendBuffer.push(msg);
    }

    static void I2C_BufferMessage(String data) {
        //Serial.println("[fNET I2C] Send data: " + data);

        delete I2C_CurrentMessage;
        I2C_CurrentMessage = new fNETMessage(data, I2C_LastMsgID);
        I2C_LastMsgID++;
    }

    static void I2C_DataReceived(String received) {
        //Serial.println("Received: " + received);

        if (received.substring(0, 4) == "PING")
            I2C_ToSend = "PONG";
        else if (received.substring(0, 3) == "CMD")
            I2C_ProcessCommsCommand(received);
        else if (received.substring(0, 6) == "GETMAC")
            I2C_ToSend = WiFi.macAddress();

        I2C_LastCommsMillis = millis();
    }

    static void I2C_OnReceiveJSON(String msg) {

        DynamicJsonDocument d(1024);

        deserializeJson(d, msg);

        //if (d["source"] != "controller")
        //    return;

        if (d.containsKey("command")) {
            //Serial.println("Received command.");
            if (d["command"] == "getConfig")
                I2C_SendConfig();
            else if (d["command"] == "setI2CAddr")
                I2C_SetAddr(d["addr"].as<int>());
        }
        else {
            Connection->OnReceiveMessage(d);
        }
    }

    static void I2C_SendConfig() {
        String str_data;
        String str_msg;
        DynamicJsonDocument d(4096);

        data["macAddress"] = WiFi.macAddress();

        serializeJson(data, str_data);

        d["tag"] = "config";
        d["data"] = str_data;

        Connection->Send(d);
    }

    static void I2C_SetAddr(uint8_t addr) {
        Serial.println("[fNET fNET] Set address: " + String(addr));
        data["i2c_address"] = addr;

        Save();

        Serial.println("[fNET fNET] Rebooting to apply changes.");
        ESP.restart();
    }

    static void I2C_TransactionReceive(int bytes) {
        if (!I2C_Enabled)
            return;

        char rc_buf[128];

        I2C.readBytes(rc_buf, bytes);

        String received = String(rc_buf);

        //Serial.println("[fNET I2C] Received data:" + received);

        String trnum_data = received.substring(0, 8);
        int trnum = ParsePaddedInt(trnum_data);
        //Serial.println("[fNET I2C] Received transaction num:" + String(trnum));
        received = received.substring(8);

        if (trnum != I2C_TransactionNum) {
            //Serial.println("[fNET I2C] New transaction!");
            I2C_TransactionNum = trnum;

            String bytenum_data = received.substring(0, 4);
            int bytenum = ParsePaddedInt(bytenum_data);

            if (bytenum) {
                String content = received.substring(4, bytenum + 4);
                //Serial.println("[fNET I2C] Transaction content: " + content);

                I2C_ToSend = "";

                I2C_DataReceived(content);
            }
        }

        String toSend = PaddedInt(I2C_TransactionNum, 8) + PaddedInt(I2C_ToSend.length(), 4) + I2C_ToSend;
        //Serial.println("[fNET I2C] Reply: " + toSend);
        I2C.print(toSend);
    }

    static void I2C_ProcessCommsCommand(String received) {
        String cmd = received.substring(3, 6);

        if (cmd == "PKT") {
            String pkt_data = received.substring(6, 10);
            //Serial.println("[fMS I2C] Select packet command.\n[fMS I2C] Packet data: " + pkt_data);
            int pkt_id = ParsePaddedInt(pkt_data.c_str());

            //Serial.println("[fMS I2C] Select packet #" + String(pkt_id));

            String toSend = I2C_CurrentMessage->packets[pkt_id].data;

            //Serial.println("[fMS I2C] ToSend: " + toSend);
            //Serial.println("[fMS I2C] Message ID: " + String(I2C_CurrentMessage.packets[pkt_id].messageID));
            //Serial.println("[fMS I2C] Packet ID: " + String(I2C_CurrentMessage.packets[pkt_id].packetID));
            I2C_ToSend = toSend;
        }
        else if (cmd == "MSG") {
            //xTaskCreate(I2C_NextMessageTask, "fNET_I2C_NxtMsg", 10000, nullptr, 0, nullptr);
            I2C_NextMessage();
        }
        else if (cmd == "SND") {
            I2C_ProcessPacketReceiveCommand(received);
        }
    }

    static int I2C_MessageFromMaster_CurrentPacket;

    static void I2C_ProcessPacketReceiveCommand(String received) {
        String cmd = received.substring(6, 9);

        if (cmd == "DAT") {
            String data = received.substring(9);

            int pktcount = ParsePaddedInt(data.substring(0, 4));
            int msgid = ParsePaddedInt(data.substring(4, 12));

            delete I2C_MessageFromMaster;
            I2C_MessageFromMaster = new fNETMessage();

            I2C_MessageFromMaster->packetCount = pktcount;
            I2C_MessageFromMaster->messageID = msgid;
            I2C_MessageFromMaster_CurrentPacket = 0;
        }
        else if (cmd == "PKT") {
            String data = received.substring(9);

            int pktid = ParsePaddedInt(data.substring(8, 12));
            int msgid = ParsePaddedInt(data.substring(0, 8));

            if (msgid != I2C_MessageFromMaster->messageID)
            {
                I2C_MessageFromMaster = new fNETMessage();
                Serial.println("[fNET fNET] Message receive error ( msg id mismatch )");
                return;
            }

            if (pktid != I2C_MessageFromMaster_CurrentPacket)
            {
                Serial.println("[fNET fNET] Message receive error ( pkt id mismatch )");
                Serial.println("[fNET fNET] Expected packet: " + String(I2C_MessageFromMaster_CurrentPacket) + ", received: " + data);
                I2C_ToSend = "PKT" + PaddedInt(I2C_MessageFromMaster_CurrentPacket, 4);
                return;
            }

            String pktData = data.substring(12);

            for (int i = pktData.length() - 1; i > 0; i--) {
                if (pktData[i] != '#')
                {
                    pktData = pktData.substring(0, i + 1);
                    break;
                }
            }
            //Serial.println("[fNET I2C] Received packet: " + pktData);
            I2C_MessageFromMaster->packets[I2C_MessageFromMaster_CurrentPacket] = fNET_I2CPacket(pktData, pktid, msgid);
            I2C_MessageFromMaster_CurrentPacket++;

            if (I2C_MessageFromMaster_CurrentPacket >= I2C_MessageFromMaster->packetCount) {
                I2C_ToSend = "FIN";
                xTaskCreate(I2C_MasterMessageReceived, "fNetReceiveTask", 10000, nullptr, 0, nullptr);
                //I2C_MasterMessageReceived();
            }
        }

        if (I2C_MessageFromMaster_CurrentPacket < I2C_MessageFromMaster->packetCount)
            I2C_ToSend = "PKT" + PaddedInt(I2C_MessageFromMaster_CurrentPacket, 4);
    }

    static void I2C_MasterMessageReceived(void* param) {
        String received = "";

        for (int i = 0; i < I2C_MessageFromMaster->packetCount; i++)
            received += I2C_MessageFromMaster->packets[i].content;

        delete I2C_MessageFromMaster;
        I2C_MessageFromMaster = new fNETMessage();

        //Serial.println("[fNET fNET] Master message received: " + received);

        if (received.substring(0, 4) == "JSON")
            I2C_OnReceiveJSON(received.substring(4));

        vTaskDelete(NULL);
        delay(0);
    }

    static void I2C_NextMessage() {
        //Serial.println("[fNET I2C] Next message.");

        if (I2C_SendBuffer.size() > 0) {
            //Serial.println("message: " + I2C_SendBuffer.first());
            String packetCountData = PaddedInt(ceil(((double)I2C_SendBuffer.first().length()) / 64.0), 4);
            String messageIDData = PaddedInt(I2C_LastMsgID, 4);

            String toSend = packetCountData + messageIDData;
            I2C_ToSend = toSend;

            I2C_BufferMessage(I2C_SendBuffer.shift());
            //xTaskCreate(I2C_NextMessageTask, "fGSI2C_msgtask", 10000, nullptr, 0, nullptr);
        }
        else {
            I2C_ToSend = "NONE";
        }
    }

    static void I2C_NextMessageTask(void* param) {
        //Serial.println("[fNET I2C] Next message.");

        I2C_BufferMessage(I2C_SendBuffer.shift());

        vTaskDelete(NULL);
        delay(1000);
    }

    static int I2C_TransactionNum;

    static bool I2C_IsConnected;

    static void I2C_OnDisconnect() {
        Serial.println("[fNET fNet] Disconencted from controller!");
        Connection->IsConnected = false;
    }

    static void I2C_OnReconnect() {
        Serial.println("[fNET fNet] Connected to controller!");
        Connection->IsConnected = true;
    }

    static void I2C_CheckConnection() {
        if (millis() - I2C_LastCommsMillis > fNET_ModuleTimeoutMS) {
            if (I2C_IsConnected) {
                I2C_OnDisconnect();
                I2C_IsConnected = false;
            }
        }
        else {
            if (!I2C_IsConnected) {
                I2C_OnReconnect();
                I2C_IsConnected = true;
            }
        }
    }

    static int LastQueryID;
};

#endif