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

#define fNET_USE_CUSTOM_PINS

#define fNET_SDA 25
#define fNET_SCK 26

#define fNET_SDA2 18
#define fNET_SCK2 19

#define fNET_PIN_INDICATOR_G 16
#define fNET_PIN_INDICATOR_Y 27
#define fNET_PIN_INDICATOR_R 14

#include "fNETLib.h"

fNETConnection* c;

byte state;

painlessMesh mesh;
fNETTunnel* data_tunnel;
float sendInterval = 2000;

#define PIN_VALVE1 32
#define PIN_VALVE2 33
#define PIN_VALVE3 13

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS LidCtl] Lid Motor Controller starting");

    mesh.init("Ata_Tohum_MESH", "16777216", 11753, WIFI_MODE_APSTA, 1);

    //mesh.setRoot(true);
    mesh.setContainsRoot(true);

    //MDNS.begin("Ata_Tohum_Hygro_A");
    //WiFi.setHostname("Ata_Tohum_Hygro_A");

    //mesh.stationManual("ARMATRON_NETWORK", "16777216");
    //mesh.setHostname("Ata_Tohum_MAIN");

    c = fNET_Mesh::Init(&mesh);

    Serial.println("Waiting for mesh to form.");
    delay(10000);

    fNETModule::Init(c);
    fNETModule::data["ModuleType"] = "ValveCtl";
    fNETModule::data["name"] = "LID A";

    SetState(0b0000);

    pinMode(PIN_VALVE1, OUTPUT);
    pinMode(PIN_VALVE2, OUTPUT);
    pinMode(PIN_VALVE3, OUTPUT);

    c->AddQueryResponder("setState", [](DynamicJsonDocument* q) {
        SetState((*q)["state"]);

        Serial.println("[fGMS ValveCtl] Set state: " + String(state));
        DynamicJsonDocument& resp = *new DynamicJsonDocument(32);
        resp["state"] = String(state);
        return &resp;
        });

    data_tunnel = new fNETTunnel(c, "data");
    data_tunnel->Init();
    data_tunnel->AcceptIncoming();

    /*
    SetState(0b0000);
    delay(500);
    SetState(0b0001);
    delay(5000);
    SetState(0b0000);
    delay(500);
    SetState(0b0010);
    delay(5000);
    SetState(0b0000);
    delay(5000);
    SetState(0b0010);
    delay(10000);
    SetState(0b0000);*/
}

void SetState(byte valve_s) {
    state = valve_s;
    Serial.println("Set state:" + String(state, 2));

    //if ((state & 0b0001 > 0) && (state & 0b0010 > 0))
    //    state = 0b0000;

    Serial.println("State is:" + String(state, 2));
    digitalWrite(PIN_VALVE1, !(state & 0b0001));
    digitalWrite(PIN_VALVE2, !(state & 0b0010));
}

long lastSentMS;
long data_id;

DynamicJsonDocument* GetValueJSON() {
    DynamicJsonDocument& send = *new DynamicJsonDocument(256);
    send["state"] = String(state);

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

    fNETModule::working = state;
    if (!data_tunnel->IsConnected)
        SetState(0b0000);

    if (data_tunnel->IsConnected)
        last_connected = millis();

    if (millis() - last_connected > 60000) {
        Serial.println("[fGMS LidCtl] Could not connect for too long. Restarting.");
        ESP.restart();
    }
    //Serial.println("loop ok");
}
