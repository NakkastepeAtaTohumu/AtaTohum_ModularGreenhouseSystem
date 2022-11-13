#include "fGMSLib.h"
#include "fSerialParser.h"

#include <ArduinoOTA.h>
#include <esp_wifi.h>

uint8_t MAC[] = { 0x34, 0x86, 0x5D, 0xFB, 0xF4, 0xE8 };

void setup() {
    fSerialParser::BeginAsTask(115200);
    //Serial.begin(115200);

    Serial.println("fGMS Simple Module Test Startup...");

    esp_wifi_set_mac(WIFI_IF_STA, &MAC[0]);
    WiFi.begin("BK_School", "8K-$cH0oL!");

    fGMSModule::Init();

    fSerialParser::AddCommand("query", []() {
        Serial.println("[fGMS Main] Sending query...");
        DynamicJsonDocument q(512);
        q["query"] = "test_responder";

        JsonObject* d = fGMSModule::Query(q);

        if (d != nullptr) {
            String resp;
            serializeJson(*d, resp);

            Serial.println("[fGMS Main] Received result: " + resp);
            delete d;
        }
        else
            Serial.println("[fGMS Main] Query failed!");
        });
}

void loop() {
    ArduinoOTA.handle();
    //Serial.println("[fGMS Main] Free heap: " + String(ESP.getFreeHeap()));
    delay(100);
}
