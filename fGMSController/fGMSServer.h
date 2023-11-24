#pragma once

#ifndef fGMSServer_h
#define fGMSServer_h

#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "mbedtls/base64.h"

#include <FS.h>
#include <LittleFS.h>
#include <SimpleFTPServer.h>
#include <Update.h>
#include <RemoteUDPLogging.h>

#include "fGMS.h"
#include "fGUI.h"
#include <PNGenc.h>

#define fGMSServerPort 80

int GetPNGImage(uint8_t* buffer);
void GetBinaryImage(uint8_t* buffer, size_t length, size_t offset);

class fGMSServer {
public:
    static void Init() {
        server = new AsyncWebServer(fGMSServerPort);
        events = new AsyncEventSource("/events");

        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

        server->on("/setGUI", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Enable GUI");
            if (!r->hasParam("enabled"))
            {
                r->send(400);
                return;
            }

            fGMS::serverEnabled = atoi(r->getParam("enabled")->value().c_str());
            fGMS::Save();

            r->send(200, "text/html", "ok");
            });

        server->on("/setValve", [](AsyncWebServerRequest* r) {
            if (!r->hasParam("module") || !r->hasParam("valve"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int mdl = atoi(r->getParam("module")->value().c_str());
            int vlv = atoi(r->getParam("valve")->value().c_str());
            bool open = atoi(r->getParam("state")->value().c_str());

            Serial.println("[fGMS Server] Toggling valve: " + String(mdl) + " : " + String(vlv));

            if (mdl >= fGMS::ValveModuleCount)
            {
                r->send(418, "text/html", "Module not found");
                return;
            }

            fGMS::ValveModule* m = fGMS::ValveModules[mdl];

            if (!m->ok)
            {
                r->send(500, "text/html", "Module error");
                return;
            }

            bool result = m->QueueState(vlv, open);

            if (result)
                r->send(200, "text/html", "ok");
            else
                r->send(400, "text/html", "no_change");
            });

        server->on("/restartModule", [](AsyncWebServerRequest* r) {
            if (!r->hasParam("module"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int mdl = atoi(r->getParam("module")->value().c_str());
            Serial.println("[fGMS Server] Restarting module: " + String(mdl));

            if (mdl >= fNETController::ModuleCount)
            {
                r->send(418, "text/html", "Module not found");
                return;
            }

            fNETController::Module* m = fNETController::Modules[mdl];

            if (!m->isOnline)
            {
                r->send(500, "text/html", "Module not online");
                return;
            }

            m->Restart();
            r->send(200, "text/html", "ok");
            });

        server->on("/values", HTTP_GET, [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get data json");

            String data_str;
            DynamicJsonDocument& data = GetDataJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        server->on("/getGMSConfig", HTTP_GET, [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get config json");

            String data_str;
            DynamicJsonDocument& data = GetConfigJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        server->on("/getGMSData", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get data json");

            String data_str;
            DynamicJsonDocument& data = GetDataJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        server->on("/getfNETState", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get state json");

            String data_str;
            DynamicJsonDocument& data = GetSystemJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);
            });

        server->on("/getfNETConfig", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Get fNET config json");

            String data_str;
            DynamicJsonDocument& data = *fNETController::GetDataJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            Serial.println("[fGMS Server] Sending: " + data_str);
            r->send(200, "application/json", data_str);

            delete& data;
            });

        server->on("/savefNET", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Save fNET");

            fNETController::Save();

            r->send(200, "text/html", "ok");
            });

        server->on("/savefGMS", [](AsyncWebServerRequest* r) {
            Serial.println("[fGMS Server] Save fGMS");

            fGMS::Save();

            r->send(200, "text/html", "ok");
            });

        server->on("/restart", [](AsyncWebServerRequest* r) {
            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Restarting.");

            restartMS = millis() + 2000;

            r->send(200, "text/html", "ok");
            });

        events->onConnect([](AsyncEventSourceClient* client) {
            if (client->lastId())
                RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Client reconnected. Last message ID that it got is: %u", client->lastId());
            else
                RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Client connected.");

            RefreshConfig();
            });

        server->on("/getScreen", [](AsyncWebServerRequest* r) {
            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Sending image.");

            if (fGUI::SpriteInitialized) {
                r->send_P(500, "text/html", "Sprite not allocated");
                return;
            }

            uint8_t* image = (uint8_t*)malloc(8192);
            int length = GetPNGImage(image);

            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Got image, length: %d", length);
            r->send_P(200, "image/png", image, length);

            free(image);
            });

        server->on("/getScreenBinary", [](AsyncWebServerRequest* r) {
            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Sending screen image in binary.");

            //uint8_t* image = (uint8_t*)malloc(128 * 40);
            //GetBinaryImage(image);

            //r->send_P(200, "application/octet-stream", image, 128 * 40);

            int data_length = fGUI::sp->width() * fGUI::sp->height() / 4;
            bytes_sent = 0;
            AsyncWebServerResponse* response = r->beginChunkedResponse("application/octet-stream", [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t
                {
                    size_t length = fGUI::sp->width() * fGUI::sp->height() / 4;
                    size_t bytes_left = length - bytes_sent;
                    size_t buf_length = min(min(maxLen / 2 - 1, (size_t)256), bytes_left);
                    uint8_t* image = (uint8_t*)malloc(buf_length);

                    ESP_LOGV("fGMS Server", "Getting image, image length: %d, bytes left: %d, bytes to send: %d.", length, bytes_left, buf_length);

                    GetBinaryImage(image, buf_length, bytes_sent);
                    ESP_LOGV("fGMS Server", "Got image.");

                    bytes_sent += buf_length;
                    for (int i = 0; i < buf_length; i++) {
                        int upper = image[i] & 0b11110000;
                        int lower = image[i] & 0b00001111;

                        itoa(upper, (char*)(buffer + i * 2), 16);
                        itoa(lower, (char*)(buffer + i * 2 + 1), 16);
                    }

                    ESP_LOGV("fGMS Server", "Sending: %s", (char*)(buffer));

                    free(image);
                    return buf_length * 2;
                });

            r->send(response);
            });


        server->on("/button", [](AsyncWebServerRequest* r) {
            if (!r->hasParam("button") && !r->hasParam("encoder"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            if (r->hasParam("encoder"))
            {
                if (r->getParam("encoder")->value() == "next")
                    fGUI::OnEncoderTickForward();
                else if (r->getParam("encoder")->value() == "prev")
                    fGUI::OnEncoderTickBackward();
                else if (r->getParam("encoder")->value() == "click")
                    fGUI::OnButtonClicked(-1);
            }

            if (r->hasParam("button"))
            {
                int button = atoi(r->getParam("button")->value().c_str());
                if (button != 0)
                    fGUI::OnButtonClicked(button);
                else
                    fGUI::OnBackButtonClicked();
            }

            r->send(200, "text/html", "ok");
            });

        server->on("/setGreenhouseSize", [](AsyncWebServerRequest* r) {
            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Setting greenhouse size");
            if (!r->hasParam("x") || !r->hasParam("y"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            fGMS::greenhouse.x_size = atof(r->getParam("x")->value().c_str());
            fGMS::greenhouse.y_size = atof(r->getParam("y")->value().c_str());

            r->send(200, "text/html", "ok");
            });

        server->on(
            "/hygrometer",
            HTTP_POST,
            [](AsyncWebServerRequest* request) {},
            NULL,
            [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index, size_t total) {
                if (index != 0)
                {
                    RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Received chunked upload!");
                    r->send(200, "text/html", "failed");
                    return;
                }

                String data_string((const char*)data, len);
                DynamicJsonDocument hygrometer_doc(256);
                deserializeJson(hygrometer_doc, data_string);

                int hygrometer = r->hasParam("id") ? atoi(r->getParam("id")->value().c_str()) : -1;

                fGMS::Hygrometer* h;
                if (hygrometer == -1)
                    h = fGMS::CreateHygrometer();
                else
                    h = fGMS::GetHygrometer(hygrometer);

                if (h == nullptr) {
                    r->send(400, "text/html", "Hygrometer not found");
                    return;
                }

                fGMS::HygrometerModule* m = fGMS::GetHygrometerModuleByMAC(hygrometer_doc["module"].as<String>());

                if (m == nullptr) {
                    r->send(400, "text/html", "Module not found");
                    return;
                }

                h->Load(hygrometer_doc.as<JsonObject>());

                RefreshConfig();
                r->send(200, "text/html", "ok");
            });

        server->on("/hygrometer", HTTP_DELETE, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int hygrometer = atoi(r->getParam("id")->value().c_str());

            fGMS::Hygrometer* h;
            h = fGMS::GetHygrometer(hygrometer);

            if (h == nullptr) {
                r->send(400, "text/html", "Hygrometer not found");
                return;
            }
            RemoteLog.log(ESP_LOG_INFO, "fGMS Server", "Removing hygrometer");
            fGMS::RemoveHygrometer(hygrometer);

            RefreshConfig();
            r->send(200, "text/html", "ok");
            });

        server->on("/hygrometer", HTTP_GET, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int hygrometer = atoi(r->getParam("id")->value().c_str());

            fGMS::Hygrometer* h;
            h = fGMS::GetHygrometer(hygrometer);

            if (h == nullptr) {
                r->send(400, "text/html", "Hygrometer not found");
                return;
            }

            DynamicJsonDocument d = h->Save();
            String d_str;
            serializeJson(d, d_str);

            r->send(200, "application/json", d_str);
            });

        server->on(
            "/group",
            HTTP_POST,
            [](AsyncWebServerRequest* request) {},
            NULL,
            [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index, size_t total) {
                if (index != 0)
                {
                    Serial.println("Received chunked upload!");
                    r->send(200, "text/html", "failed");
                    return;
                }

                String data_string((const char*)data, len);
                DynamicJsonDocument doc(256);
                deserializeJson(doc, data_string);

                int group = r->hasParam("id") ? atoi(r->getParam("id")->value().c_str()) : -1;
                fGMS::HygrometerGroup* g;
                if (group == -1)
                    g = fGMS::CreateHygrometerGroup();
                else
                    g = fGMS::GetHygrometerGroup(group);

                if (g == nullptr)
                    r->send(400, "text/html", "Group not found");

                g->Load(doc.as<JsonObject>());

                RefreshConfig();
                r->send(200, "text/html", "ok");
            });

        server->on("/group", HTTP_DELETE, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int group = atoi(r->getParam("id")->value().c_str());

            fGMS::HygrometerGroup* g;
            g = fGMS::GetHygrometerGroup(group);

            if (g == nullptr) {
                r->send(400, "text/html", "Group not found");
                return;
            }

            fGMS::RemoveHygrometerGroup(group);

            RefreshConfig();
            r->send(200, "text/html", "ok");
            });

        server->on("/group", HTTP_GET, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int group = atoi(r->getParam("id")->value().c_str());

            fGMS::HygrometerGroup* g;
            g = fGMS::GetHygrometerGroup(group);

            if (g == nullptr) {
                r->send(400, "text/html", "Group not found");
                return;
            }

            DynamicJsonDocument d = g->Save();
            String d_str;
            serializeJson(d, d_str);

            r->send(200, "application/json", d_str);
            });

        server->on("/override", HTTP_GET, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id") || !r->hasParam("override") || !r->hasParam("water"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int group = atoi(r->getParam("id")->value().c_str());
            bool ovrride = atoi(r->getParam("override")->value().c_str());
            bool water = atoi(r->getParam("water")->value().c_str());

            fGMS::HygrometerGroup* g;
            g = fGMS::GetHygrometerGroup(group);

            if (g == nullptr) {
                r->send(400, "text/html", "Group not found");
                return;
            }

            g->SetOverride(ovrride, water);
            r->send(200, "text/html", "ok");
            });

        server->on(
            "/gms_config",
            HTTP_POST,
            [](AsyncWebServerRequest* request) {},
            NULL,
            [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index, size_t total) {
                if (index != 0)
                {
                    Serial.println("Received chunked upload!");
                    r->send(200, "text/html", "failed");
                    return;
                }

                String data_string((const char*)data, len);

                File data_file = LittleFS.open("/fGMS_Data.json", "w", true);
                data_file.print(data_string.c_str());
                data_file.close();

                ESP.restart();
            });

        server->on("/watering", HTTP_GET, [](AsyncWebServerRequest* r) {
            if (!r->hasParam("id") || !r->hasParam("override") || !r->hasParam("water"))
            {
                r->send(400, "text/html", "Invalid request");
                return;
            }

            int group = atoi(r->getParam("id")->value().c_str());
            bool ovrride = atoi(r->getParam("override")->value().c_str());
            bool water = atoi(r->getParam("water")->value().c_str());

            fGMS::HygrometerGroup* g;
            g = fGMS::GetHygrometerGroup(group);

            if (g == nullptr) {
                r->send(400, "text/html", "Group not found");
                return;
            }

            g->SetOverride(ovrride, water);
            r->send(200, "text/html", "ok");
            });

        /*server->on("/update", HTTP_POST, []() {
            server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            ESP.restart();
            }, []() {
                HTTPUpload& upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    Serial.printf("Update: %s\n", upload.filename.c_str());
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_WRITE) {
                    /* flashing firmware to ESP*
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_END) {
                    if (Update.end(true)) { //true to set the size to the current progress
                        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                    }
                    else {
                        Update.printError(Serial);
                    }
                }
            });*/


        server->onNotFound([](AsyncWebServerRequest* r) {
            r->send(404, "text/html", "Not found");
            });

        server->addHandler(events);

        server->begin();

        ftpsrv = new FtpServer();

        ftpsrv->begin("feoranis", "16777216");

        xTaskCreate(task, "fGMSServerTask", 8192, nullptr, 0, nullptr);
        xTaskCreate(FTPTask, "fGMFTPTask", 4096, nullptr, 0, nullptr);
    }
private:
    static AsyncWebServer* server;
    static AsyncEventSource* events;
    static FtpServer* ftpsrv;

    static int bytes_sent;
    static long restartMS;

    static DynamicJsonDocument& GetConfigJSON() {
        DynamicJsonDocument& d = *new DynamicJsonDocument(8192);

        Serial.println("hygro array");
        JsonArray hygrometersArray = d.createNestedArray("hygrometers");

        for (int i = 0; i < fGMS::HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();

            if (o["m"] = fGMS::Hygrometers[i]->Module != nullptr)
                o["m"] = fGMS::Hygrometers[i]->Module->module_mac;
            else
                o["m"] = "";
            o["c"] = fGMS::Hygrometers[i]->Channel;

            o["x"] = fGMS::Hygrometers[i]->x;
            o["y"] = fGMS::Hygrometers[i]->y;

            o["l"] = fGMS::Hygrometers[i]->map_min;
            o["u"] = fGMS::Hygrometers[i]->map_max;
        }

        Serial.println("group array");
        JsonArray groupsArray = d.createNestedArray("groups");

        for (int i = 0; i < fGMS::HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();
            o.set(fGMS::HygrometerGroups[i]->Save().as<JsonObject>());
        }

        Serial.println("hmodule array");
        JsonArray devicesArray = d.createNestedArray("devices");

        for (int i = 0; i < fGMS::HygrometerModuleCount; i++) {
            fGMS::HygrometerModule* mdl = fGMS::HygrometerModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "hygro";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");
            JsonArray values_array = data.createNestedArray("values");

            for (int i = 0; i < mdl->Channels; i++)
                values_array.add(floor(mdl->Values[i] * 100.0) / 100.0);

            data["channels"] = mdl->Channels;
        }

        Serial.println("vmodule array");
        for (int i = 0; i < fGMS::ValveModuleCount; i++) {
            fGMS::ValveModule* mdl = fGMS::ValveModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "valve";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");

            data["state"] = mdl->State;
        }

        Serial.println("smodule array");
        for (int i = 0; i < fGMS::SensorModuleCount; i++) {
            fGMS::SensorModule* mdl = fGMS::SensorModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "sensor";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");

            data["CO2"] = mdl->ppm;
            data["temp"] = mdl->temp;
            data["humidity"] = mdl->humidity;
        }

        Serial.println("data");
        JsonObject size = d.createNestedObject("hygrometerGridSize");

        size["x"] = fGMS::greenhouse.x_size;
        size["y"] = fGMS::greenhouse.y_size;

        return d;
    }

    static DynamicJsonDocument& GetDataJSON() {
        DynamicJsonDocument& d = *new DynamicJsonDocument(8192);

        JsonArray hygrometersArray = d.createNestedArray("h_val");

        for (int i = 0; i < fGMS::HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();

            o["id"] = i;
            float val = fGMS::Hygrometers[i]->GetValue();
            o["v"] = val >= 0 ? val * 100 : -1;

            o["r"] = fGMS::Hygrometers[i]->GetRaw();
        }

        JsonArray devicesArray = d.createNestedArray("devices");

        for (int i = 0; i < fGMS::HygrometerModuleCount; i++) {
            fGMS::HygrometerModule* mdl = fGMS::HygrometerModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "hygro";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");
            JsonArray values_array = data.createNestedArray("values");

            for (int i = 0; i < mdl->Channels; i++)
                values_array.add(floor(mdl->Values[i] * 100.0) / 100.0);

            data["channels"] = mdl->Channels;
        }

        for (int i = 0; i < fGMS::ValveModuleCount; i++) {
            fGMS::ValveModule* mdl = fGMS::ValveModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "valve";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");

            data["state"] = mdl->State;
        }

        for (int i = 0; i < fGMS::SensorModuleCount; i++) {
            fGMS::SensorModule* mdl = fGMS::SensorModules[i];
            JsonObject o = devicesArray.createNestedObject();

            o["n"] = mdl->mdl->Config["name"];
            o["t"] = "sensor";
            o["m"] = mdl->module_mac;
            o["o"] = mdl->ok;
            o["i"] = i;

            JsonObject data = o.createNestedObject("d");

            data["CO2"] = mdl->ppm;
            data["temp"] = mdl->temp;
            data["humidity"] = mdl->humidity;
        }

        JsonArray groupsArray = d.createNestedArray("g");

        for (int i = 0; i < fGMS::HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();

            o["i"] = i;
            float val = fGMS::HygrometerGroups[i]->GetAverage();
            o["a"] = val >= 0 ? val * 100 : -1;

            o["w"] = fGMS::AutomaticWatering ? fGMS::HygrometerGroups[i]->UpdateWateringState() : false;
        }

        d["auto"] = fGMS::AutomaticWatering;
        d["period"] = fGMS::watering_period;
        d["active"] = fGMS::watering_active_period;
        d["left"] = fGMS::GetSecondsToWater();
        d["until"] = fGMS::GetSecondsUntilWatering();
        d["wtr"] = fGMS::IsWatering();

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

        if (d.overflowed())
            Serial.println("OVERFLOWED!!");

        return d;
    }

    static DynamicJsonDocument& GetSystemJSON() {
        DynamicJsonDocument& d = *new DynamicJsonDocument(4096);

        d["uptime"] = millis();
        d["heap"] = ESP.getFreeHeap();
        d["block"] = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

        JsonArray stats = d.createNestedArray("module_statistics");

        for (int i = 0; i < fNETController::ModuleCount; i++) {
            JsonObject o = stats.createNestedObject();
            fNETController::Module* mdl = fNETController::Modules[i];

            o["id"] = i;
            o["mac"] = mdl->MAC_Address;
            o["name"] = mdl->Config["name"];
            o["type"] = mdl->Config["ModuleType"];
            o["ping"] = mdl->Ping;
            o["online"] = mdl->isOnline;
            //o["config"] = mdl->Config;
        }

        return d;
    }

    static void task(void* param) {
        while (true) {
            delay(2500);

            String data_str;
            DynamicJsonDocument& data = GetDataJSON();
            serializeJsonPretty(data, data_str);
            delete& data;

            String system_str;
            DynamicJsonDocument& data_sys = GetSystemJSON();
            serializeJsonPretty(data_sys, system_str);
            delete& data_sys;

            Serial.println("[fGMS Server Events] Sending: " + String(data_str.length()));
            Serial.println("[fGMS Server Events] Sending: " + String(system_str.length()));

            events->send("", "test");


            events->send(data_str.c_str(), "update");
            events->send(system_str.c_str(), "update_system");

            if (millis() - restartMS > 0 && restartMS != 0)
                ESP.restart();
        }
    }

    static void FTPTask(void* param) {
        while (true) {
            ftpsrv->handleFTP();
            delay(5);
        }
    }

    static void RefreshConfig() {
        String data_str;
        DynamicJsonDocument& data = GetConfigJSON();
        serializeJsonPretty(data, data_str);
        delete& data;

        Serial.println("[fGMS Server Events] Sending: " + data_str);

        events->send(data_str.c_str(), "config");
    }
};

#endif