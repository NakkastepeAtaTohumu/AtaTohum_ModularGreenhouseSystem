#define I2C_BUFFER_LENGTH 128

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>


#include <TFT_eSPI.h>
#include <ESP32Encoder.h>

#include "fNETLib.h"
#include "fGUILib.h"

#include <ESP32Encoder.h>
#include "fGMSControllerGUI.h"
#include "fGMS.h"

#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "fGMSServer.h"

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

    fNETConnection* c = fNETController::Init();
    fGMS::Init(c);


    //fGMS::SensorModules[0]->SetFan(true);


    if (fGMS::serverEnabled) {
        WiFi.mode(WIFI_STA);
        //WiFi.softAP("AtaTohum_MGMS", "16777216");
        WiFi.setHostname("main");
        WiFi.hostname("main");
        WiFi.begin("ARMATRON_NETWORK", "16777216");

        MDNS.begin("main");

        fGMSServer::Init();

        fGMS::serverEnabled = false;
        fGMS::Save();
    }
    else {
        fGMSControllerMenu::Init();
    }

    //fGMS::Hygrometer* h = fGMS::CreateHygrometer();
    //fGMS::Save();
}

void loop() {
    //vTaskDelete(NULL); //loop() is unused

    Serial.println("Free heap: " + String(ESP.getFreeHeap()));

    /*char buf[2048];
    vTaskGetRunTimeStats(buf);;
    Serial.println(String(buf));*/

    delay(2500);
}
