#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <CircularBuffer.h>
#include <esp_log.h>

#include "fNET.h"
#include "fNETLib.h"

#define LED_PIN 22
#define LED_ON LOW
#define LED_OFF HIGH

painlessMesh mesh;

bool inhibit_led;

void newConnectionCallback(uint32_t nodeId) {
    ESP_LOGI("main", "New connection: %s.", String(nodeId, 16));
    ESP_LOGI("main", "Connection list: %s.", String(mesh.subConnectionJson()));
}

void indicator_task(void* param) {
    bool isOK = false;

    while (true) {
        if (!isOK && fNETModule::comm_tunnel->IsConnected)
        {
            digitalWrite(LED_PIN, LED_ON);
            delay(200);
            digitalWrite(LED_PIN, LED_OFF);
            delay(50);
            digitalWrite(LED_PIN, LED_ON);
            delay(200);
            digitalWrite(LED_PIN, LED_OFF);
            delay(50);
            digitalWrite(LED_PIN, LED_ON);
            delay(500);
            digitalWrite(LED_PIN, LED_OFF);
            delay(100);
        }

        isOK = fNETModule::comm_tunnel->IsConnected;

        if (millis() - fNETModule::comm_tunnel->lastTransmissionMS < 100) {
            digitalWrite(LED_PIN, (isOK && mesh.getNodeTime() % 1000000 < 500000) ? LED_OFF : LED_ON);
            delay(100);
            digitalWrite(LED_PIN, (isOK && mesh.getNodeTime() % 1000000 < 500000) ? LED_ON : LED_OFF);
            delay(100);
            digitalWrite(LED_PIN, (isOK && mesh.getNodeTime() % 1000000 < 500000) ? LED_OFF : LED_ON);
            delay(100);
        }

        digitalWrite(LED_PIN, (isOK && mesh.getNodeTime() % 1000000 < 500000) ? LED_ON : LED_OFF);
        delay(25);
    }
}

void on_connect(void* param) {
    while (true) {
        vTaskSuspend(NULL);
        delay(0);


        inhibit_led = true;
        digitalWrite(LED_PIN, LED_ON);
        delay(200);
        digitalWrite(LED_PIN, LED_OFF);
        delay(50);
        digitalWrite(LED_PIN, LED_ON);
        delay(200);
        digitalWrite(LED_PIN, LED_OFF);
        delay(50);
        digitalWrite(LED_PIN, LED_ON);
        delay(500);
        digitalWrite(LED_PIN, LED_OFF);
        delay(100);
        inhibit_led = false;
    }
}

void on_disconnect(void* param) {
    while (true) {
        vTaskSuspend(NULL);
        delay(0);


        inhibit_led = true;
        digitalWrite(LED_PIN, LED_ON);
        delay(500);
        digitalWrite(LED_PIN, LED_OFF);
        delay(100);
        inhibit_led = false;
    }
}
TaskHandle_t c_task;
TaskHandle_t dc_task;

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    mesh.init("Ata_Tohum_MESH", "16777216", 11753);
    mesh.onNewConnection(&newConnectionCallback);

    //mesh.setRoot(true);
    mesh.setContainsRoot(true);

    fNETConnection* c = fNET_Mesh::Init(&mesh);
    //fNETController::Init(c);

    //fNETController::Scan();
    fNETModule::Init(c);

    xTaskCreate(indicator_task, "led_task", 1024, NULL, 0, NULL);
    //xTaskCreate(on_connect, "c_task", 1024, NULL, 0, &c_task);
    //xTaskCreate(on_disconnect, "dc_task", 1024, NULL, 0, &dc_task);

    fNETController::OnModuleConnected.AddHandler(new EventHandlerFunc([](void* param) {
        vTaskResume(c_task);
        }));

    fNETController::OnModuleDisconnected.AddHandler(new EventHandlerFunc([](void* param) {
        vTaskResume(dc_task);
        }));

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);
}

// the loop function runs over and over again until power down or reset
void loop() {
    delay(25);

    //if (!inhibit_led)
    //    digitalWrite(LED_PIN, (mesh.getNodeTime() % 1000000 < 500000) ? LED_ON : LED_OFF);
    //digitalWrite(5, fNETModule::comm_tunnel->IsConnected);
}
