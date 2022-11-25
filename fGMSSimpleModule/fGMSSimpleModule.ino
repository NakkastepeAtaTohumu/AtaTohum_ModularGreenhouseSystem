#include <ESP32Tone.h>
#include <ESP32Servo.h>
#include <ESP32PWM.h>
#include <analogWrite.h>
#include "fNETLib.h"
#include "fSerialParser.h"

#include <ArduinoOTA.h>
#include <esp_wifi.h>

void setup() {
    fSerialParser::BeginAsTask(115200);
    //Serial.begin(115200);

    Serial.println("fGMS Simple Module Test Startup...");

    WiFi.begin("BK_School", "8K-$cH0oL!");

    fNETConnection* c = fNETModule::Init();

    DynamicJsonDocument d(512);

    d["tag"] = "test";

    c->Send(d);

    DynamicJsonDocument d2(512);

    d2["recipient"] = "24:0A:C4:EC:9C:3C";
    d2["tag"] = "test1234";

    c->Send(d2);

    fSerialParser::AddCommand("save", []() {
        fNETModule::Save();
        });

    fSerialParser::AddCommand("reset", []() {
        LittleFS.remove("/fGMS_ModuleData.json");
        ESP.restart();
        });

    /*fSerialParser::AddCommand("query", []() {
        Serial.println("[fGMS Main] Sending query...");
        DynamicJsonDocument q(512);
        q["query"] = "test_responder";

        JsonObject* d = fNETModule::Query(q);

        if (d != nullptr) {
            String resp;
            serializeJson(*d, resp);

            Serial.println("[fGMS Main] Received result: " + resp);
            delete d;
        }
        else
            Serial.println("[fGMS Main] Query failed!");
        });*/
}

void loop() {
    ArduinoOTA.handle();
    //Serial.println("[fGMS Main] Free heap: " + String(ESP.getFreeHeap()));
    delay(100);
}
