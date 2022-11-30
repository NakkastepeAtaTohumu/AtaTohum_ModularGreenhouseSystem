#include <Adafruit_ADS1X15.h>
#include "fNETLib.h"

#include <Wire.h>

Adafruit_ADS1115 adc1;
TwoWire auxI2C(1);

float sendInterval = 0;

fNETConnection* c;

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS HygroCtl] Hygrometer Controller starting");

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "HygroCtl";
    fNETModule::data["name"] = "HYGRO {uniqueLetter}";

    auxI2C.begin(fNET_SDA2, fNET_SCK2);

    if (!adc1.begin(72, &auxI2C)) {
        Serial.println("[fGMS HygroCtl] Can't connect to integrated ADC!");
        fNETModule::SetFatalErrorState();
    }
    else
        Serial.println("[fGMS HygroCtl] Connected to integrated ADC.");

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

        c->Send(send);

        lastSentMS = millis();
    }
}

void loop() {
    delay(100);

    SendData();
}
