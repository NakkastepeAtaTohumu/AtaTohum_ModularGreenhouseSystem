#include <Adafruit_SHT31.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

#include "fNETLib.h"

TwoWire auxI2C(1);
HardwareSerial CO2SensorUART(1);

Adafruit_SHT31 airSensor(&auxI2C);

float sendInterval = 0;

fNETConnection* c;

byte CO2UART_GetCheckSum(byte* packet)
{
    char i, checksum;
    for (i = 1; i < 8; i++)
        checksum += packet[i];

    checksum = 0xff - checksum;
    checksum += 1;
    return checksum;
}

float ReadCO2FromSensor() {
    while (CO2SensorUART.available())
        CO2SensorUART.read();

    char buf[] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
    CO2SensorUART.write(buf, 9);

    byte buffer[9];
    CO2SensorUART.readBytes(buffer, 9);

    String read;
    for (int i = 0; i < 9; i++)
        read += "0x" + String(buffer[i], HEX) + " ";

    byte checksum = CO2UART_GetCheckSum(buffer);

    if (buffer[8] != checksum) {
        Serial.println("[fGMS AirSensor CO2] Checksum mismatch!");
        return NAN;
    }

    float ppm = buffer[2] * 256 + buffer[3];
    return ppm;
}

void setup() {
    Serial.begin(115200);
    Serial.println("[fGMS HygroCtl] Air Sensor starting");

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "AirSensor";
    fNETModule::data["name"] = "AIR A"; //TODO Unique to moduele!
    fNETModule::data["channels"] = 4;

    auxI2C.begin(fNET_SDA2, fNET_SCK2);
    airSensor.begin();
    CO2SensorUART.begin(9600, SERIAL_8N1, 17, 16);

    if (!airSensor.begin()) {
        Serial.println("[fGMS AirSensor SHT31] Can't connect to SHT31!");
        fNETModule::SetFatalErrorState();
    }
    else
        Serial.println("[fGMS AirSensor SHT31] Connected to SHT31.");

    Serial.println("[fGMS AirSensor SHT31] Testing sensor.");

    float temperature, humidity;
    airSensor.readBoth(&temperature, &humidity);

    Serial.println("[fGMS AirSensor SHT31] Read temperature: " + String(temperature) + " C humidity: " + String(humidity) + "%.");

    if (isnan(temperature) ||isnan(humidity))
    {
        Serial.println("[fGMS AirSensor SHT31] Sensor error!");
        fNETModule::SetFatalErrorState();
    }
    else
        Serial.println("[fGMS AirSensor SHT31] Sensor OK.");

    Serial.println("[fGMS AirSensor CO2] Testing sensor.");

    float ppm = ReadCO2FromSensor();
    Serial.println("[fGMS AirSensor CO2] Read ppm: " + String(ppm));

    if (isnan(ppm))
    {
        Serial.println("[fGMS AirSensor CO2] Sensor error!");
        fNETModule::SetFatalErrorState();
    }
    else {
        Serial.println("[fGMS AirSensor CO2] Checksum match.");
        Serial.println("[fGMS AirSensor CO2] Sensor OK.");
    }

    c->AddQueryResponder("setValueInterval", [](DynamicJsonDocument q) {
        sendInterval = q["interval"];

        Serial.println("[fGMS AirSensor] Set send interval: " + String(sendInterval) + " ms.");

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
