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
    fNETConnection(String my_mac, void(*send_message)(String, String)) {
        mac = my_mac;
        queue_msg = send_message;
    }

    void AddQueryResponder(String query, DynamicJsonDocument(*Response)(DynamicJsonDocument)) {
        Responders[ResponderNum] = new fNETQueryResponder(query, Response);
        ResponderNum++;
    }

    JsonObject* Query(DynamicJsonDocument q, String mac) {
        int sentQueryID = LastQueryID++;

        Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] Querying " + q["recipient"].as<String>() + ": " + q["query"].as<String>());

        q["queryID"] = sentQueryID;
        q["tag"] = "query";
        q["recipient"] = mac;

        Send(q);

        long startWaitms = millis();
        while (millis() - startWaitms < 5000) {
            delay(250);

            for (int i = 0; i < ReceivedJSONBuffer.size(); i++) {
                DynamicJsonDocument* r = ReceivedJSONBuffer[i];
                //Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] check returned data:" + String(i));
                //Serial.println("[fGMS fNet Query " + String(sentQueryID) + "] data:" + r["tag"]);

                if ((*r)["tag"] == "queryResult" && (*r)["queryID"] == sentQueryID && (*r)["recipient"] == mac) {
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

        queue_msg("JSON" + str, d["recipient"]);
    }

    void OnReceiveMessageService(DynamicJsonDocument d) {
        String s;
        serializeJsonPretty(d, s);

        Serial.println("[fNET] Received message from " + d["source"].as<String>() + ": " + s);

        if (d["tag"] == "query")
            ProcessQuery(d);
        else if (d["recipient"] != "controller") {
            Serial.println("[fNET] Forwarding message from " + d["source"].as<String>() + " to " + d["recipient"].as<String>());
            Send(d); // Forward message
        }
        else if (MessageReceived != nullptr)
            MessageReceived(d);

        ReceivedJSONBuffer.unshift(new DynamicJsonDocument(d));
    }

    void(*MessageReceived)(DynamicJsonDocument) = nullptr;

    bool IsConnected = false;
private:
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
        Send(send);
    }

    void OnDisconnectService() {
        Serial.println("[fGMS fNet] Disconencted from controller!");
    }

    void OnReconnectService() {
        Serial.println("[fGMS fNet] Connected to controller!");
    }

    int LastQueryID = 0;

    CircularBuffer<DynamicJsonDocument*, 8> ReceivedJSONBuffer;

    fNETQueryResponder* Responders[32];
    int ResponderNum = 0;

    String mac = "";

    void(*queue_msg)(String, String);
};
#endif