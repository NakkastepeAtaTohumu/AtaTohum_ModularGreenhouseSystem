#define I2C_BUFFER_LENGTH 4096

//#include <Wire.h>
//#include "fNETLib.h"

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

    //DynamicJsonDocument d(256);

    //d["recipient"] = "34:86:5D:FB:F4:E8";
    //d["tag"] = "test from controller";

    //c->Send(d);

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
}

void loop() {
    //Serial.println("Free heap: " + String(ESP.getFreeHeap()));
    delay(100);
}
