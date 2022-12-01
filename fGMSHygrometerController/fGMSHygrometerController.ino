#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

#include "fNETLib.h"

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;
TwoWire auxI2C(1);

float sendInterval = 0;

fNETConnection* c;

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
    }
    else
        Serial.println("[fGMS HygroCtl] Connected to integrated ADC.");

    if (!adc2.begin(73, &auxI2C))
        Serial.println("[fGMS HygroCtl] No external ADC.");
    else {
        fNETModule::data["channels"] = 8;
        Serial.println("[fGMS HygroCtl] Connected to external ADC.");
    }

    adc1.setGain(GAIN_ONE);

    c->AddQueryResponder("getValues", [](DynamicJsonDocument q) {
        DynamicJsonDocument resp(256);

        JsonArray a = resp.createNestedArray("values");

        float humidity_0 = adc1.computeVolts(adc1.readADC_SingleEnded(0));
        float humidity_1 = adc1.computeVolts(adc1.readADC_SingleEnded(1));
        float humidity_2 = adc1.computeVolts(adc1.readADC_SingleEnded(2));
        float humidity_3 = adc1.computeVolts(adc1.readADC_SingleEnded(3));

        a.add<float>(humidity_0);
        a.add<float>(humidity_1);
        a.add<float>(humidity_2);
        a.add<float>(humidity_3);

        return resp;
        });

    c->AddQueryResponder("setValueInterval", [](DynamicJsonDocument q) {
        sendInterval = q["interval"];

        Serial.println("[fGMS HygroCtl] Set send interval: " + String(sendInterval) + " ms.");

        DynamicJsonDocument resp(256);
        resp["interval"] = String(sendInterval);
        return resp;
        });
}

long lastSentMS;
long data_id;

void SendData() {
    if (sendInterval != 0 && millis() - lastSentMS > sendInterval && c->GetQueuedMessageCount() <= 2) {
        DynamicJsonDocument send(256);

        JsonArray a = send.createNestedArray("values");

        float humidity_0 = adc1.computeVolts(adc1.readADC_SingleEnded(0));
        float humidity_1 = adc1.computeVolts(adc1.readADC_SingleEnded(1));
        float humidity_2 = adc1.computeVolts(adc1.readADC_SingleEnded(2));
        float humidity_3 = adc1.computeVolts(adc1.readADC_SingleEnded(3));

        a.add<float>(humidity_0);
        a.add<float>(humidity_1);
        a.add<float>(humidity_2);
        a.add<float>(humidity_3);

        send["recipient"] = "controller";
        send["tag"] = "values";
        send["auto"] = true;
        send["dataID"] = data_id++;

        c->Send(send);

        lastSentMS = millis();
    }
}

void loop() {
    delay(100);

    SendData();
    fNETModule::working = sendInterval != 0 && c->GetQueuedMessageCount() <= 2;
}
