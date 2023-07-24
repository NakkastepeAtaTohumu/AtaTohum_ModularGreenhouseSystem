//#define I2C_BUFFER_LENGTH 128

#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

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
    Serial.println("Starting");

    //pinMode(4, OUTPUT);

    //digitalWrite(4, HIGH);
    //delay(250);
    //digitalWrite(4, LOW);

    //delay(5000);

    WiFi.mode(WIFI_STA);
    WiFi.setHostname("main");
    WiFi.hostname("main");
    //WiFi.softAP("AtaTohum_MGMS", "16777216");
    //WiFi.begin("ARMATRON_NETWORK", "16777216");

    MDNS.begin("main");

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
        fGMSServer::Init();

        //fGMS::serverEnabled = false;
        //fGMS::Save();

        ESP32Encoder* e = new ESP32Encoder();
        ESP32Encoder::useInternalWeakPullResistors = UP;
        e->attachSingleEdge(22, 21);
        pinMode(34, INPUT);

        fGUI::AttachButton(16);
        fGUI::AttachButton(17);
        fGUI::AttachButton(19);
        fGUI::AttachButton(27);

        fGUI::AddMenu(new ServerGUI());

        fGUI::Init(e, 34);
    }
    else {
        Serial.println("Heap      : " + String(ESP.getFreeHeap()));
        delay(2000);
        Serial.println("Heap      : " + String(ESP.getFreeHeap()));
        fGMSControllerMenu::Init();
    }

    //fGMS::Hygrometer* h = fGMS::CreateHygrometer();
    //fGMS::Save();
}

void loop() {
    //vTaskDelete(NULL); //loop() is unused
    //delay(0);
    
    float ms = millis();
    float seconds = ms / 1000;
    float minutes = seconds / 60.0;
    float hours = minutes / 60.0;
    float days = hours / 24.0;

    String uptime = String(floor(days), 0) + "d" + String(floor(fmod(hours, 24)), 0) + "h" + String(floor(fmod(minutes, 60)), 0) + "m" + String(floor(fmod(seconds, 60)), 0) + "s";

    Serial.println("Up        : " + String(uptime));
    Serial.println("Heap      : " + String(ESP.getFreeHeap()));
    Serial.println("IP        : " + WiFi.localIP().toString());

    /*char buf[2048];
    vTaskGetRunTimeStats(buf);;
    Serial.println(String(buf));
    if (Serial.available()) {
        String read = "";

        bool loop = true;
        while (loop) {
            read += Serial.readString();
            Serial.println(read);
            while(!Serial.available())
                delay(100);

            if (Serial.peek() == 'C')
                loop = false;
        }

        Serial.println("FULL: " + read);
        LittleFS.begin();
        File f = LittleFS.open("/fGMS_Data.json", "w", true);
        f.print(read);
        f.close();
    }*/

    delay(2500);
}
