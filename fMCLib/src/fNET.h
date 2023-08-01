#pragma once

#ifndef fNET_h
#define fNET_h

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <CircularBuffer.h>
#include <esp_check.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>

#include "fNETStringFunctions.h"
#include "fEvents.h"

class fNETConnection;

class QueryResponseHandler_d {
public:
    virtual DynamicJsonDocument* Handle(DynamicJsonDocument* d) {
        Serial.println("Error");
        return nullptr;
    }
};

class DefaultQueryResponder {
public:
    DefaultQueryResponder(DynamicJsonDocument* (*response)(DynamicJsonDocument*)) {
        Response = response;
    }

    DynamicJsonDocument* HandleQueryResponse(DynamicJsonDocument* d) {
        return Response(d);
    }

    DynamicJsonDocument* (*Response)(DynamicJsonDocument*);
};

template<class T>
class QueryResponseHandler : public QueryResponseHandler_d {
public:
    QueryResponseHandler(T* t_) {
        t = t_;
    }


    DynamicJsonDocument* Handle(DynamicJsonDocument* d) override {
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

    DynamicJsonDocument* Handle(DynamicJsonDocument* d) {
        return h->Handle(d);
    }
};

class PingData {
public:
    PingData() {

    }

    String mac;
    int32_t ping;
};

class fNETConnection {
public:
    fNETConnection(String my_mac) {
        mac = my_mac;
    }

    void AddQueryResponder(String query, DynamicJsonDocument* (*Response)(DynamicJsonDocument*)) {
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

        ESP_LOGV("fNET", "Query %d: Querying %s: %s", sentQueryID, q["recipient"].as<String>().c_str(), q["query"].as<String>().c_str());

        if (!Send(q)) {
            ESP_LOGV("fNET", "Query %s: Send failed.", String(sentQueryID));
            return false;
        }

        long startWaitms = millis();
        while (millis() - startWaitms < 5000) {
            //TasksWaitingForMessage.push(xTaskGetCurrentTaskHandle());
            //vTaskSuspend(NULL);
            delay(200);
            for (int i = 0; i < ReceivedJSONBuffer.size(); i++) {
                DynamicJsonDocument& r = *ReceivedJSONBuffer[i];
                if (r["tag"] == "queryResult" && r["queryID"].as<int>() == sentQueryID && r["recipient"].as<String>() == mac) {
                    ESP_LOGV("fNET", "Query %s: Received query return.", String(sentQueryID).c_str());

                    if (result != nullptr)
                        result->set(r["queryResult"]);


                    if (dt == nullptr)
                        delete data;

                    return true;
                }
            }
        }

        String d_str;

        serializeJson(q, d_str);
        ESP_LOGE("fNET", "Query %d: Query failed.", sentQueryID);
        ESP_LOGE("fNET", "Query %d: Query was: %s", sentQueryID, d_str.c_str());

        if (dt == nullptr)
            delete data;

        return false;
    }

    bool Send(DynamicJsonDocument& d) {
        if (!d.containsKey("source"))
            d["source"] = mac;

        if (!d.containsKey("recipient"))
            return false;

        String str;
        serializeJson(d, str);

        //Serial.println("sending: " + str);

        return SendMessage(str, d["recipient"]);
    }

    virtual bool IsAddressValid(String address) {

    }

    virtual void Ping(String mac) {

    }

    void(*MessageReceived)(DynamicJsonDocument*) = nullptr;

    Event OnMessageReceived;
    Event OnPingReceived;

    String mac = "";

protected:
    virtual bool SendMessage(String d, String r) {

    }

    void OnReceiveMessageService(DynamicJsonDocument& d) {
        //if (d["auto"].as<bool>() != true) {
        //String s;
        //serializeJsonPretty(d, s);

        //ESP_LOGV("fNET", "Received message from %s : %s", d["source"].as<String>().c_str(), s.c_str());
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
            MessageReceived(&d);

        while (!TasksWaitingForMessage.isEmpty()) {
            vTaskResume(TasksWaitingForMessage.shift());
        }

        //Serial.println("[fNET] Message received!");
        //Serial.println("[fNET] " + d["source"].as<String>());
        //Serial.println("[fNET] " + d["source"].as<String>());




        delay(0);
        OnMessageReceived.Invoke(&d);
    }

private:
    void ProcessQuery(DynamicJsonDocument& q) {
        DynamicJsonDocument* r = nullptr;

        String q_str;
        serializeJson(q, q_str);

        //Serial.println("[fGMS fNET] Query: " + q_str);

        for (int i = 0; i < ResponderNum; i++) {
            if (Responders[i]->queryType == q["query"]) {
                r = Responders[i]->Handle(&q);
                break;
            }
        }

        if (r == nullptr)
            return;

        DynamicJsonDocument send(256);

        send["recipient"] = q["source"];
        send["tag"] = "queryResult";
        send["query"] = q["query"];
        send["queryID"] = q["queryID"];
        send["queryResult"] = *r;
        send["auto"] = true;

        String s_str;
        serializeJson(send, s_str);

        //Serial.println("[fGMS fNET] Query response: " + s_str);
        Send(send);

        if (r != nullptr)
            delete r;
    }

    int LastQueryID = 0;

    QueryResponder* Responders[32];
    int ResponderNum = 0;

    CircularBuffer<DynamicJsonDocument*, 4> ReceivedJSONBuffer;
    CircularBuffer<TaskHandle_t, 4> TasksWaitingForMessage;
};

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

        ESP_LOGD("fNET", "Tunnel %s: Initializing", portName.c_str());

        if (!TunnelManager::Initialized)
            TunnelManager::Init();

        TunnelManager::Add(this);

        c->AddQueryResponder("fNETTunnelPort_" + portName, new QueryResponseHandler<fNETTunnel>(this));
        c->OnMessageReceived.AddHandler(new EventHandler<fNETTunnel>(this, [](fNETTunnel* t, void* args) { t->HandleMessage(args); }));

        Initialized = true;
    }

    void AcceptIncoming() {
        ESP_LOGD("fNET", "Tunnel %s: Accepting remote requests.", portName.c_str());

        Accept = true;
    }

    void BlockIncoming() {
        ESP_LOGD("fNET", "Tunnel %s: Blocking remote requests.", portName.c_str());

        Accept = false;
    }

    bool Send(DynamicJsonDocument data) {
        if (!Initialized) {
            ESP_LOGE("fNET", "Tunnel %s : Tunnel not initialized!", portName.c_str());
            return false;
        }

        if (!IsConnected) {
            ESP_LOGE("fNET", "Tunnel %s : Tunnel not connected!", portName.c_str());
            return false;
        }
        ESP_LOGV("fNET", "Tunnel %s : Sending", portName.c_str());

        DynamicJsonDocument& d = *GetMessageFormat("data", 2048);
        d["data"] = data;

        if (d.overflowed())
            ESP_LOGE("fNET", "Tunnel %s : Data alloc failed!", portName.c_str());

        d.shrinkToFit();


        bool ok = c->Send(d);
        delete& d;

        if(!ok)
            ESP_LOGE("fNET", "Tunnel %s : Send failed!", portName.c_str());

        return ok;
    }

    void onReceive(void (*handler)(DynamicJsonDocument&)) {
        receiveHandler = handler;
    }

    bool TryConnect(String remote_mac) {
        remoteMAC = remote_mac;
        return TryConnect();
    }

    void Close(String mac) {
        if (!Initialized)
            return;

        ESP_LOGD("fNET", "Tunnel %s: Closing connection.", portName.c_str());

        DynamicJsonDocument& d = *GetMessageFormat("disconnect_request", 256);
        c->Send(d);
        delete& d;
    }

    DynamicJsonDocument* HandleQueryResponse(DynamicJsonDocument* dat) {
        DynamicJsonDocument& d = *dat;
        if (!(d["port"] == portName))
            return nullptr;

        if (d["type"] == "connection_request")
            return HandleConnect(dat);

        if (remoteMAC != "" && d["source"] != remoteMAC)
            return nullptr;

        if (d["sessionID"] != sessionID) {
            if (IsConnected)
                Close(d["source"]);

            ESP_LOGD("fNET", "Tunnel %s: Closing connection due to invalid query: %s", portName.c_str(), d["type"].as<String>().c_str());

            return nullptr;
        }

        return nullptr;
    }

    void HandleMessage(void* param) {
        DynamicJsonDocument d = *(DynamicJsonDocument*)param;

        if (d["tag"] != "fNETTunnel")
            return;

        if (d["port"] != portName)
            return;

        if (remoteMAC != "" && d["source"] != remoteMAC)
            return;

        //String d_s;
        //serializeJson(d, d_s);
        //ESP_LOGI("fNET", "Tunnel %s:%s : Received: %s", portName.c_str(), sessionID.c_str(), d_s.c_str());

        if (d["sessionID"] != sessionID) {
            if (IsConnected) {
                if (d["type"] == "ping")
                    Close(d["source"]);
                else if (d["type"] == "disconnect_request")
                    HandleDisconnect();
                else
                    LostConnection();
            }

            return;
        }

        if (d["source"] != remoteMAC)
            return;

        lastTransmissionMS = millis();

        if (d["type"] == "data") {
            DynamicJsonDocument r(d["data"].as<JsonObject>());
            OnMessageReceived.Invoke(&r);
            //Serial.println("invpke ok!");

            if (receiveHandler != nullptr)
                receiveHandler(r);

            //Serial.println("handler ok!");

            //Serial.println("delete ok!");
        }
        else if (d["type"] == "ping")
            Pong();

        //digitalWrite(2, LOW);
        //delay(100);
        //digitalWrite(2, HIGH);
    }

    void SetRemoteMAC(String rm) {
        remoteMAC = rm;
    }

protected:
    bool TryConnect() {
        ESP_LOGD("fNET", "Tunnel %s:%s : Connecting to: %s", portName.c_str(), sessionID.c_str(), remoteMAC.c_str());

        if (!Initialized) {
            ESP_LOGE("fNET", "Tunnel %s : Tunnel not initialized!", portName.c_str());
            return false;
        }

        if (!Available) {
            ESP_LOGE("fNET", "Tunnel %s : Tunnel not available to connect.", portName.c_str());
            return false;
        }

        lastConnectionAttemptMS = millis();

        GenerateSessionID();

        DynamicJsonDocument result(128);

        bool ok = false;
        DynamicJsonDocument& d = *GetQueryFormat("connection_request");
        ok = c->Query(remoteMAC, "fNETTunnelPort_" + portName, &d, &result);
        delete& d;

        if (!ok) {
            return false;
            ESP_LOGE("fNET", "Tunnel %s:%s : Connection failed (query error).", portName.c_str(), sessionID.c_str());
        }
        //Serial.println("[fNet Tunnel " + portName + ":" + sessionID + "] Got query result.");

        if (result["status"] != "ok") {
            ESP_LOGE("fNET", "Tunnel %s:%s : Connection failed (invalid state).", portName.c_str(), sessionID.c_str());
            ESP_LOGE("fNET", "Tunnel %s:%s : State: %s", portName, sessionID, result["status"].as<String>());
            return false;
        }

        IsConnected = true;
        Available = false;


        ESP_LOGD("fNET", "Tunnel %s:%s : Connected.", portName.c_str(), sessionID.c_str());
        OnConnect.Invoke(&remoteMAC);
        lastTransmissionMS = millis();

        return true;
    }

    void GenerateSessionID() {
        uint32_t random = esp_random();
        sessionID = "FNT-" + String((unsigned long)random, 16U);
    }

    DynamicJsonDocument* HandleConnect(DynamicJsonDocument* dat) {
        ESP_LOGD("fNET", "Tunnel %s : Received connection request.", portName.c_str());

        DynamicJsonDocument& d = *dat;
        DynamicJsonDocument* r = new DynamicJsonDocument(256);
        (*r)["status"] = "refused";

        if (!Accept && d["source"] != remoteMAC)
            return r;

        if (!Available && d["source"] != remoteMAC)
            return r;

        Available = false;
        IsConnected = true;

        sessionID = d["sessionID"].as<String>();
        remoteMAC = d["source"].as<String>();

        OnConnect.Invoke(&remoteMAC);

        (*r)["status"] = "ok";

        ESP_LOGD("fNET", "Tunnel %s:%s : Connected to %s.", portName.c_str(), sessionID.c_str(), remoteMAC.c_str());

        lastTransmissionMS = millis();

        return r;
    }

    DynamicJsonDocument& HandleDisconnect() {
        ESP_LOGD("fNET", "Tunnel %s:%s : Connection closed by remote host.", portName.c_str(), sessionID.c_str());

        String rm = remoteMAC;

        LostConnection();
        OnDisconnect.Invoke(&rm);
    }

    void LostConnection() {
        ESP_LOGD("fNET", "Tunnel %s:%s : Lost connection.", portName.c_str(), sessionID.c_str());

        sessionID = "";
        //remoteMAC = "";

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

        DynamicJsonDocument& d = *GetMessageFormat("ping");
        c->Send(d);
        delete& d;
    }
    void Pong() {
        DynamicJsonDocument& d = *GetMessageFormat("pong");
        c->Send(d);
        delete& d;
    }

    void CheckConnection() {
        if (!IsConnected)
            return;

        if (millis() - lastTransmissionMS > 2000 && millis() - lastPingMS > 1000)
            Ping();

        if (millis() - lastTransmissionMS > 10000)
            LostConnection();
    }


    DynamicJsonDocument* GetMessageFormat(String type) {
        return GetMessageFormat(type, 256);
    }

    DynamicJsonDocument* GetMessageFormat(String type, int bytes) {
        DynamicJsonDocument& d = *new DynamicJsonDocument(bytes);

        d["port"] = portName;
        d["tag"] = "fNETTunnel";
        d["type"] = type;
        d["sessionID"] = sessionID;
        d["source"] = c->mac;

        d["recipient"] = remoteMAC;

        return &d;
    }

    DynamicJsonDocument* GetQueryFormat(String type) {
        DynamicJsonDocument& d = *new DynamicJsonDocument(256);

        d["port"] = portName;
        d["sessionID"] = sessionID;
        d["source"] = c->mac;
        d["type"] = type;

        d["query"] = "fNETTunnelPort_" + portName;

        return &d;
    }

public:
    bool IsConnected = false;
    bool Available = true;
    bool Accept = false;
    bool Initialized = false;
    long lastTransmissionMS = 0;

    String remoteMAC;

    Event OnConnect;
    Event OnDisconnect;
    Event OnMessageReceived;

protected:
    String portName;
    fNETConnection* c;

    String sessionID;

    void (*receiveHandler)(DynamicJsonDocument&) = nullptr;

    long lastPingMS = 0;
    long lastConnectionAttemptMS = 0;
};


bool IsValidMACAddress(String mac);
uint8_t* ToMACAddress(String mac);
String ToMACString(const uint8_t* mac);

bool IsValidChipID(String ID);
uint32_t ToChipID(String ID);
String ToChipID(uint32_t ID);

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

        bool IsAddressValid(String address) override {
            return IsValidMACAddress(address);
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

class fNET_Mesh {
public:
    class Connection_t : public fNETConnection {
    public:
        Connection_t(String id) : fNETConnection(id) {

        }

        bool SendMessage(String d, String r) override {
            return send(d, ToChipID(r));
        }

        void OnReceiveMessage(DynamicJsonDocument& d) {
            fNETConnection::OnReceiveMessageService(d);
        }

        bool IsAddressValid(String address) override {
            return IsValidChipID(address);
        }

        void Ping(String id) override {
            ping(ToChipID(id));
        }
    };

    static painlessMesh* mesh;

    static Connection_t* Init(painlessMesh* Mesh) {
        ESP_LOGD("fNET", "Mesh: Initializing.");
        ESP_LOGD("fNET", "Mesh: I am: %s", ToChipID(Mesh->getNodeId()).c_str());

        mesh = Mesh;

        xTaskCreate(update_task, "mesh_update", 8192, NULL, 0, NULL);
        xTaskCreate(event_task, "mesh_msg_proc", 8192, NULL, 0, &event_task_handle);

        mesh->onReceive(&on_received_data);
        mesh->onNodeDelayReceived(&on_ping_return);
        //mesh.init(SSID, Password, Port);

        Connection = new Connection_t(ToChipID(mesh->getNodeId()));

        return Connection;

    }
private:
    static Connection_t* Connection;

    static bool send(String msg, uint32_t dest) {
        ESP_LOGV("fNET", "Mesh: Sending %s to %s.", msg.c_str(), String(dest, 16).c_str());

        bool result;

        if (dest == 0xFFFFFFFF)
            result = mesh->sendBroadcast(msg, false);
        else
            result = mesh->sendSingle(dest, msg);

        if (!result) {
            ESP_LOGV("fNET", "Mesh: Failed to send.");
            ESP_LOGV("fNET", "Mesh: Data was: %s", msg.c_str());

            return false;
        }

        return true;
    }

    static CircularBuffer<DynamicJsonDocument*, 32> event_queue;
    static TaskHandle_t event_task_handle;

    static void event_task(void* param) {
        while (true) {
            while (!event_queue.isEmpty())
            {
                DynamicJsonDocument* d = event_queue.shift();
                ESP_LOGV("fNET", "Mesh: Processing message: %s", (*d)["source"].as<String>().c_str());
                Connection->OnReceiveMessage(*d);
                delete d;
            }

            vTaskSuspend(NULL);
            delay(0);
        }
    }

    static void update_task(void* param) {
        long last_millis = millis();
        while (true) {
            mesh->update();
            delay(10);

            if (millis() - last_millis > 30)
                ESP_LOGD("fNET", "Mesh: Update took too long: %d ms.", millis() - last_millis);

            last_millis = millis();
        }
    }

    static void on_received_data(const uint32_t from, String received) {
        ESP_LOGV("fNET", "Mesh: Received message: %s", received.c_str());
        DynamicJsonDocument* d = new DynamicJsonDocument(2048);

        //Serial.println("[ESP-NOW] Buffer: " + buffer);
        if (deserializeJson(*d, received) == DeserializationError::Ok) {
            (*d).shrinkToFit();
            event_queue.push(d);
            vTaskResume(event_task_handle);
        }
        else {
            ESP_LOGE("fNET", "Mesh: Received malformed message: %s", received.c_str());
            //delete d;
        }
    }

    static void on_ping_return(uint32_t mac, int32_t ping) {
        PingData d = PingData();
        d.mac = ToChipID(mac);
        d.ping = ping;
        Connection->OnPingReceived.Invoke((void*)&d);
    }

    static void ping(uint32_t mac) {
        mesh->startDelayMeas(mac);
    }
};

#endif