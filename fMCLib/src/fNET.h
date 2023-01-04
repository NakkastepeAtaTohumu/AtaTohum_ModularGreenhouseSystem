#pragma once

#ifndef fNET_h
#define fNET_h

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <CircularBuffer.h>
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

    JsonObject* Query(String mac, String query) {
        return Query(mac, query, nullptr);
    }

    JsonObject* Query(String macToQuery, String query, DynamicJsonDocument* qd) {
        if (qd == nullptr)
            qd = new DynamicJsonDocument(256);

        DynamicJsonDocument q = DynamicJsonDocument(*qd);

        int sentQueryID = LastQueryID++;

        q["queryID"] = sentQueryID;
        q["tag"] = "query";
        q["recipient"] = macToQuery;
        q["query"] = query;

        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Querying " + q["recipient"].as<String>() + ": " + q["query"].as<String>());

        Send(q);

        long startWaitms = millis();
        while (millis() - startWaitms < 5000) {
            delay(250);

            for (int i = 0; i < ReceivedJSONBuffer.size(); i++) {
                DynamicJsonDocument* r = ReceivedJSONBuffer[i];
                if ((*r)["tag"] == "queryResult" && (*r)["queryID"].as<int>() == sentQueryID && (*r)["recipient"].as<String>() == mac) {
                    Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Received query return.");
                    return new JsonObject((*r)["queryResult"].as<JsonObject>());
                }
            }
        }

        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Query failed!");
        return nullptr;
    }

    void Send(DynamicJsonDocument data) {
        DynamicJsonDocument d(data);

        if (!d.containsKey("source"))
            d["source"] = mac;

        if (!d.containsKey("recipient"))
            d["recipient"] = "controller";

        String str;
        serializeJson(d, str);

        //Serial.println("sending: " + str);

        QueueMessage("JSON" + str, d["recipient"]);
    }

    void(*MessageReceived)(DynamicJsonDocument) = nullptr;

    bool IsConnected = false;

    virtual int GetQueuedMessageCount() {

    }

    CircularBuffer<DynamicJsonDocument*, 32> ReceivedJSONBuffer;
    Event OnMessageReceived;
    String mac = "";

protected:
    virtual void QueueMessage(String d, String r) {

    }

    void OnReceiveMessageService(DynamicJsonDocument d) {
        if (d["auto"].as<bool>() != true) {
            String s;
            serializeJsonPretty(d, s);

            //Serial.println("[fNET] Received message from " + d["source"].as<String>() + ": " + s);
        }

        if (d["tag"] == "query")
            ProcessQuery(d);
        else if (d["recipient"] != mac) {
            Serial.println("[fNET] Forwarding message from " + d["source"].as<String>() + " to " + d["recipient"].as<String>());
            Send(d); // Forward message
        }
        else if (MessageReceived != nullptr)
            MessageReceived(d);

        //Serial.println("Message received!");
        //Serial.println(d["source"].as<String>());

        ReceivedJSONBuffer.unshift(new DynamicJsonDocument(d));

        if (ReceivedJSONBuffer.isFull()) {
            delete ReceivedJSONBuffer.pop();
        }

        delay(0);
        OnMessageReceived.Invoke(&d);
    }

private:
    void ProcessQuery(DynamicJsonDocument q) {
        DynamicJsonDocument r(1024);

        String q_str;
        serializeJson(q, q_str);

        Serial.println("[fGMS fNET] Query: " + q_str);

        for (int i = 0; i < ResponderNum; i++) {
            if (Responders[i]->queryType == q["query"]) {
                r = Responders[i]->Handle(q);
                break;
            }
        }

        DynamicJsonDocument send(1280);

        send["recipient"] = q["source"];
        send["tag"] = "queryResult";
        send["query"] = q["query"];
        send["queryID"] = q["queryID"];
        send["queryResult"] = r;
        send["auto"] = true;

        String s_str;
        serializeJson(send, s_str);

        Serial.println("[fGMS fNET] Query response: " + s_str);
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

    bool TryConnect(String remote_MAC) {
        if (!Initialized)
            return false;

        GenerateSessionID();

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connecting to " + remote_MAC + "...");

        JsonObject* result = nullptr;
        for (int i = 0; i < 5; i++) {
            DynamicJsonDocument d = GetQueryFormat("connection_request");
           result = c->Query(remote_MAC, "fNETTunnelPort_" + portName, &d);

           if (result != nullptr)
               break;
        }

        if (result == nullptr) {
            return false;
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection failed ( query error ).");
        }

        JsonObject res = *result;

        if (res["status"] != "ok") {
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection failed ( invalid state ).");
            Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] State: " + res["status"].as<String>());
            return false;
        }

        IsConnected = true;
        Available = false;

        remoteMAC = remote_MAC;

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connected!");
        lastTransmissionMS = millis();

        return true;
    }

    void Disconnect() {
        if (!Initialized)
            return;

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Disconnecting.");

        DynamicJsonDocument d = GetQueryFormat("disconnection_request");

        JsonObject* result = c->Query(remoteMAC, "fNETTunnelPort_" + portName);
    }

    DynamicJsonDocument HandleQueryResponse(DynamicJsonDocument d) {
        if (!(d["port"] == portName))
            return DynamicJsonDocument(0);

        if (d["type"] == "connection_request")
            return HandleConnect(d);

        if (d["sessionID"] != sessionID) {
            if (IsConnected)
                LostConnection();

            return DynamicJsonDocument(0);
        }

        else if (d["type"] == "disconnect_request")
            return HandleDisconnect(d);

        return DynamicJsonDocument(0);
    }

    void HandleMessage(void* param) {
        DynamicJsonDocument d = *(DynamicJsonDocument*)param;
        //String d_s;
        //serializeJson(d, d_s);
        //Serial.println("received: " + d_s);

        if (d["tag"] != "fNETTunnel")
            return;

        if (d["source"] != remoteMAC)
            return;

        if (d["port"] != portName)
            return;

        if (d["sessionID"] != sessionID) {
            if (IsConnected)
                LostConnection();

            return;
        }

        lastTransmissionMS = millis();

        if (d["type"] == "data") {
            //Serial.println("Received data!");
            DynamicJsonDocument* r = new DynamicJsonDocument(d["data"].as<JsonObject>());
            OnMessageReceived.Invoke(r);
            //Serial.println("invpke ok!");

            if(receiveHandler != nullptr)
                receiveHandler(*r);

            //Serial.println("handler ok!");

            delete r;
            //Serial.println("delete ok!");
        }
        else if (d["type"] == "ping") {
            Pong();
        }
    }

protected:
    void GenerateSessionID() {
        uint32_t random = esp_random();
        sessionID = "FNT-" + String((unsigned long)random, 16U);
    }

    DynamicJsonDocument HandleConnect(DynamicJsonDocument d) {
        Serial.println("[fNet Tunnel " + portName + "] Received connection request.");

        DynamicJsonDocument r(64);
        r["status"] = "refused";

        if (!Accept)
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
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Disconnecting.");

        DynamicJsonDocument r(64);

        r["status"] = "disconnected";

        String rm = remoteMAC;

        LostConnection();
        OnDisconnect.Invoke(&rm);

        return r;
    }

    void LostConnection() {
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connection closed.");

        sessionID = "";
        remoteMAC = "";

        IsConnected = false;
        Available = true;

        OnDisconnect.Invoke();
    }

    void Update() {
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

        if (millis() - lastTransmissionMS > 60000)
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
};

#endif