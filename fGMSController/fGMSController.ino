#define I2C_BUFFER_LENGTH 128

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include "fNETLib.h"
#include "fGUILib.h"

#include <ESP32Encoder.h>
#include "fGMSControllerGUI.h"

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

    JsonObject* r = c->Query("C8:F0:9E:9E:70:38", "getValues");

    DynamicJsonDocument d(256);
    d["interval"] = 1000;
    c->Query("C8:F0:9E:9E:70:38", "setValueInterval", &d);

    if (r != nullptr) {
        String f;
        serializeJsonPretty(*r, f);

        Serial.println("Query return:\n"+ f);
    }
}

void loop() {
    vTaskDelete(NULL); //loop() is unused
    delay(0);
}
