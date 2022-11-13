#pragma once

#ifndef fGMSModule_h
#define fGMSModule_h

//#define I2C_BUFFER_LENGTH 4096

#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

#include "fGMSStringFunctions.h"
#include "fGMSMessages.h"

#define fGMS_ModuleTimeoutMS 500

enum fGMSModuleState {
    CONNECTED_WORKING = 10,
    CONNECTED_IDLE = 9,
    CONNECTED_ERR = 8,

    DISCONNECTED = 0,
    DISCONNECTED_ERROR = -2,
    FATAL_ERROR = -3
};

class fGMSModule {
public:
    static void Init() {
        Serial.println("[fGMS] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        SetPins();
        ReadConfig();

        SetupI2C();

        xTaskCreate(MainTask, "fGMS_MainTask", 8192, nullptr, 0, nullptr);
    }

    static String ModuleType;

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fGMS] Saving...");
        String data_serialized;

        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fGMS_ModuleData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();
        Serial.println("[fGMS] Saved.");
    }

    static void SendToController(String tag, String data) {
        DynamicJsonDocument d(1024);
        d["tag"] = tag;
        d["data"] = data;
        d["recipient"] = "controller";
        d["source"] = WiFi.macAddress();

        String str;

        serializeJson(d, str);

        I2C_Queue("JSON" + str);
    }

    static void SendToController(DynamicJsonDocument data) {
        DynamicJsonDocument d(data);
        

        if (!d.containsKey("source"))
            d["source"] = WiFi.macAddress();

        if (!d.containsKey("recipient"))
            d["recipient"] = "controller";

        String str;
        serializeJson(d, str);

        I2C_Queue("JSON" + str);
    }

    static JsonObject* Query(DynamicJsonDocument q) {
        int sentQueryID = LastQueryID++;

        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Querying " + q["recipient"].as<String>() + ": " + q["query"].as<String>());

        q["queryID"] = sentQueryID;
        q["tag"] = "query";

        SendToController(q);

        long startWaitms = millis();
        while (millis() - startWaitms < 5000) {
            delay(250);

            for (int i = 0; i < ReceivedJSONBuffer.size(); i++) {
                DynamicJsonDocument* r = ReceivedJSONBuffer[i];
                //Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] check returned data:" + String(i));
                //Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] data:" + r["tag"]);

                if ((*r)["tag"] == "queryResult" && (*r)["queryID"] == sentQueryID) {
                    Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Received query return.");
                    return new JsonObject((*r)["queryResult"].as<JsonObject>());
                }
            }
        }

        return nullptr;
    }

    static void AddQueryResponder(String query, DynamicJsonDocument(*Response)(DynamicJsonDocument)) {
        Responders[ResponderNum] = new fGMSQueryResponder(query, Response);
        ResponderNum++;
    }

    static fGMSModuleState State;

private:
    static TwoWire I2C;

    static uint8_t I2C_Address;

    static bool I2C_Enabled;

    static bool err, fatal_err, working;

    static void MainTask(void* param) {
        while (true) {
            I2C_CheckConnection();
            UpdateSignalState();

            delay(250);
        }
    }

    static void SetPins() {
        pinMode(fGMS_PIN_INDICATOR_G, OUTPUT);
        pinMode(fGMS_PIN_INDICATOR_Y, OUTPUT);
        pinMode(fGMS_PIN_INDICATOR_R, OUTPUT);

        //pinMode(2, OUTPUT);

        digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);
    }

    static void UpdateSignalState() {
        bool blink = (millis() % 1000) < 500;

        if (I2C_IsConnected) {
            State = CONNECTED_IDLE;

            if (working)
                State = CONNECTED_WORKING;

            if (err)
                State = CONNECTED_ERR;
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
            digitalWrite(fGMS_PIN_INDICATOR_G, LOW);
            digitalWrite(fGMS_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);

            break;

        case DISCONNECTED_ERROR:
            digitalWrite(fGMS_PIN_INDICATOR_G, LOW);
            digitalWrite(fGMS_PIN_INDICATOR_Y, blink);
            digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);

            break;

        case FATAL_ERROR:
            digitalWrite(fGMS_PIN_INDICATOR_G, LOW);
            digitalWrite(fGMS_PIN_INDICATOR_Y, blink);
            digitalWrite(fGMS_PIN_INDICATOR_R, !blink);
            break;

        case CONNECTED_WORKING:
            digitalWrite(fGMS_PIN_INDICATOR_G, HIGH);
            digitalWrite(fGMS_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);

            break;

        case CONNECTED_IDLE:
            digitalWrite(fGMS_PIN_INDICATOR_G, blink);
            digitalWrite(fGMS_PIN_INDICATOR_Y, HIGH);
            digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);

            break;

        case CONNECTED_ERR:
            digitalWrite(fGMS_PIN_INDICATOR_G, HIGH);
            digitalWrite(fGMS_PIN_INDICATOR_Y, blink);
            digitalWrite(fGMS_PIN_INDICATOR_R, HIGH);

            break;
        }
    }

    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted) {
            xTaskCreate(err_LittleFSMountError, "fgms_err_mount", 1024, nullptr, 0, nullptr);
            return;
        }

        File data_file = LittleFS.open("/fGMS_ModuleData.json", "r");

        if (!data_file) {
            xTaskCreate(err_DataFileNotFound, "fgms_err_config", 1024, nullptr, 0, nullptr);
            return;
        }

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fGMS] Read config: " + data_rawJson);

        I2C_Address = data["i2c_address"];
    }

    static void err_LittleFSMountError(void* param) {
        Serial.println("[fGMS LittleFS] LittleFS Mount error!");
        Serial.println("[fGMS LittleFS] Attempt flash? (y/n)");

        fatal_err = true;
        Error();

        while (!Serial.available()) {
            delay(100);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fGMS LittleFS] Flash cancelled.");
            delay(500);
            ESP.restart();
        }

        Serial.println("[fGMS LittleFS] Attempting to flash LittleFS.");
        Serial.println("[fGMS LittleFS] This should not take more than 30 seconds.");

        LittleFS.format();

        Serial.println("[fGMS LittleFS] LittleFS reflash done.");
        Serial.println("[fGMS LittleFS] Rebooting.");

        delay(5000);

        ESP.restart();
    }

    static void err_DataFileNotFound(void* param) {
        Serial.println("[fGMS] No data found!");
        Serial.println("[fGMS] Attempt reconfiguration?");

        Error();

        while (!Serial.available()) {
            delay(100);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fGMS] Reconfiguration cancelled.");
            delay(500);
            ESP.restart();
        }

        Reconfigure();
    }

    static void Reconfigure() {
        Serial.println("[fGMS] Reconfiguring...");

        data = DynamicJsonDocument(1024);
        data["ModuleType"] = ModuleType;
        data["i2c_address"] = I2C_Address;

        Save();
    }

    static void Error() {
        I2C_Enabled = false;
        err = true;
    }

    static unsigned long I2C_LastCommsMillis;
    static CircularBuffer<String, 100> I2C_SendBuffer;
    static String I2C_ToSend;

    static fGMS_I2CMessage* I2C_CurrentMessage;
    static int I2C_LastMsgID;


    static fGMS_I2CMessage* I2C_MessageFromMaster;

    static void SetupI2C() {
        if (I2C_Address < 0x01) {
            Serial.println("[fGMS fNET] No I2C Addres configured!");
            Serial.println("[fGMS fNET] Requesting new one from master.");

            I2C_Address = 0x01;
        }

        Serial.println("[fGMS fNET] Beginnig I2C as slave with address: " + String(I2C_Address));

        I2C.begin(I2C_Address, fGMS_SDA, fGMS_SCK, (uint32_t)800000);

        I2C.onReceive(I2C_TransactionReceive);

        I2C_Queue("test1");
        I2C_Queue("test2");
        I2C_Queue("test3");
        I2C_Queue("test4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAb");
        I2C_Queue("START  |1234567|1234567|1234567|1234567|1234567|1234567|1234567/");
        I2C_Queue("START  |1234567|1234567|1234567|1234567|1234567|1234567|1234567/1234567|1234567|1234567|1234567|1234567|1234567|1234567|1234567/1234567|1234567|1234567|1234567|1234567|1234567|1234567|1234567/1234567|1234567|1234567|1234567|1234567|1234567|1234567|1234567/1234567|1234567|1234567|1234567|1234567|1234567|1234567|12");

        I2C_Enabled = true;
    }

    static void I2C_Queue(String msg) {
        //Serial.println("[fGMS I2C] Queue data: " + msg);
        I2C_SendBuffer.push(msg);
    }

    static void I2C_BufferMessage(String data) {
        //Serial.println("[fGMS I2C] Send data: " + data);

        delete I2C_CurrentMessage;
        I2C_CurrentMessage = new fGMS_I2CMessage(data, I2C_LastMsgID);
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

        if (d["command"] == "getConfig")
            I2C_SendConfig();
        else if (d["command"] == "setI2CAddr")
            I2C_SetAddr(d["addr"].as<int>());
        else if (d["tag"] == "query")
            ProcessQuery(d);

        ReceivedJSONBuffer.unshift(new DynamicJsonDocument(d));
    }

    static void I2C_SendConfig() {
        String str_data;

        data["macAddress"] = WiFi.macAddress();

        serializeJson(data, str_data);

        SendToController("config", str_data);
    }

    static void I2C_SetAddr(uint8_t addr) {
        Serial.println("[fGMS fNET] Set address: " + String(addr));
        data["i2c_address"] = addr;

        Save();

        Serial.println("[fGMS fNET] Rebooting to apply changes.");
        ESP.restart();
    }

    static fGMSQueryResponder* Responders[32];
    static int ResponderNum;

    static void ProcessQuery(DynamicJsonDocument q) {
        DynamicJsonDocument r(4096);

        if (q["query"] == "getConfig") {
            r = data;
        }

        for (int i = 0; i < ResponderNum; i++) {
            if (Responders[i]->queryType == q["query"]) {
                r = Responders[i]->Response(q);
                break;
            }
        }

        DynamicJsonDocument send(4096);

        send["recipient"] = q["source"];
        send["tag"] = "queryResult";
        send["query"] = q["query"];
        send["queryID"] = q["queryID"];
        send["queryResult"] = r;

        String s_str;
        serializeJson(send, s_str);

        Serial.println("[fGMS fNET] Query response: " + s_str);
        SendToController(send);
    }

    static void I2C_TransactionReceive(int bytes) {
        if (!I2C_Enabled)
            return;

        char rc_buf[128];

        I2C.readBytes(rc_buf, bytes);

        String received = String(rc_buf);

        //Serial.println("[fGMS I2C] Received data:" + received);

        String trnum_data = received.substring(0, 8);
        int trnum = ParsePaddedInt(trnum_data);
        //Serial.println("[fGMS I2C] Received transaction num:" + String(trnum));
        received = received.substring(8);

        if (trnum != I2C_TransactionNum) {
            //Serial.println("[fGMS I2C] New transaction!");
            I2C_TransactionNum = trnum;

            String bytenum_data = received.substring(0, 4);
            int bytenum = ParsePaddedInt(bytenum_data);

            if (bytenum) {
                String content = received.substring(4, bytenum + 4);
                //Serial.println("[fGMS I2C] Transaction content: " + content);

                I2C_ToSend = "";

                I2C_DataReceived(content);
            }
        }

        String toSend = PaddedInt(I2C_TransactionNum, 8) + PaddedInt(I2C_ToSend.length(), 4) + I2C_ToSend;
        //Serial.println("[fGMS I2C] Reply: " + toSend);
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
            //xTaskCreate(I2C_NextMessageTask, "fGMS_I2C_NxtMsg", 10000, nullptr, 0, nullptr);
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
            I2C_MessageFromMaster = new fGMS_I2CMessage();

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
                I2C_MessageFromMaster = new fGMS_I2CMessage();
                Serial.println("[fGMS fNET] Message receive error ( msg id mismatch )");
                return;
            }

            if (pktid != I2C_MessageFromMaster_CurrentPacket)
            {
                Serial.println("[fGMS fNET] Message receive error ( pkt id mismatch )");
                Serial.println("[fGMS fNET] Expected packet: " + String(I2C_MessageFromMaster_CurrentPacket) + ", received: " + data);
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
            //Serial.println("[fGMS I2C] Received packet: " + pktData);
            I2C_MessageFromMaster->packets[I2C_MessageFromMaster_CurrentPacket] = fGMS_I2CPacket(pktData, pktid, msgid);
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
        I2C_MessageFromMaster = new fGMS_I2CMessage();

        Serial.println("[fGMS fNET] Master message received: " + received);

        if (received.substring(0, 4) == "JSON")
            I2C_OnReceiveJSON(received.substring(4));

        vTaskDelete(xTaskGetCurrentTaskHandle());
        delay(0);
    }

    static void I2C_NextMessage() {
        //Serial.println("[fGMS I2C] Next message.");

        if (I2C_SendBuffer.size() > 0) {
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
        //Serial.println("[fGMS I2C] Next message.");

        I2C_BufferMessage(I2C_SendBuffer.shift());

        vTaskDelete(xTaskGetCurrentTaskHandle());
        delay(1000);
    }

    static int I2C_TransactionNum;

    static bool I2C_IsConnected;

    static void I2C_OnDisconnect() {
        Serial.println("[fGMS fNet] Disconencted from controller!");
    }

    static void I2C_OnReconnect() {
        Serial.println("[fGMS fNet] Connected to controller!");
    }

    static void I2C_CheckConnection() {
        if (millis() - I2C_LastCommsMillis > fGMS_ModuleTimeoutMS) {
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

    static CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;
};

#endif