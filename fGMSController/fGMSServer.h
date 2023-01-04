#pragma once

#ifndef fGMSModule_h
#define fGMSModule_h

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"

#include "fGMS.h"

#define fGMSServerPort 80

class fGMSServer {
public:
     static void Init() {
        server = new AsyncWebServer(fGMSServerPort);
        events = new AsyncEventSource("/events");

        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

        server->on("/getSystem", [](AsyncWebServerRequest *r) {
            Serial.println("[fGMS Server] Get config json");

            String data_str;
            DynamicJsonDocument data = GetConfigJSON();
            serializeJsonPretty(data, data_str);

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        server->on("/getValues", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get data json");

            String data_str;
            DynamicJsonDocument data = GetDataJSON();
            serializeJsonPretty(data, data_str);

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        events->onConnect([](AsyncEventSourceClient* client) {

            if (client->lastId())
                Serial.printf("[fGMS Server Events] Client reconnected. Last message ID that it got is: %u\n", client->lastId());
            else
                Serial.println("[fGMS Server Events] Client connected.");

            String data_str;
            DynamicJsonDocument data = GetConfigJSON();
            serializeJsonPretty(data, data_str);

            Serial.println("[fGMS Server Events] Sending: " + data_str);

            events->send(data_str.c_str(), "config");

            });

        server->onNotFound([](AsyncWebServerRequest* r) {
            r->send(404, "text/html", "Not found");
            });

        server->addHandler(events);

        server->begin();

        xTaskCreate(task, "fGMSServerTask", 8192, nullptr, 0, nullptr);
    }
private:
    static AsyncWebServer* server;
    static AsyncEventSource* events;

    static DynamicJsonDocument GetConfigJSON() {
        DynamicJsonDocument d(4096);

        JsonArray hygrometersArray = d.createNestedArray("hygrometers");

        for (int i = 0; i < fGMS::HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();
            JsonObject posObj = o.createNestedObject("pos");
            
            posObj["x"] = fGMS::Hygrometers[i]->x;
            posObj["y"] = fGMS::Hygrometers[i]->y;
        }

        JsonArray groupsArray = d.createNestedArray("groups");

        for (int i = 0; i < fGMS::HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();
            JsonArray idsArray = o.createNestedArray("ids");

            for (int j = 0; j < fGMS::HygrometerGroups[i]->numHygrometers; j++)
                idsArray.add(fGMS::HygrometerGroups[i]->hygrometers[j]->id);

            o["color"] = "#00ff00";
        }

        JsonArray devicesArray = d.createNestedArray("devices");

        for (int i = 0; i < fGMS::ValveModuleCount; i++) {
            fGMS::ValveModule* mdl = fGMS::ValveModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["name"] = mdl->mdl->Config["name"];
            o["type"] = "valveModule";
            o["mac"] = mdl->module_mac;
            o["ok"] = mdl->ok;

            JsonObject data = o.createNestedObject("data");

            data["state"] = mdl->State;
        }

        for (int i = 0; i < fGMS::SensorModuleCount; i++) {
            fGMS::SensorModule* mdl = fGMS::SensorModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["name"] = mdl->mdl->Config["name"];
            o["type"] = "sensorModule";
            o["mac"] = mdl->module_mac;
            o["ok"] = mdl->ok;

            JsonObject data = o.createNestedObject("data");

            data["CO2"] = mdl->ppm;
            data["temp"] = mdl->temp;
            data["humidity"] = mdl->humidity;
        }

        JsonObject size = d.createNestedObject("hygrometerGridSize");

        size["x"] = fGMS::greenhouse.x_size;
        size["y"] = fGMS::greenhouse.y_size;

        return d;
    }

    static DynamicJsonDocument GetDataJSON() {
        DynamicJsonDocument d(4096);

        JsonArray hygrometersArray = d.createNestedArray("hygrometer_values");

        for (int i = 0; i < fGMS::HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();

            o["id"] = i;
            float val = fGMS::Hygrometers[i]->GetValue();
            o["value"] =  val >= 0 ? val * 100 : -1;
        }

        JsonArray devicesArray = d.createNestedArray("devices");

        for (int i = 0; i < fGMS::ValveModuleCount; i++) {
            fGMS::ValveModule* mdl = fGMS::ValveModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["name"] = mdl->mdl->Config["name"];
            o["type"] = "valveModule";
            o["mac"] = mdl->module_mac;
            o["ok"] = mdl->ok;

            JsonObject data = o.createNestedObject("data");

            data["state"] = mdl->State;
        }

        for (int i = 0; i < fGMS::SensorModuleCount; i++) {
            fGMS::SensorModule* mdl = fGMS::SensorModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["name"] = mdl->mdl->Config["name"];
            o["type"] = "sensorModule";
            o["mac"] = mdl->module_mac;
            o["ok"] = mdl->ok;

            JsonObject data = o.createNestedObject("data");

            data["CO2"] = mdl->ppm;
            data["temp"] = mdl->temp;
            data["humidity"] = mdl->humidity;
        }

        /*
        * TEST DATA
        *
        JsonObject o = devicesArray.createNestedObject();

        o["name"] = "AIR A";
        o["type"] = "sensorModule";
        o["mac"] = "00:00:00:00:00:03";

        JsonObject data = o.createNestedObject("data");

        data["CO2"] = 500.0 + millis() / 1000.0;
        data["temp"] = 24.96 + millis() / 10000.0;
        data["humidity"] = 0.45 + + millis() / 100000.0;

        JsonObject o2 = devicesArray.createNestedObject();

        o2["name"] = "VALVE A";
        o2["type"] = "valveModule";
        o2["mac"] = "00:00:00:00:00:04";

        JsonObject data2 = o2.createNestedObject("data");

        data2["state"] = 12;*/

        return d;
    }

    static void task(void* param) {
        while (true) {
            delay(1000);
            
            String data_str;
            DynamicJsonDocument data = GetDataJSON();
            serializeJsonPretty(data, data_str);

            //Serial.println("[fGMS Server Events] Sending: " + data_str);

            events->send(data_str.c_str(), "update");
        }
    }
};

#endif