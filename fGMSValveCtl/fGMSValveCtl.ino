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

fNETConnection* c;

byte valve_state;

#define PIN_VALVE1 16
#define PIN_VALVE2 17
#define PIN_VALVE3 18
#define PIN_VALVE4 19

painlessMesh mesh;
fNETTunnel* data_tunnel;

float sendInterval = 2000;

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS ValveCtl] Valve Controller starting");

    mesh.init("Ata_Tohum_MESH", "16777216", 11753, WIFI_MODE_APSTA, 1);
    mesh.setContainsRoot(true);

    c = fNET_Mesh::Init(&mesh);

    Serial.println("Waiting for mesh to form.");
    delay(10000);

    fNETModule::Init(c);
    fNETModule::data["ModuleType"] = "ValveCtl";
    fNETModule::data["name"] = "VALVE A";

    data_tunnel = new fNETTunnel(c, "data");
    data_tunnel->Init();
    data_tunnel->AcceptIncoming();
    

    pinMode(PIN_VALVE1, OUTPUT);
    pinMode(PIN_VALVE2, OUTPUT);
    pinMode(PIN_VALVE3, OUTPUT);
    pinMode(PIN_VALVE4, OUTPUT);

    c->AddQueryResponder("setState", [](DynamicJsonDocument* q) {
        SetValveState((*q)["state"]);

        Serial.println("[fGMS ValveCtl] Set state: " + String(valve_state));
        DynamicJsonDocument& resp = *new DynamicJsonDocument(32);
        resp["state"] = String(valve_state);
        return &resp;
        });

    SetValveState(0b0001);
    delay(25);
    SetValveState(0b0010);
    delay(25);
    SetValveState(0b0100);
    delay(25);
    SetValveState(0b1000);
    delay(25);
    SetValveState(0b0000);
}

void SetValveState(byte valve_s) {
    valve_state = valve_s;
    digitalWrite(PIN_VALVE1, !(valve_state & 0b0001));
    digitalWrite(PIN_VALVE2, !(valve_state & 0b0010));
    digitalWrite(PIN_VALVE3, !(valve_state & 0b0100));
    digitalWrite(PIN_VALVE4, !(valve_state & 0b1000));
}

long lastSentMS;
long data_id;

DynamicJsonDocument* GetValueJSON() {
    DynamicJsonDocument& send = *new DynamicJsonDocument(256);
    send["state"] = String(valve_state);

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
    delay(100);

    SendData();

    fNETModule::working = valve_state;
    if (!data_tunnel->IsConnected)
        SetValveState(0b0000);

    if (data_tunnel->IsConnected)
        last_connected = millis();

    if (millis() - last_connected > 60000) {
        Serial.println("[fGMS ValveCtl] Could not connect for too long. Restarting.");
        ESP.restart();
    }
    //Serial.println("loop ok");
}