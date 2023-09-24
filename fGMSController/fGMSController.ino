//#define I2C_BUFFER_LENGTH 128
#define DEFAULT_FTP_SERVER_NETWORK_TYPE_ESP32 NETWORK_ESP32
#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_LITTLEFS

#include <painlessMesh.h>

#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

#include <Update.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <NTPClient.h>

#include "fNETLib.h"

#include <TFT_eSPI.h>
#include <ESP32Encoder.h>
#include <zutil.h>
#include <PNGenc.h>
#include "fGUI.h"


#include "fGMSControllerGUI.h"
#include "fGMS.h"

#include <Update.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <FFat.h>
#include <SimpleFTPServer.h>
#include "fGMSServer.h"

#include "esp_bt.h"

//#include "fSerialParser.h"

//fNETSlaveConnection* d = new fNETSlaveConnection((uint8_t)0x01, (int)0);

painlessMesh mesh;

//#define configUSE_TRACE_FACILITY 1

void newConnectionCallback(uint32_t nodeId) {
    ESP_LOGI("main", "New connection: %s.", String(nodeId, 16).c_str());
    ESP_LOGI("main", "Connection list: %s.", String(mesh.subConnectionJson()).c_str());
}

void setup() {
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    //fSerialParser::BeginAsTask(115200);
    Serial.begin(115200);
    Serial.println("Starting");
    Serial.printf("trace: %d\n", configUSE_TRACE_FACILITY);

    //pinMode(4, OUTPUT);

    //digitalWrite(4, HIGH);
    //delay(250);
    //digitalWrite(4, LOW);

    //delay(5000);

    //WiFi.mode(WIFI_STA);
    //WiFi.setHostname("main");
    //WiFi.hostname("main");
    //WiFi.softAP("AtaTohum_MGMS", "16777216");
    //WiFi.begin("ARMATRON_NETWORK", "16777216");


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

    mesh.init("Ata_Tohum_MESH", "16777216", 11753, WIFI_MODE_APSTA, 1);

    mesh.stationManual("Ata_Tohum_Sera", "Sera_16777216", 0, IPAddress(192, 168, 5, 4));
    mesh.setHostname("Ata_Tohum_Controller");

    mesh.setRoot(true);
    mesh.setContainsRoot(true);

    mesh.onNewConnection(newConnectionCallback);

    MDNS.begin("Ata_Tohum_MAIN");
    WiFi.setHostname("Ata_Tohum_MAIN");

    fNETConnection* c = fNET_Mesh::Init(&mesh);

    fNETController::Init(c);
    fGMS::Init(c);


    //fGMS::SensorModules[0]->SetFan(true);

    fGMSServer::Init();

    /*if (fGMS::serverEnabled) {
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

        fGUI::Init(/*e, 34);*/
    //}
    //else {
        fGMSControllerMenu::Init();
    //}

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

    Serial.println("Up              : " + String(uptime));
    Serial.println("Heap            : " + String(ESP.getFreeHeap()));
    Serial.println("Largest block   : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
    Serial.println("IP              : " + WiFi.localIP().toString());

    /*
    TaskStatus_t statuses[32];
    uxTaskGetSystemState(statuses, 32, NULL);

    for (int i = 0; i < uxTaskGetNumberOfTasks(); i++) {
        TaskStatus_t& status = statuses[i];

        Serial.printf("Task info        : %s\n", status.pcTaskName);
        Serial.printf("Runtime          : %d\n", status.ulRunTimeCounter * xPortGetTickRateHz());
        Serial.printf("Highest stack    : %s\n", status.usStackHighWaterMark);
        Serial.printf("------------------\n");
    }*/
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
