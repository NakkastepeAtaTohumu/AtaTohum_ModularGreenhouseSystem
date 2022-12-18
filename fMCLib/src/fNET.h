#pragma once

#ifndef fNET_h
#define fNET_h

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
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

        DynamicJsonDocument q = *qd;

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

    CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;
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

        OnMessageReceived.Invoke(&d);
    }

private:
    void ProcessQuery(DynamicJsonDocument q) {
        DynamicJsonDocument r(1024);

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
public:
    fNETTunnel(fNETConnection* connection, String port_name) {
        portName = port_name;

        c = connection;

        c->AddQueryResponder("fNETTunnelPort_" + portName, new QueryResponseHandler<fNETTunnel>(this));

        c->OnMessageReceived.AddHandler(new EventHandler<fNETTunnel>(this, [](fNETTunnel* t, void* args) { t->HandleMessage(args); }));
    }

    void AcceptIncoming() {
        Accept = true;
    }

    void BlockIncoming() {
        Accept = false;
    }

    void Send(DynamicJsonDocument data) {
        if (!IsConnected)
            return;

        DynamicJsonDocument d(8192);

        d["port"] = portName;
        d["tag"] = "fNETTunnel";
        d["type"] = "data";

        d["recipient"] = remoteMAC;

        d["data"] = data;

        c->Send(d);
    }

    void onReceive(void (*handler)(DynamicJsonDocument)) {
        receiveHandler = handler;
    }

    bool TryConnect(String remote_MAC) {
        GenerateSessionID();

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connecting to " + remote_MAC + "...");

        DynamicJsonDocument d(128);

        d["portName"] = portName;
        d["sessionID"] = sessionID;
        d["from"] = c->mac;
        d["type"] = "connection_request";

        JsonObject* result = c->Query(remoteMAC, "fNETTunnelPort_" + portName);

        if (result == nullptr)
            return false;

        JsonObject res = *result;

        if (res["status"] != "ok")
            return false;
        
        IsConnected = true;
        Available = false;

        remoteMAC = remote_MAC;
        sessionID = res["sessionID"].as<String>();

        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connected!");

        return true;
    }

    void Disconnect() {
        DynamicJsonDocument d(128);

        d["portName"] = portName;
        d["sessionID"] = sessionID;
        d["from"] = c->mac;
        d["type"] = "disconnection_request";

        JsonObject* result = c->Query(remoteMAC, "fNETTunnelPort_" + portName);
    }

    DynamicJsonDocument HandleQueryResponse(DynamicJsonDocument d) {
        if (!(d["portName"] == portName))
            return DynamicJsonDocument(0);

        if (d["type"] == "connection_request")
            return HandleConnect(d);

        else if (d["sessionID"] != sessionID)
            return DynamicJsonDocument(0);

        else if (d["type"] == "disconnect_request")
            return HandleDisconnect(d);
    }

    void HandleMessage(void* param) {
        DynamicJsonDocument d = *(DynamicJsonDocument*)param;

        if (d["tag"] != "fNETTunnel")
            return;

        if (d["source"] != remoteMAC)
            return;

        if (d["port"] != portName || d["sessionID"] != sessionID)
            return;

        lastTransmissionMS = millis();

        if (d["type"] == "data") {
            DynamicJsonDocument* r = new DynamicJsonDocument(d["data"].as<JsonObject>());
            OnMessageReceived.Invoke(r);

            receiveHandler(*r);
            delete r;
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

        if (!(Available && Accept)) 
            return r;

        Available = false;
        IsConnected = true;

        sessionID = d["sessionID"].as<String>();
        remoteMAC = d["from"].as<String>();

        OnConnect.Invoke(&remoteMAC);

        r["status"] = "ok";
        Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Connected to " + remoteMAC + "!");

        return r;
    }

    DynamicJsonDocument HandleDisconnect(DynamicJsonDocument d) {
        DynamicJsonDocument r(64);

        r["status"] = "disconnected";

        String rm = remoteMAC;

        LostConnection();
        OnDisconnect.Invoke(&rm);

        return r;
    }

    void LostConnection() {
        sessionID = "";
        remoteMAC = "";

        IsConnected = false;
        Available = true;

        OnDisconnect.Invoke();
    }

    void task(void* param) {
        while (true) {
            delay(100);

            if (!IsConnected)
                continue;

            CheckConnection();
        }
    }

    void Ping() {
        DynamicJsonDocument d(256);

        d["port"] = portName;
        d["tag"] = "fNETTunnel";
        d["type"] = "ping";

        c->Send(d);
    }

    void CheckConnection() {
        if (!IsConnected)
            return;

        if (millis() - lastTransmissionMS > 500)
            Ping();

        if (millis() - lastTransmissionMS > 2000)
            LostConnection();
    }

public:
    bool IsConnected = false;
    bool Available = true;
    bool Accept = false;

    Event OnConnect;
    Event OnDisconnect;
    Event OnMessageReceived;

protected:
    String portName;
    String remoteMAC;
    fNETConnection* c;

    String sessionID;

    void (*receiveHandler)(DynamicJsonDocument);

    long lastTransmissionMS = 0;
};

#endif