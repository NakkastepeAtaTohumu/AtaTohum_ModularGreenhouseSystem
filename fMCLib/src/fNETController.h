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
    class ControllerConnection : public fNETConnection {
    public:
        ControllerConnection() : fNETConnection("controller") {

        }

        void QueueMessage(String d, String r) override {
            msghandler(d, r);
        }

        void OnReceiveMessage(DynamicJsonDocument d) {
            fNETConnection::OnReceiveMessageService(d);
        }

        int GetQueuedMessageCount() override {
            int cnt = 0;
            for (int i = 0; i < ModuleCount; i++) {
                cnt += Modules[i]->QueuedMessageCount();
            }

            return cnt;
        }
    };

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
            valid = false;
            isOnline = false;
        }

        void RequestConfig() {
            //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Request config");
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

            long start_ms = millis();
            if (isOnline) {
                ReadPendingMessage(wire);
                SendPendingMessage(wire);
            }

            if (millis() - lastCheckedMillis > 1000)
                CheckConnection(wire);

            UpdateAveragePollTime(millis() - start_ms);
        }

        void QueueStr(String msg) {
            //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Queue: " + msg);
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

            nextI2cAddr = addr;
        }

        String GetMacAddress(TwoWire* wire) {
            return Transaction(wire, "GETMAC").substring(0, 17);
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

        int QueuedMessageCount() {
            return SendBuffer.size();
        }

        DynamicJsonDocument Config;

        bool valid = true;

        float AverageTransactionTimeMillis = 0;
        float AveragePollTimeMillis = 0;

        int TotalTransactions = 0;
        int FailedTransactions = 0;
    private:
        CircularBuffer<String, 100> SendBuffer;

        int trNum = 0;

        String Transaction(TwoWire* wire, String send, int* error = nullptr) {
            long start_ms = millis();
            String read;
            int receivedTrNum = -2;

            trNum++;
            TotalTransactions++;
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
                        //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Transaction error ( transmission error )");
                        //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Error: " + String(err));
                    }

                    if (errcount > 3) {
                        if (isOnline)
                            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Transmission failed! ( transmission error )");

                        FailedTransactions++;
                        if (error != nullptr)
                            *error = -1;

                        return "";
                    }

                    errcount++;
                    delay(100);
                    continue;
                }

                delay(2);

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

                Serial.println("[fNET fNET, Module: " + MAC_Address + "] Transaction error ( resend )");
                Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received: " + read);

                if (errcount > 3) {
                    Serial.println("[fNET fNET, Module: " + MAC_Address + "] Transmission failed! ( resend )");

                    FailedTransactions++;

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

            long time = millis() - start_ms;
            float totalTime = 0;

            for (int i = 0; i < 31; i++) {
                transactionTimes[i] = transactionTimes[i + 1];
                totalTime += transactionTimes[i];
            }

            AverageTransactionTimeMillis = totalTime / 31;

            transactionTimes[31] = time;

            return read.substring(12);
        }

        void SendPendingMessage(TwoWire* wire) {
            if (SendBuffer.size() == 0)
                return;

            String toSend = SendBuffer.first();

            //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Sending: " + toSend);

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
                        Serial.println("[fNET fNET, Module: " + MAC_Address + "] Message poll error ( packet transmission error )");
                        Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received: " + read);
                        return;
                    }

                    String msgIDData = read.substring(0, 8);
                    String packetIDData = read.substring(8, 12);
                    String packetData = read.substring(12, 76);


                    int pkt_msgID = ParsePaddedInt(msgIDData.c_str());
                    int pkt_ID = ParsePaddedInt(packetIDData.c_str());

                    pkt_data = packetData;

                    if (pkt_msgID != msg_ID) {
                        Serial.println("[fNET fNET, Module: " + MAC_Address + "] Message poll error ( received msg id mismatch )");
                        Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received: " + read);
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
                    Serial.println("[fNET fNET, Module: " + MAC_Address + "] Message poll error ( packet data invalid )");
                    return;
                }

                msg_data += pkt_data;
            }

            if (msg_data != "") {
                OnReceiveMsg(msg_data);

                //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Message transport took: " + String(millis() - sms) + "ms.");
            }
        }

        void OnReceiveMsg(String msg) {
            //Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received message data:\n" + msg);

            if (msg.substring(0, 4) == "JSON")
                OnReceiveJSON(msg.substring(4));
        }

        CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;

        void OnReceiveJSON(String msg) {
            DynamicJsonDocument d = DynamicJsonDocument(1024);

            deserializeJson(d, msg);

            if (d["tag"] == "config")
                OnReceiveConfig(d["data"]);
            else
                Connection->OnReceiveMessage(d);
        }

        void OnReceiveConfig(String d) {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Received config.");

            String c_str;
            serializeJson(Config, c_str);

            if (c_str != d)
            {
                Serial.println("[fNET fNET, Module: " + MAC_Address + "] Save new config");
                deserializeJson(Config, d);

                MAC_Address = Config["macAddress"].as<String>();

                Save();
            }

            awaitingConfiguration = false;
        }

        void Disconnected() {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Disconnected!");
            lastCheckedMillis = millis();

            if (nextI2cAddr != -1) {
                i2c_address = nextI2cAddr;
                nextI2cAddr = -1;
            }
        }

        void Reconnected() {
            Serial.println("[fNET fNET, Module: " + MAC_Address + "] Reconnected!");

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

        int nextI2cAddr = -1;

        long transactionTimes[32];
        long pollTimes[32];

        void UpdateAveragePollTime(long time) {
            float totalTime = 0;

            for (int i = 0; i < 31; i++) {
                pollTimes[i] = pollTimes[i + 1];
                totalTime += pollTimes[i];
            }

            AveragePollTimeMillis = totalTime / 31;

            pollTimes[31] = time;
        }
    };

    static fNETConnection* Init() {
        status_d = "init";
        Connection = new ControllerConnection();

        Serial.println("[fNET] Build date/time: " + String(__DATE__) + " / " + String(__TIME__));

        status_d = "read_cfg";
        ReadConfig();
        Serial.println("[fNET] Loading modules...");

        status_d = "load_mdl";
        LoadModules();

        Serial.println("[fNET] Setup I2C...");
        status_d = "setup_i2c";
        SetupI2C();

        Connection->AddQueryResponder("getAllCurrentModuleData", [](DynamicJsonDocument) {
            DynamicJsonDocument r(1024);
            SaveModules();

            JsonArray a = JsonArray(data["modules"]);
            JsonArray a2 = r.createNestedArray("modules");

            a2.set(a);
            return r;
            });


        Serial.println("[fNET] Done.");

        Connection->IsConnected = true;
        status_d = "ok";
        return Connection;
    }

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fNET] Saving...");

        SaveModules();

        String data_serialized;
        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fNET_SystemData.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fNET] Saved.");
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

    static int GetModuleIndex(fNETSlaveConnection* c) {
        for (int i = 0; i < ModuleCount; i++) {
            if (Modules[i] == c) {
                return i;
            }
        }

        return -1;
    }

    static void SetI2CState(bool enabled, float freq) {
        data["isI2CEnabled"] = enabled;
        data["I2CFrequency"] = freq;

        Save();
        ESP.restart();
    }

    static float I2C_freq;
    static bool I2C_IsEnabled;

    static fNETSlaveConnection* Modules[32];
    static int ModuleCount;
    static ControllerConnection* Connection;

    static String status_d;

private:
    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            err_LittleFSMountError();

        File data_file = LittleFS.open("/fNET_SystemData.json", "r");

        if (!data_file)
            err_DataFileNotFound();

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fNET Controller] Read config: " + data_rawJson);
    }

    static void err_LittleFSMountError() {
        status_d = "mount_fail";

        Serial.println("[fNET LittleFS] LittleFS Mount error!");
        Serial.println("[fNET LittleFS] Attempt flash? (y/n)");

        while (!Serial.available()) {
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            delay(500);

            digitalWrite(fNET_PIN_INDICATOR_Y, LOW);
            delay(500);
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

    static void err_DataFileNotFound() {
        status_d = "cfg_err";

        Serial.println("[fNET] No data found!");
        Serial.println("[fNET] Reset system config?");

        while (!Serial.available()) {
            digitalWrite(fNET_PIN_INDICATOR_Y, HIGH);
            delay(500);

            digitalWrite(fNET_PIN_INDICATOR_Y, LOW);
            delay(500);
        }

        if (Serial.readStringUntil('\n') != "y") {
            Serial.println("[fNET] Reconfiguration cancelled.");
            delay(500);
            ESP.restart();
        }

        data = DynamicJsonDocument(1024);
        Save();
    }

    static void SaveModules() {
        Serial.println("[fNET Controller] Saving modules.");
        JsonArray modulesArray = data.createNestedArray("modules");

        for (int i = 0; i < ModuleCount; i++) {
            fNETSlaveConnection* mod = Modules[i];
            Serial.println("[fNET Controller] Saving module " + String(mod->MAC_Address));

            JsonObject object = modulesArray.createNestedObject();

            object["addr"] = mod->i2c_address;
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

            Serial.println("[fNET Controller] Loaded module " + String(mac));

            fNETSlaveConnection* d = new fNETSlaveConnection(mac, -1, 0);
            d->Config = config_o;

            Modules[ModuleCount] = d;
            ModuleCount++;
        }
    }
    

    static TwoWire I2C1;
    static TwoWire I2C2;

    static void SetupI2C() {
        Serial.println("[fNET fNET Controller] Beginnig I2Cs as master");

        I2C_IsEnabled = data["isI2CEnabled"].as<bool>();
        I2C_freq = data["I2CFrequency"].as<int>();

        if (I2C_IsEnabled) {
            I2C1.begin(fNET_SDA, fNET_SCK, (uint32_t)I2C_freq);
            I2C_ScanModules(&I2C1);
            //I2C2.begin(fNET_SDA2, fNET_SCK2, (uint32_t)800000);

            xTaskCreate(I2CTask, "fNET_fNETTask", 10000, nullptr, 0, nullptr);
        }
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
        Serial.println("[fNET Controller] Add new module: " + mac);

        fNETSlaveConnection* dat = new fNETSlaveConnection(mac);
        //dat->SetI2CAddr(I2C_GetEmptyAddr(0));

        Modules[ModuleCount] = dat;
        ModuleCount++;
        Save();
    }

    static uint8_t I2C_GetEmptyAddr(int port) {
        for (int addr = 16; addr < 127; addr++) {
            I2C1.beginTransmission(addr);
            int err = I2C1.endTransmission();

            if (err) {
                bool ok = true;
                for (int i = 0; i < ModuleCount; i++)
                    if (Modules[i]->i2c_address == addr) {
                        ok = false;
                        break;
                    }

                if(ok)
                    return addr;
            }
        }

        return 128;
        //return ModuleCount + 16;
    }

    static bool I2C_IsDevicePresentAtAddress(uint8_t addr, TwoWire* wire) {
        //Serial.println("[fNET fNET] Check address");
        wire->beginTransmission(addr);
        int err = wire->endTransmission();

        //Serial.println("[fNET fNET] Present? :" + String(err));

        return err == 0;
    }

    static String GetMacAddress(uint8_t addr, TwoWire* wire) {
        //Serial.println("[fNET fNET] Get MAC");
        if (!I2C_IsDevicePresentAtAddress(addr, wire))
            return "";

        //Serial.println("[fNET fNET] Address present.");

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
        //Serial.println("[fNET fNET] Scanning for modules...");

        I2C_LastScanMs = millis();

        for (int i = 16; i < 32; i++) {
            //Serial.println("[fNET fNET] Check " + String(i));
            String mac = GetMacAddress(i, wire);
            //Serial.println("[fNET fNET] MAC: " + mac);

            if (mac == "")
                continue;

            bool found = false;
            for (int j = 0; j < ModuleCount; j++) {
                if (Modules[j]->MAC_Address == mac) {
                    if (!Modules[j]->isOnline)
                        Serial.println("[fNET fNET] Found module " + mac + " at address: " + i);

                    Modules[j]->i2c_address = i;

                    found = true;
                    break;
                }
            }

            if (!found) {
                Serial.println("[fNET fNET] Found NEW module " + mac + " at address: " + i);
                AddNewModule(mac);
            }
        }

        String mac = GetMacAddress(0x01, wire);
        //Serial.println(mac);

        if (mac != "") {
            fNETSlaveConnection* d = GetModuleByMAC(mac);

            if (d != nullptr) {
                Serial.println("[fNET fNET] Found module " + mac + " at address 0x01");
                d->i2c_address = 0x01;
                d->SetI2CAddr(I2C_GetEmptyAddr(0));
            }
            else {
                Serial.println("[fNET fNET] Found NEW module " + mac + " at address 0x01");
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