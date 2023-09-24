#include <painlessMesh.h>

#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <CircularBuffer.h>

#include <ESPmDNS.h>
#include <Update.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>

#include "fNETLib.h"


#include <PCF8574.h>


fNETConnection* c;

painlessMesh mesh;
fNETTunnel* data_tunnel;

float sendInterval = 2000;

PCF8574 io(0x20);

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS ControlPanel] Controller starting");

    mesh.init("Ata_Tohum_MESH", "16777216", 11753, WIFI_MODE_APSTA, 1);
    mesh.setContainsRoot(true);

    c = fNET_Mesh::Init(&mesh);

    fNETModule::Init(c);
    fNETModule::data["ModuleType"] = "ValveCtl";
    fNETModule::data["name"] = "VALVE A";

    data_tunnel = new fNETTunnel(c, "data");
    data_tunnel->Init();
    data_tunnel->AcceptIncoming();

    bool ok = io.begin(25, 26);

    if (!ok)
        ESP_LOGE("fGMS Ctrl", "Failed to initialize PCF8574!");

    io.write(3, LOW);
    io.write(4, LOW);

    ledcSetup(0, 3000, 8);
    ledcAttachPin(18, 0);
}

long lastSentMS;
long data_id;

DynamicJsonDocument* GetValueJSON() {
    DynamicJsonDocument& send = *new DynamicJsonDocument(256);

    return &send;
}

void SendData() {
    if (sendInterval != 0 && millis() - lastSentMS > sendInterval) {
        DynamicJsonDocument& send = *GetValueJSON();
        data_tunnel->Send(send);
        delete& send;

        lastSentMS = millis();
        //Serial.println("send data ok");
    }
}

long last_connected = 0;

void loop() {
    ledcWrite(0, 128);
    delay(100);

    SendData();

    fNETModule::working = true;

    if (data_tunnel->IsConnected)
        last_connected = millis();

    if (millis() - last_connected > 60000) {
        Serial.println("[fGMS ControlPanel] Could not connect for too long. Restarting.");
        ESP.restart();
    }

    if (!io.read(0))
    {
        io.write(3, HIGH);
        io.write(4, HIGH);
        io.write(5, LOW);
    }
    else {
        io.write(3, !io.read(2));
        io.write(4, !io.read(1));
        io.write(5, HIGH);
    }
    //Serial.println("loop ok");
}