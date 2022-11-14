#pragma once

#ifndef fNET_h
#define fNET_h

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

class fNETQueryResponder {
public:
    DynamicJsonDocument(*Response)(DynamicJsonDocument);
    String queryType;


    fNETQueryResponder(String query, DynamicJsonDocument(*response)(DynamicJsonDocument)) {
        queryType = query;
        Response = response;
    }
};

class fNETConnection {
public:
    void AddQueryResponder(String query, DynamicJsonDocument(*Response)(DynamicJsonDocument)) {
        Responders[ResponderNum] = new fNETQueryResponder(query, Response);
        ResponderNum++;
    }

    virtual void SendMessage(JsonObject msg, String destination_mac) {

    }

    JsonObject* Query(DynamicJsonDocument q) {
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

private:

    virtual void QueueMessage(String msg) {

    }

    void SendToController(DynamicJsonDocument data) {
        DynamicJsonDocument d(data);


        if (!d.containsKey("source"))
            d["source"] = WiFi.macAddress();

        if (!d.containsKey("recipient"))
            d["recipient"] = "controller";

        String str;
        serializeJson(d, str);

        QueueMessage("JSON" + str);
    }

    void OnReceiveMessageService(DynamicJsonDocument d) {
        DynamicJsonDocument d(1024);

        if (d["tag"] == "query")
            ProcessQuery(d);

        ReceivedJSONBuffer.unshift(new DynamicJsonDocument(d));
    }

    void ProcessQuery(DynamicJsonDocument q) {
        DynamicJsonDocument r(4096);

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

    bool IsConnected;

    void OnDisconnectService() {
        Serial.println("[fGMS fNet] Disconencted from controller!");
    }

    void OnReconnectService() {
        Serial.println("[fGMS fNet] Connected to controller!");
    }

    int LastQueryID;

    CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;

    fNETQueryResponder* Responders[32];
    int ResponderNum;

    String mac;

};
#endif