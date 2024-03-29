#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

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

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;
Adafruit_ADS1115 adc3;
Adafruit_ADS1115 adc4;

Adafruit_ADS1115* adcs[4];
int adcNum;

TwoWire auxI2C(1);

float sendInterval = 2000;

fNETConnection* c;
fNETTunnel* tunnel;

painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS HygroCtl] Hygrometer Controller starting");

    mesh.init("Ata_Tohum_MESH", "16777216", 11753, WIFI_MODE_APSTA, 1);

    //mesh.setRoot(true);
    mesh.setContainsRoot(true);

    //MDNS.begin("Ata_Tohum_Hygro_A");
    //WiFi.setHostname("Ata_Tohum_Hygro_A");

    //mesh.stationManual("ARMATRON_NETWORK", "16777216");
    //mesh.setHostname("Ata_Tohum_MAIN");

    c = fNET_Mesh::Init(&mesh);

    Serial.println("Waiting for mesh to form.");
    fNETModule::Idle_blink(10000);

    fNETModule::Init(c);
    fNETModule::data["ModuleType"] = "HygroCtl";
    fNETModule::data["name"] = "HYGRO A";
    fNETModule::data["channels"] = 4;

    auxI2C.begin(fNET_SDA2, fNET_SCK2);

    if (!adc1.begin(72, &auxI2C)) {
        Serial.println("[fGMS HygroCtl] Can't connect to integrated ADC!");
        fNETModule::SetFatalErrorState();
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

    c->AddQueryResponder("getValues", [](DynamicJsonDocument* q) {
        DynamicJsonDocument* resp = GetValueJSON();
        return resp;
        });

    c->AddQueryResponder("setValueInterval", [](DynamicJsonDocument* q) {
        sendInterval = (*q)["interval"];

        Serial.println("[fGMS HygroCtl] Set send interval: " + String(sendInterval) + " ms.");

        DynamicJsonDocument& resp = *new DynamicJsonDocument(256);
        resp["interval"] = String(sendInterval);
        return &resp;
        });

    tunnel = new fNETTunnel(c, "data");
    tunnel->Init();
    tunnel->AcceptIncoming();
}

long lastSentMS;
long data_id;

DynamicJsonDocument* GetValueJSON() {
    DynamicJsonDocument& send = *new DynamicJsonDocument(256);

    JsonArray a = send.createNestedArray("hygrometers");
    for (int adcN = 0; adcN < adcNum; adcN++) {
        Adafruit_ADS1115* adc = adcs[adcN];

        for (int i = 0; i < 4; i++) {
            float d = adc->computeVolts(adc->readADC_SingleEnded(i));
            //Serial.println("REad:" + String(i) + "=" + String(d));
            a.add<int>(floor(d * 100));
        }
    }

    return &send;
}

void SendData() {
    if (sendInterval != 0 && millis() - lastSentMS > sendInterval) {
        DynamicJsonDocument& send = *GetValueJSON();
        tunnel->Send(send);
        delete& send;

        lastSentMS = millis();
        //Serial.println("send data ok");
    }
}

long last_connected = 0;

void loop() {
    delay(100);

    SendData();
    fNETModule::working = tunnel->IsConnected;

    if (tunnel->IsConnected)
        last_connected = millis();

    if (millis() - last_connected > 60000)
        ESP.restart();
    //Serial.println("loop ok");
}
