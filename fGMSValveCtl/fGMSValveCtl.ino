#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

#include "fNETLib.h"

fNETConnection* c;

byte valve_state;

#define PIN_VALVE1 16
#define PIN_VALVE2 17
#define PIN_VALVE3 18
#define PIN_VALVE4 19

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS HygroCtl] Hygrometer Controller starting");

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "ValveCtl";
    fNETModule::data["name"] = "VALVE A";
    

    pinMode(PIN_VALVE1, OUTPUT);
    pinMode(PIN_VALVE2, OUTPUT);
    pinMode(PIN_VALVE3, OUTPUT);
    pinMode(PIN_VALVE4, OUTPUT);

    c->AddQueryResponder("setState", [](DynamicJsonDocument q) {
        SetValveState(q["state"]);

        Serial.println("[fGMS ValveCtl] Set state: " + String(valve_state));
        DynamicJsonDocument resp(32);
        resp["state"] = String(valve_state);
        return resp;
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

void loop() {
    delay(100);

    fNETModule::working = valve_state;
    if (!c->IsConnected)
        SetValveState(0b0000);
}
