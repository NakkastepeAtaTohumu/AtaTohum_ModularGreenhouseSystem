#define I2C_BUFFER_LENGTH 128

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include "fNETLib.h"
#include "fGUILib.h"

#include <ESP32Encoder.h>
#include "fGMSControllerGUI.h"
#include "fGMS.h"

//#include "fSerialParser.h"

//fNETSlaveConnection* d = new fNETSlaveConnection((uint8_t)0x01, (int)0);

void setup() {
    //fSerialParser::BeginAsTask(115200);
    Serial.begin(115200);

    pinMode(2, OUTPUT);

    digitalWrite(2, HIGH);
    delay(250);
    digitalWrite(2, LOW);

    //Wire.begin(25, 26, (uint32_t)800000);
    //Serial.println(d->Transaction(&Wire, "GETMAC"));
    /*
    fSerialParser::AddCommand("save", []() {
        fNETController::Save();
        });

    fSerialParser::AddCommand("reset", []() {
        LittleFS.remove("/fGMS_SystemData.json");
        ESP.restart();
        });*/

    fGMSControllerMenu::Init();
    fNETConnection* c = fNETController::Init();
    fGMS::Init(c);

    DynamicJsonDocument d(256);
    d["state"] = 0b1111;
    JsonObject* result = c->Query("C8:F0:9E:9E:7C:80", "setState", &d);
}

void loop() {
    Serial.println("Free heap: " + String(ESP.getFreeHeap()));
    //vTaskDelete(NULL); //loop() is unused
    delay(1000);
}
