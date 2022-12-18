#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

#include "fNETLib.h"

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;
Adafruit_ADS1115 adc3;
Adafruit_ADS1115 adc4;

Adafruit_ADS1115* adcs[4];
int adcNum;

TwoWire auxI2C(1);

float sendInterval = 0;

fNETConnection* c;
fNETTunnell* tunnel;

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS HygroCtl] Hygrometer Controller starting");

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "HygroCtl";
    fNETModule::data["name"] = "HYGRO A"; //TODO Unique to moduele!
    fNETModule::data["channels"] = 4;

    auxI2C.begin(fNET_SDA2, fNET_SCK2);

    if (!adc1.begin(72, &auxI2C)) {
        Serial.println("[fGMS HygroCtl] Can't connect to integrated ADC!");
        fNETModule::SetFatalErrorState();
        return;
    }
    else {
        Serial.println("[fGMS HygroCtl] Connected to integrated ADC.");
        adc2.setGain(GAIN_ONE);

        adcs[adcNum++] = &adc1;
    }

    if (!adc2.begin(73, &auxI2C))
        Serial.println("[fGMS HygroCtl] External ADC 1 not present.");
    else {
        Serial.println("[fGMS HygroCtl] Connected to external ADC 1.");
        adc2.setGain(GAIN_ONE);

        adcs[adcNum++] = &adc2;
    }

    fNETModule::data["channels"] = adcNum * 4;

    c->AddQueryResponder("getValues", [](DynamicJsonDocument q) {
        DynamicJsonDocument resp = GetValueJSON();
        return resp;
        });

    c->AddQueryResponder("setValueInterval", [](DynamicJsonDocument q) {
        sendInterval = q["interval"];

        Serial.println("[fGMS HygroCtl] Set send interval: " + String(sendInterval) + " ms.");

        DynamicJsonDocument resp(256);
        resp["interval"] = String(sendInterval);
        return resp;
        });

    tunnel = new fNETTunnell(c, "data");
    tunnel->AcceptIncoming();
}

long lastSentMS;
long data_id;

DynamicJsonDocument GetValueJSON() {
    DynamicJsonDocument send(256);

    JsonArray a = send.createNestedArray("hygrometers");
    for (int adcN = 0; adcN < adcNum; adcN++) {
        Adafruit_ADS1115* adc = adcs[adcN];

        for (int i = 0; i < 4; i++)
            a.add<float>(adc->computeVolts(adc->readADC_SingleEnded(i)));
    }

    return send;
}

void SendData() {
    if (sendInterval != 0 && millis() - lastSentMS > sendInterval && c->GetQueuedMessageCount() <= 5) {
        DynamicJsonDocument send = GetValueJSON();
        tunnel->Send(send);

        lastSentMS = millis();
    }
}

void loop() {
    delay(100);

    SendData();
    fNETModule::working = sendInterval != 0 && c->GetQueuedMessageCount() <= 5;
}
