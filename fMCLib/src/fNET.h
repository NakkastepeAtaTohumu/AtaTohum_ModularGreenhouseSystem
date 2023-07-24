#pragma once

#ifndef fNET_h
#define fNET_h

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <CircularBuffer.h>
#include <esp_check.h>
#include <esp_now.h>
#include <WiFi.h>
#include "fNETStringFunctions.h"
#include "fEvents.h"

class fNETConnection;

class QueryResponseHandler_d {
public:
    virtual DynamicJsonDocument Handle(DynamicJsonDocument d) {
        Serial.println("Error");
        return DynamicJsonDocument(0);
    }
};

class DefaultQueryResponder {
public:
    DefaultQueryResponder(DynamicJsonDocument(*response)(DynamicJsonDocument)) {
        Response = response;
    }

    DynamicJsonDocument HandleQueryResponse(DynamicJsonDocument d) {
        return Response(d);
    }

    DynamicJsonDocument(*Response)(DynamicJsonDocument);
};

template<class T>
class QueryResponseHandler : public QueryResponseHandler_d {
public:
    QueryResponseHandler(T* t_) {
        t = t_;
    }


    DynamicJsonDocument Handle(DynamicJsonDocument d) override {
        return t->HandleQueryResponse(d);
    }

private:
    T* t;
};

class QueryResponder {
public:
    QueryResponseHandler_d* h;
    String queryType;


    QueryResponder(String query, QueryResponseHandler_d* hnd) {
        queryType = query;
        h = hnd;
    }

    DynamicJsonDocument Handle(DynamicJsonDocument d) {
        return h->Handle(d);
    }
};

class fNETConnection {
public:
    fNETConnection(String my_mac) {
        mac = my_mac;
    }

    void AddQueryResponder(String query, DynamicJsonDocument(*Response)(DynamicJsonDocument)) {
        DefaultQueryResponder* r = new DefaultQueryResponder(Response);
        QueryResponseHandler_d* hnd = new QueryResponseHandler<DefaultQueryResponder>(r);
        Responders[ResponderNum] = new QueryResponder(query, hnd);
        ResponderNum++;
    }

    void AddQueryResponder(String query, QueryResponseHandler_d* response) {
        Responders[ResponderNum] = new QueryResponder(query, response);
        ResponderNum++;
    }

    bool Query(String mac, String query) {
        return Query(mac, query, nullptr, nullptr);
    }

    bool Query(String mac, String query, DynamicJsonDocument* result) {
        return Query(mac, query, nullptr, result);
    }

    bool Query(String macToQuery, String query, DynamicJsonDocument* dt, DynamicJsonDocument* result) {
        DynamicJsonDocument* data = dt;

        if (dt == nullptr)
            data = new DynamicJsonDocument(256);

        DynamicJsonDocument q = *data;

        int sentQueryID = LastQueryID++;

        q["queryID"] = sentQueryID;
        q["tag"] = "query";
        q["recipient"] = macToQuery;
        q["query"] = query;

        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Querying " + q["recipient"].as<String>() + ": " + q["query"].as<String>());

        if (!Send(q)) {
            Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Send failed.");
            return false;
        }

        long startWaitms = millis();
        while (millis() - startWaitms < 5000) {
            delay(200);

            for (int i = 0; i < ReceivedJSONBuffer.size(); i++) {
                DynamicJsonDocument* r = ReceivedJSONBuffer[i];
                if ((*r)["tag"] == "queryResult" && (*r)["queryID"].as<int>() == sentQueryID && (*r)["recipient"].as<String>() == mac) {
                    Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Received query return.");

                    if (result != nullptr)
                        result->set((*r)["queryResult"]);


                    if (dt == nullptr) {
                        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Deleting query doc.");
                        delete data;
                    }

                    return true;
                }
            }
        }

        String d_str;

        serializeJson(q, d_str);
        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Query failed!");
        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Query is: " + d_str);


        if (dt == nullptr) {
            Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Deleting query doc.");
            delete data;
        }

        return false;
    }

    bool Send(DynamicJsonDocument data) {
        DynamicJsonDocument d(data);

        if (!d.containsKey("source"))
            d["source"] = mac;

        if (!d.containsKey("recipient"))
            return false;

        String str;
        serializeJson(d, str);

        //Serial.println("sending: " + str);

        return SendMessage(str, d["recipient"]);
    }

    void(*MessageReceived)(DynamicJsonDocument) = nullptr;

    bool IsConnected = false;

    Event OnMessageReceived;
    String mac = "";

protected:
    virtual bool SendMessage(String d, String r) {

    }

    void OnReceiveMessageService(DynamicJsonDocument d) {
        //if (d["auto"].as<bool>() != true) {
        String s;
        serializeJsonPretty(d, s);

        Serial.println("[fNET] Received message from " + d["source"].as<String>() + ": " + s);
        //}

        if (d["tag"] == "query")
            ProcessQuery(d);
        else if (d["tag"] == "queryResult") {
            ReceivedJSONBuffer.unshift(new DynamicJsonDocument(d));
            if (ReceivedJSONBuffer.isFull()) {
                delete ReceivedJSONBuffer.pop();
            }
        }


        if (MessageReceived != nullptr)
            MessageReceived(d);

        //Serial.println("[fNET] Message received!");
        //Serial.println("[fNET] " + d["source"].as<String>());
        //Serial.println("[fNET] " + d["source"].as<String>());


        

        delay(0);
        OnMessageReceived.Invoke(&d);
    }

private:
    void ProcessQuery(DynamicJsonDocument q) {
        DynamicJsonDocument r(1024);

        String q_str;
        serializeJson(q, q_str);

        //Serial.println("[fGMS fNET] Query: " + q_str);

        for (int i = 0; i < ResponderNum; i++) {
            if (Responders[i]->queryType == q["query"]) {
                r = Responders[i]->Handle(q);
                break;
            }
        }

        DynamicJsonDocument send(256);

        send["recipient"] = q["source"];
        send["tag"] = "queryResult";
        send["query"] = q["query"];
        send["queryID"] = q["queryID"];
        send["queryResult"] = r;
        send["auto"] = true;

        String s_str;
        serializeJson(send, s_str);

        //Serial.println("[fGMS fNET] Query response: " + s_str);
        Send(send);
    }

    void OnDisconnectService() {
        Serial.println("[fGMS fNet] Disconencted from controller!");
    }

    void OnReconnectService() {
        Serial.println("[fGMS fNet] Connected to controller!");
    }

    int LastQueryID = 0;

    QueryResponder* Responders[32];
    int ResponderNum = 0;

    CircularBuffer<DynamicJsonDocument*, 4> ReceivedJSONBuffer;
};

#warning ne

class fNETTunnel {
private:
    class TunnelManager {
    public:
        static void Add(fNETTunnel* t) {
            tunnels[tunnelNum++] = t;
        }

        static void Init() {
            xTaskCreate(task, "fNETTunnelTask", 4096, nullptr, 0, nullptr);

            Initialized = true;
        }

        static void task(void* param) {
            while (true) {
                delay(100);

                for (int i = 0; i < tunnelNum; i++)
                    tunnels[i]->Update();
            }
        }

        static bool Initialized;

    private:
        static fNETTunnel* tunnels[64];
        static int tunnelNum;
    };

public:
    fNETTunnel(fNETConnection* connection, String port_name) {
        portName = port_name;
        c = connection;
    }

    fNETTunnel(fNETConnection* connection, String remote_mac, String port_name) {
        portName = port_name;
        remoteMAC = remote_mac;
        c = connection;
    }

    void Init() {
        if (Initialized)
            return;

        Serial.println("[fNet Tunnel " + portName + "] Initializing.");

        if (!TunnelManager::Initialized)
            TunnelManager::Init();

        TunnelManager::Add(this);

        c->AddQueryResponder("fNETTunnelPort_" + portName, new QueryResponseHandler<fNETTunnel>(this));
        c->OnMessageReceived.AddHandler(new EventHandler<fNETTunnel>(this, [](fNETTunnel* t, void* args) { t->HandleMessage(args); }));

        Initialized = true;
    }

    void AcceptIncoming() {
        Serial.println("[fNet Tunnel " + portName + "] Accepting remote requests.");

        Accept = true;
    }

    void BlockIncoming() {
        Serial.println("[fNet Tunnel " + portName + "] Blocking remote requests.");

        Accept = false;
    }

    void Send(DynamicJsonDocument data) {
        if (!Initialized)
            return;

        if (!IsConnected)
            return;

        DynamicJsonDocument d = GetMessageFormat("data", 8192);
        d["data"] = data;

        c->Send(d);
    }

    void onReceive(void (*handler)(DynamicJsonDocument)) {
        receiveHandler = handler;
    }

    bool TryConnect(String remote_mac) {
        remoteMAC = remote_mac;
        return TryConnect();
    }

    void Close(String mac) {
        if (!Initialized)
            return;

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Closing connection.");

        DynamicJsonDocument d(256);

        d["port"] = portName;
        d["tag"] = "fNETTunnel";
        d["type"] = "disconnection_request";
        d["from"] = c->mac;
        d["query"] = "fNETTunnelPort_" + portName;

        d["recipient"] = mac;

        DynamicJsonDocument result(128);
        c->Query(mac, "fNETTunnelPort_" + portName, &d, &result);
    }

    DynamicJsonDocument HandleQueryResponse(DynamicJsonDocument d) {
        if (!(d["port"] == portName))
            return DynamicJsonDocument(0);

        if (d["type"] == "connection_request")
            return HandleConnect(d);

        if (d["type"] == "disconnect_request")
            return HandleDisconnect(d);

        if (d["sessionID"] != sessionID) {
            if (IsConnected)
                Close(d["source"]);

            Serial.println("Closed connection, invalid query: " + d["type"].as<String>());

            return DynamicJsonDocument(0);
        }

        return DynamicJsonDocument(0);
    }

    void HandleMessage(void* param) {
        DynamicJsonDocument d = *(DynamicJsonDocument*)param;

        if (d["tag"] != "fNETTunnel")
            return;

        if (d["port"] != portName)
            return;

        String d_s;
        serializeJson(d, d_s);
        Serial.println("received: " + d_s);

        if (d["sessionID"] != sessionID) {
            if (IsConnected) {
                if (d["type"] == "ping")
                    Close(d["source"]);
                else
                    LostConnection();
            }

            return;
        }

        if (d["source"] != remoteMAC)
            return;

        lastTransmissionMS = millis();

        if (d["type"] == "data") {
            //Serial.println("Received data!");
            DynamicJsonDocument* r = new DynamicJsonDocument(d["data"].as<JsonObject>());
            OnMessageReceived.Invoke(r);
            //Serial.println("invpke ok!");

            if (receiveHandler != nullptr)
                receiveHandler(*r);

            //Serial.println("handler ok!");

            delete r;
            //Serial.println("delete ok!");
        }
        else if (d["type"] == "ping") {
            Pong();
        }

        digitalWrite(2, LOW);
        delay(100);
        digitalWrite(2, HIGH);
    }

protected:
    bool TryConnect() {
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connecting to " + remoteMAC + "...");

        if (!Initialized) {
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Tunnel not initialized!");
            return false;
        }

        if (!Available) {
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Tunnel not available to connect.");
            return false;
        }

        lastConnectionAttemptMS = millis();

        GenerateSessionID();

        DynamicJsonDocument result(128);
        bool ok = false;
        for (int i = 0; i < 5; i++) {
            DynamicJsonDocument d = GetQueryFormat("connection_request");
            ok = c->Query(remoteMAC, "fNETTunnelPort_" + portName, &d, &result);

            if (ok || remoteMAC == "")
                break;

            delay(200);
        }

        if (!ok) {
            return false;
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection failed ( query error ).");
        }
        //Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Got query result.");

        if (result["status"] != "ok") {
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection failed ( invalid state ).");
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] State: " + result["status"].as<String>());
            return false;
        }

        IsConnected = true;
        Available = false;


        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connected!");
        OnConnect.Invoke(&remoteMAC);
        lastTransmissionMS = millis();

        return true;
    }

    void GenerateSessionID() {
        uint32_t random = esp_random();
        sessionID = "FNT-" + String((unsigned long)random, 16U);
    }

    DynamicJsonDocument HandleConnect(DynamicJsonDocument d) {
        Serial.println("[fNet Tunnel " + portName + "] Received connection request.");

        DynamicJsonDocument r(256);
        r["status"] = "refused";

        if (!Accept && d["from"] != remoteMAC)
            return r;

        if (!Available && d["from"] != remoteMAC)
            return r;

        Available = false;
        IsConnected = true;

        sessionID = d["sessionID"].as<String>();
        remoteMAC = d["from"].as<String>();

        OnConnect.Invoke(&remoteMAC);

        r["status"] = "ok";
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connected to " + remoteMAC + "!");

        lastTransmissionMS = millis();

        return r;
    }

    DynamicJsonDocument HandleDisconnect(DynamicJsonDocument d) {
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection closed by remote host.");

        DynamicJsonDocument r(64);

        r["status"] = "disconnected";

        String rm = remoteMAC;

        LostConnection();
        OnDisconnect.Invoke(&rm);

        return r;
    }

    void LostConnection() {
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Lost connection.");

        sessionID = "";
        remoteMAC = "";

        IsConnected = false;
        Available = true;

        OnDisconnect.Invoke();
    }

    void Update() {
        if (!IsConnected && (millis() - lastConnectionAttemptMS > 1000) && remoteMAC != "")
            TryConnect();

        if (!IsConnected)
            return;

        CheckConnection();
    }

    void Ping() {
        lastPingMS = millis();
        //Serial.println("Pinging");
        c->Send(GetMessageFormat("ping"));
    }
    void Pong() {
        c->Send(GetMessageFormat("pong"));
    }

    void CheckConnection() {
        if (!IsConnected)
            return;

        if (millis() - lastTransmissionMS > 2000 && millis() - lastPingMS > 1000)
            Ping();

        if (millis() - lastTransmissionMS > 10000)
            LostConnection();
    }


    DynamicJsonDocument GetMessageFormat(String type) {
        return GetMessageFormat(type, 256);
    }

    DynamicJsonDocument GetMessageFormat(String type, int bytes) {
        DynamicJsonDocument d(bytes);

        d["port"] = portName;
        d["tag"] = "fNETTunnel";
        d["type"] = type;
        d["sessionID"] = sessionID;
        d["from"] = c->mac;

        d["recipient"] = remoteMAC;

        return d;
    }

    DynamicJsonDocument GetQueryFormat(String type) {
        DynamicJsonDocument d(256);

        d["port"] = portName;
        d["sessionID"] = sessionID;
        d["from"] = c->mac;
        d["type"] = type;

        d["query"] = "fNETTunnelPort_" + portName;

        return d;
    }

public:
    bool IsConnected = false;
    bool Available = true;
    bool Accept = false;
    bool Initialized = false;

    Event OnConnect;
    Event OnDisconnect;
    Event OnMessageReceived;

protected:
    String portName;
    String remoteMAC;
    fNETConnection* c;

    String sessionID;

    void (*receiveHandler)(DynamicJsonDocument) = nullptr;

    long lastTransmissionMS = 0;
    long lastPingMS = 0;
    long lastConnectionAttemptMS = 0;
};


bool IsValidMACAddress(String mac);
uint8_t* ToMACAddress(String mac);
String ToMACString(const uint8_t* mac);

class fNET_ESPNOW {
public:
    class Connection_t : public fNETConnection {
    public:
        Connection_t() : fNETConnection(WiFi.macAddress()) {

        }

        bool SendMessage(String d, String r) override {
            return send_message(d, ToMACAddress(r));
        }

        void OnReceiveMessage(DynamicJsonDocument d) {
            fNETConnection::OnReceiveMessageService(d);
        }
    };

    static String buffer;

    static Connection_t* Init() {
        Serial.println("[ESP-NOW] WiFi channel: " + String(WiFi.channel()));

        if (esp_now_init() != ESP_OK)
            Serial.println("[ESP-NOW] Failed to initialize.");

        Connection = new Connection_t();

        esp_now_register_recv_cb(on_received_data);
        esp_now_register_send_cb(msg_callback);

        //xTaskCreate(event_task, "msg_receive_buf", 8192, NULL, 0, NULL);

        return Connection;
    }

    static void AddPeer(uint8_t* mac) {
        esp_now_peer_info peer_info = {};

        Serial.println("[ESP-NOW] Adding peer: " + ToMACString(mac));
        memcpy(peer_info.peer_addr, mac, 6);
        peer_info.channel = 1;

        if (esp_now_add_peer(&peer_info) != ESP_OK)
            Serial.println("[ESP-NOW] Failed to add peer.");
        else
            Serial.println("[ESP-NOW] Ok");
    }

private:
    static Connection_t* Connection;

    static bool send_message(String msg, uint8_t* dest) {
        if (!esp_now_is_peer_exist(dest))
            AddPeer(dest);

        while (msg.length() > 240) {
            if (send("C" + msg.substring(0, 240), dest))
                msg = msg.substring(240);
            else
                return false;
        }

        bool ok = send("F" + msg, dest);
        delete dest;
        return ok;
    }

    static bool send(String msg, uint8_t* dest) {
        if (esp_now_send(dest, (uint8_t*)msg.c_str(), msg.length()) != ESP_OK) {
            Serial.println("[ESP-NOW] Failed to send.");
            Serial.println("[ESP-NOW] Data was: " + msg);

            return false;
        }

        return true;
    }

    //static CircularBuffer<DynamicJsonDocument*, 32> event_queue;

    /*static void event_task(void* param) {
        while (true) {
            while (!event_queue.isEmpty())
            {
                DynamicJsonDocument* d = event_queue.shift();
                Connection->OnReceiveMessage(*d);
                delete d;
            }

            delay(50);
        }
    }*/

    static void on_received_data(const uint8_t* mac, const uint8_t* data, int length) {
        String received = String((const char*)data, length);
        bool send_to_buffer = false;
        //Serial.println("[ESP-NOW] Received: " + received);

        if (received[0] == 'C')
            send_to_buffer = true;
        else if (received[0] != 'F')
            return;


        String message = received.substring(1);
        //Serial.println("[ESP-NOW] Message: " + received);

        //Serial.println("[ESP-NOW] Complete data: " + complete);
        buffer += message;

        if (!send_to_buffer) {
            Serial.println("[ESP-NOW] Received message: " + buffer);
            DynamicJsonDocument d(1024);

            //Serial.println("[ESP-NOW] Buffer: " + buffer);
            if (deserializeJson(d, buffer) == DeserializationError::Ok)
                Connection->OnReceiveMessage(d);//event_queue.push(d);
            else {
                Serial.println("[ESP-NOW] Received malformed message: " + buffer);
                //delete d;
            }

            buffer = "";
        }
    }

    static void msg_callback(const uint8_t* mac, esp_now_send_status_t status) {
        if (status != ESP_NOW_SEND_SUCCESS)
            Serial.println("[ESP-NOW] Send failed, status: " + String(status));
        //else
        //    Serial.println("[ESP-NOW] Send OK.");
    }
};

#endif