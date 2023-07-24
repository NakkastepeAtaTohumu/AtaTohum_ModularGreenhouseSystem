#include <SPI.h>
#include <WiFi.h>

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

#define PIN_VALVE1 32
#define PIN_VALVE2 33
#define PIN_VALVE3 13

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS LidCtl] Lid Motor Controller starting");

    WiFi.mode(WIFI_STA);

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "ValveCtl";
    fNETModule::data["name"] = "LID A";

    SetState(0b0000);

    pinMode(PIN_VALVE1, OUTPUT);
    pinMode(PIN_VALVE2, OUTPUT);
    pinMode(PIN_VALVE3, OUTPUT);

    c->AddQueryResponder("setState", [](DynamicJsonDocument q) {
        SetState(q["state"]);

        Serial.println("[fGMS LidCtl] Set state: " + String(state));
        DynamicJsonDocument resp(32);
        resp["state"] = String(state);
        return resp;
        });


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
    SetState(0b0000);
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

void loop() {
    delay(100);

    fNETModule::working = state;
    if (!c->IsConnected && state > 0)
        SetState(0b0000);

    if(state & 0b0011 > 0)
        digitalWrite(PIN_VALVE3, millis() % 500 < 250);
}
