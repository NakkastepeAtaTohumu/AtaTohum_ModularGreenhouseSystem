#pragma once

#ifndef fNETController_h
#define fNETController_h

#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <UUID.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

#include "fNETStringFunctions.h"
#include "fNETMessages.h"
#include "fNETLib.h"

class fNETController {
public:
    class fNETSlaveConnection {
    public:
        String MAC_Address = "00:00:00:00:00:00";
        uint8_t i2c_address = -1;
        int i2c_port = -1;

        bool isOnline = false;
        bool awaitingConfiguration = true;

        fNETSlaveConnection(uint8_t address, int port) : Config(1024) {
            i2c_address = address;
            i2c_port = port;
            RequestConfig();
        }

        fNETSlaveConnection(String mac, uint8_t address, int port) : Config(1024) {
            MAC_Address = mac;
            i2c_address = address;
            i2c_port = port;
            RequestConfig();
        }

        fNETSlaveConnection(String mac_address) : Config(1024) {
            i2c_address = -1;
            MAC_Address = mac_address;

            isOnline = false;
            RequestConfig();
        }

        fNETSlaveConnection() : Config(1024) {
            isOnline = false;
        }

        void RequestConfig() {
            //Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Request config");
            //QueueMessage("JSON{\"command\":\"getConfig\"}");

            DynamicJsonDocument q(128);

            q["source"] = "controller";
            q["command"] = "getConfig";

            String str;
            serializeJson(q, str);

            QueueStr("JSON" + str);
        }

        void Poll(TwoWire* wire) {
            if (i2c_address == -1)
                return;

            if (isOnline) {
                ReadPendingMessage(wire);
                SendPendingMessage(wire);
            }

            if (millis() - lastCheckedMillis > 1000)
                CheckConnection(wire);
        }

        void QueueStr(String msg) {
            //Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Queue: " + msg);
            SendBuffer.push(msg);
        }

        void SendMessage(DynamicJsonDocument data) {
            DynamicJsonDocument d(data);

            if (!d.containsKey("source"))
                d["source"] = "controller";

            if (!d.containsKey("recipient"))
                d["recipient"] = MAC_Address;

            Connection->Send(d);
        }

        void SetI2CAddr(uint8_t addr) {
            QueueStr("JSON{\"command\":\"setI2CAddr\", \"addr\":" + String(addr) + "\"}");
        }

        String GetMacAddress(TwoWire* wire) {
            return Transaction(wire, "GETMAC").substring(0, 17);
        }

        DynamicJsonDocument Config;
    private:
        CircularBuffer<String, 100> SendBuffer;

        int trNum = 0;

        String Transaction(TwoWire* wire, String send, int* error = nullptr) {
            String read;
            int receivedTrNum = -2;

            trNum++;

            //Serial.println("start transaction " + String(i2c_address));
            //Serial.println("send: " + send);
            //Serial.println("trnum: " + String(trNum));

            int errcount = 0;

            while (receivedTrNum < trNum) {
                //Serial.println("begin");
                //Serial.println("transaction " + String((int)i2c_address));
                wire->beginTransmission(i2c_address);

                int err = wire->endTransmission();
                //Serial.println("result:" + String(err));


                if (!err) {
                    wire->beginTransmission(i2c_address);
                    wire->print(PaddedInt(trNum, 8) + PaddedInt(send.length(), 4) + send);

                    err = wire->endTransmission();
                }

                if (err) {

                    if (isOnline) {
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Transaction error ( transmission error )");
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Error: " + String(err));
                    }

                    if (errcount > 3) {
                        if (isOnline)
                            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Transmission failed!");

                        if (error != nullptr)
                            *error = -1;

                        return "";
                    }

                    errcount++;
                    delay(100);
                    continue;
                }

                delay(10);

                err = wire->requestFrom((int)i2c_address, 128);
                //Serial.println("result:" + String(err));

                read.clear();

                for (int i = 0; i < 128; i++)
                    read += (char)wire->read();

                //Serial.println("result:" + read);

                receivedTrNum = ParsePaddedInt(read.substring(0, 8));
                int receivedByteNum = ParsePaddedInt(read.substring(8, 12));

                if (receivedTrNum >= trNum)
                    break;

                Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Transaction error ( resend )");
                Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Received: " + read);

                if (errcount > 3) {
                    Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Transmission failed!");

                    if (error != nullptr)
                        *error = -2;

                    Disconnected();

                    return "";
                }
                errcount++;
                delay(25);
            }

            if (error != nullptr)
                *error = 0;

            return read.substring(12);
        }

        void SendPendingMessage(TwoWire* wire) {
            if (SendBuffer.size() == 0)
                return;

            String toSend = SendBuffer.first();

            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Sending: " + toSend);

            fNETMessage msg = fNETMessage(toSend, 0);
            //Serial.println("Converted to msg.");

            String data = PaddedInt(msg.packetCount, 4) + PaddedInt(msg.messageID, 8);

            int error = 0;

            String transactionSend = "CMDSNDDAT" + data;
            String transactionResult = Transaction(wire, transactionSend, &error);
            String cmd = transactionResult.substring(0, 3);

            while (error == 0 && transactionResult != "" && cmd == "PKT") {
                int pktId = ParsePaddedInt(transactionResult.substring(3, 7));
                //Serial.println("send packet: " + String(pktId));
                transactionSend = "CMDSNDPKT" + msg.packets[pktId].data;

                //Serial.println("sent: " + transactionSend);
                transactionResult = Transaction(wire, transactionSend, &error);
                cmd = transactionResult.substring(0, 3);

                //Serial.println("transaction result: " + transactionResult);

                if (cmd == "FIN")
                {
                    SendBuffer.shift();
                    break;
                }
            }

            //Serial.println("done.");

            toSend = "";
        }

        void ReadPendingMessage(TwoWire* wire) {
            long sms = millis();
            int error = 0;

            String msgData = Transaction(wire, "CMDMSG", &error);
            if (error || msgData == "NONE")
                return;

            int packetCount = ParsePaddedInt(msgData.substring(0, 4));
            int msg_ID = ParsePaddedInt(msgData.substring(4, 12));

            String msg_data = "";

            for (int i = 0; i < packetCount; i++) {
                String pkt_data = "";

                while (error == 0) {
                    String read = Transaction(wire, "CMDPKT" + String(i), &error);

                    if (read.length() != 116) {
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Message poll error ( packet transmission error )");
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Received: " + read);
                        return;
                    }

                    String msgIDData = read.substring(0, 8);
                    String packetIDData = read.substring(8, 12);
                    String packetData = read.substring(12, 76);


                    int pkt_msgID = ParsePaddedInt(msgIDData.c_str());
                    int pkt_ID = ParsePaddedInt(packetIDData.c_str());

                    pkt_data = packetData;

                    if (pkt_msgID != msg_ID) {
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Message poll error ( received msg id mismatch )");
                        Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Received: " + read);
                        return;
                    }

                    if (pkt_ID == i)
                        break;
                }

                for (int i = pkt_data.length() - 1; i > 0; i--) {
                    if (pkt_data[i] != '#')
                    {
                        pkt_data = pkt_data.substring(0, i + 1);
                        break;
                    }
                }

                if (pkt_data == "invalid") {
                    Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Message poll error ( packet data invalid )");
                    return;
                }

                msg_data += pkt_data;
            }

            if (msg_data != "") {
                OnReceiveMsg(msg_data);

                Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Message transport took: " + String(millis() - sms) + "ms.");
            }
        }

        void OnReceiveMsg(String msg) {
            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Received message data:\n" + msg);

            if (msg.substring(0, 4) == "JSON")
                OnReceiveJSON(msg.substring(4));
        }

        CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;

        void OnReceiveJSON(String msg) {
            DynamicJsonDocument d = DynamicJsonDocument(1024);

            deserializeJson(d, msg);

            if (d["tag"] == "config")
                OnReceiveConfig(d["data"]);
            else if (d["recipient"] != "controller")
                Connection->Send(d); // Forward message
            else
                Connection->OnReceiveMessageService(d);
        }

        void OnReceiveConfig(String d) {
            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Received config.");
            deserializeJson(Config, d);

            MAC_Address = Config["macAddress"].as<String>();

            awaitingConfiguration = false;
        }

        void Disconnected() {
            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Disconnected!");
            lastCheckedMillis = millis();
        }

        void Reconnected() {
            Serial.println("[fGMS fNET, Module: " + MAC_Address + "] Reconnected!");

            //awaitingConfiguration = true;
        }

        bool wasOnline = false;

        void CheckConnection(TwoWire* wire) {
            int err = 0;
            String ret = Transaction(wire, "PING", &err);

            if (!err && ret.substring(0, 4) == "PONG")
                isOnline = true;
            else
                isOnline = false;

            if (isOnline && !wasOnline)
                Reconnected();
            else if (!isOnline && wasOnline)
                Disconnected();

            wasOnline = isOnline;
            lastCheckedMillis = millis();
        }

        long lastCheckedMillis = 0;
    };

    static fNETConnection* Init() {
        Connection = new fNETConnection("controller", &msghandler);

        Serial.println("[fGMS] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        ReadConfig();

        SetupI2C();

        Serial.println("[fGMS] I2C OK.");

        Connection->AddQueryResponder("getAllCurrentModuleData", [](DynamicJsonDocument) {
            DynamicJsonDocument r(1024);
            SaveModules();

            JsonArray a = JsonArray(data["modules"]);
            JsonArray a2 = r.createNestedArray("modules");

            a2.set(a);
            return r;
            });

        Serial.println("[fGMS] Loading modules...");

        LoadModules();

        Serial.println("[fGMS] Done.");

        Connection->IsConnected = true;
        return Connection;
    }

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fGMS] Saving...");

        SaveModules();

        String data_serialized;
        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fGMS_SystemData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fGMS] Saved.");
    }

    static fNETSlaveConnection* GetModuleByMAC(String mac) {
        Serial.println("[fNET] Get module: " + mac);
        for (int i = 0; i < ModuleCount; i++) {
            fNETSlaveConnection* d = Modules[i];

            if (d->MAC_Address == mac)
                return d;
        }
        Serial.println("[fNET] Module not found");
        return nullptr;
    }

    static fNETSlaveConnection* Modules[32];
    static int ModuleCount;
    static fNETConnection* Connection;

private:
    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            err_LittleFSMountError();

        File data_file = LittleFS.open("/fGMS_SystemData.json", "r");

        if (!data_file)
            err_DataFileNotFound();

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fGMS Controller] Read config: " + data_rawJson);
    }

    static void err_LittleFSMountError() {
        Serial.println("[fGMS LittleFS] LittleFS Mount error!");
        Serial.println("[fGMS LittleFS] Attempt flash? (y/n)");

        while (!Serial.available()) {
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            delay(500);

            digitalWrite(fNET_PIN_INDICATOR_Y, LOW);
            delay(500);
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

    static void err_DataFileNotFound() {
        Serial.println("[fGMS] No data found!");
        Serial.println("[fGMS] Reset system config?");

        while (!Serial.available()) {
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            delay(500);

            digitalWrite(fNET_PIN_INDICATOR_Y, LOW);
            delay(500);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fGMS] Reconfiguration cancelled.");
            delay(500);
            ESP.restart();
        }

        data = DynamicJsonDocument(1024);
        Save();
    }

    static void SaveModules() {
        Serial.println("[fGMS Controller] Saving modules.");
        JsonArray modulesArray = data.createNestedArray("modules");

        for (int i = 0; i < ModuleCount; i++) {
            fNETSlaveConnection* mod = Modules[i];
            Serial.println("[fGMS Controller] Saving module " + String(mod->MAC_Address));

            JsonObject object = modulesArray.createNestedObject();

            object["id"] = mod->i2c_address;
            object["port"] = mod->i2c_port;
            object["macAddress"] = mod->MAC_Address;
            object["data"] = mod->Config.as<JsonObject>();
            object["isOnline"] = mod->isOnline;
        }
    }

    static void LoadModules() {
        JsonArray modulesArray = data["modules"];
        ModuleCount = 0;

        for (JsonObject o : modulesArray) {
            String mac = o["macAddress"];
            JsonObject config_o = o["data"];

            Serial.println("[fGMS Controller] Loaded module " + String(mac));

            fNETSlaveConnection* d = new fNETSlaveConnection(mac, -1, 0);
            d->Config = config_o;

            Modules[ModuleCount] = d;
            ModuleCount++;
        }
    }

    static TwoWire I2C1;
    static TwoWire I2C2;

    static void SetupI2C() {
        Serial.println("[fGMS fNET Controller] Beginnig I2Cs as master");

        I2C1.begin(fNET_SDA, fNET_SCK, (uint32_t)800000);
        //I2C2.begin(fGMS_SDA2, fGMS_SCK2, (uint32_t)800000);

        xTaskCreate(I2CTask, "fGMS_fNETTask", 10000, nullptr, 0, nullptr);
    }

    static long I2C_LastScanMs;

    static void I2CTask(void* param) {
        while (true) {
            I2C_PollModules();

            if (millis() - I2C_LastScanMs > 1000)
                I2C_ScanModules(&I2C1);

            delay(100);
        }
    }

    static void I2C_PollModules() {
        for (int i = 0; i < ModuleCount; i++)
            Modules[i]->Poll(&I2C1);
    }

    static void AddNewModule(String mac) {
        Serial.println("[fGMS Controller] Add new module: " + mac);

        fNETSlaveConnection* dat = new fNETSlaveConnection(mac);
        //dat->SetI2CAddr(I2C_GetEmptyAddr(0));

        Modules[ModuleCount] = dat;
        ModuleCount++;
        Save();
    }

    static uint8_t I2C_GetEmptyAddr(int port) {
        return ModuleCount + 16;
    }

    static bool I2C_IsDevicePresentAtAddress(uint8_t addr, TwoWire* wire) {
        //Serial.println("[fGMS fNET] Check address");
        wire->beginTransmission(addr);
        int err = wire->endTransmission();

        //Serial.println("[fGMS fNET] Present? :" + String(err));

        return err == 0;
    }

    static String GetMacAddress(uint8_t addr, TwoWire* wire) {
        //Serial.println("[fGMS fNET] Get MAC");
        if (!I2C_IsDevicePresentAtAddress(addr, wire))
            return "";

        //Serial.println("[fGMS fNET] Address present.");

        fNETSlaveConnection* m = new fNETSlaveConnection(addr, 0);

        String mac = m->GetMacAddress(wire);
        m->GetMacAddress(wire); //increment trnum

        delete m;

        return mac;
    }

    static int I2C_LocateModule(String mac, TwoWire* wire) {
        for (int i = 16; i < 32; i++) {
            String foundMac = GetMacAddress(i, wire);

            if (foundMac == mac)
                return i;
        }

        return -1;
    }

    static void I2C_ScanModules(TwoWire* wire) {
        //Serial.println("[fGMS fNET] Scanning for modules...");

        I2C_LastScanMs = millis();

        for (int i = 16; i < 32; i++) {
            //Serial.println("[fGMS fNET] Check " + String(i));
            String mac = GetMacAddress(i, wire);
            //Serial.println("[fGMS fNET] MAC: " + mac);

            if (mac == "")
                continue;

            bool found = false;
            for (int j = 0; j < ModuleCount; j++) {
                if (Modules[j]->MAC_Address == mac) {
                    if (!Modules[j]->isOnline)
                        Serial.println("[fGMS fNET] Found module " + mac + " at address: " + i);

                    Modules[j]->i2c_address = i;

                    found = true;
                    break;
                }
            }

            if (!found) {
                Serial.println("[fGMS fNET] Found NEW module " + mac + " at address: " + i);
                AddNewModule(mac);
            }
        }

        String mac = GetMacAddress(0x01, wire);
        //Serial.println(mac);

        if (mac != "") {
            fNETSlaveConnection* d = GetModuleByMAC(mac);

            if (d != nullptr) {
                Serial.println("[fGMS fNET] Found module " + mac + " at address 0x01");
                d->i2c_address = 0x01;
                d->SetI2CAddr(I2C_GetEmptyAddr(0));
            }
            else {
                Serial.println("[fGMS fNET] Found NEW module " + mac + " at address 0x01");
                AddNewModule(mac);
            }
        }
    }

    static void msghandler(String msg, String dest) {
        fNETSlaveConnection* m = GetModuleByMAC(dest);

        if (m != nullptr)
            m->QueueStr(msg);
    }
};
#endif