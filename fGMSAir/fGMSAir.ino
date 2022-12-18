#include <Adafruit_SHT31.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_I2CDevice.h>

#include "soc/rtc_cntl_reg.h"

#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

#include "fNETLib.h"

TwoWire auxI2C(1);
HardwareSerial CO2SensorUART(1);

Adafruit_SHT31 airSensor(&auxI2C);

float sendInterval = 0;
bool fanOn = false;

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
        ESP_LOGE("fGMS AirSensor CO2", "Checksum mismatch!");
        return NAN;
    }

    float ppm = buffer[2] * 256 + buffer[3];
    return ppm;
}

void SetFan(bool on) {
    ESP_LOGD("fGMS AirSensor", "Set fan %s.", (on ? "on" : "off"));
    fanOn = on;

    digitalWrite(13, on);
}

void setup() {
    Serial.begin(115200);
    ESP_LOGE("fGMS AirSensor", "Air Sensor starting");

    c = fNETModule::Init();
    fNETModule::data["ModuleType"] = "AirSensor";
    fNETModule::data["name"] = "AIR A"; //TODO Unique to moduele!
    

    bool i2cOK = auxI2C.begin(fNET_SDA2, fNET_SCK2);
    CO2SensorUART.begin(9600, SERIAL_8N1, 17, 16);

    if (!i2cOK)
    {
        ESP_LOGE("fGMS Aux I2C", "I2C failed to begin!");
        fNETModule::SetFatalErrorState();
    }

    if (!airSensor.begin()) {
        ESP_LOGE("fGMS AirSensor SHT31", "Can't connect to SHT31!");
        fNETModule::SetFatalErrorState();
    }
    else
        ESP_LOGI("fGMS AirSensor SHT31", "Connected to SHT31.");

    ESP_LOGI("fGMS AirSensor SHT31", "Testing sensor.");

    float temperature, humidity;
    airSensor.readBoth(&temperature, &humidity);

    ESP_LOGI("fGMS AirSensor SHT31", "Read temperature : %s C humidity : %s %.", String(temperature), String(humidity));

    if (isnan(temperature) ||isnan(humidity))
    {
        ESP_LOGE("fGMS AirSensor SHT31", "Sensor error!");
        fNETModule::SetFatalErrorState();
    }
    else
        ESP_LOGI("fGMS AirSensor SHT31", "Sensor OK.");

    ESP_LOGI("fGMS AirSensor CO2", "Testing sensor.");
    float ppm = ReadCO2FromSensor();
    ESP_LOGI("fGMS AirSensor CO2", "Read ppm : %s", String(ppm));

    if (isnan(ppm))
    {
        ESP_LOGE("fGMS AirSensor CO2", "Sensor error!");
        fNETModule::SetFatalErrorState();


        int reg = READ_PERI_REG(RTC_CNTL_RESET_CAUSE_APPCPU);

        ESP_LOGE("fGMS AirSensor CO2", "Reset cause: %d", reg);

        delay(10000);
        ESP.restart();
    }
    else {
        ESP_LOGD("fGMS AirSensor CO2", "Checksum match.");
        ESP_LOGI("fGMS AirSensor CO2", "Sensor OK.");
    }

    pinMode(13, OUTPUT);

    SetFan(true);
    delay(1000);
    SetFan(false);

    c->AddQueryResponder("setValueInterval", [](DynamicJsonDocument q) {
        sendInterval = q["interval"];

        ESP_LOGD("fGMS AirSensor", "Set send interval : %f ms.", sendInterval);

        DynamicJsonDocument resp(256);
        resp["interval"] = String(sendInterval);
        return resp;
        });

    c->AddQueryResponder("setFan", [](DynamicJsonDocument q) {
        SetFan(q["enabled"]);

        DynamicJsonDocument resp(256);
        resp["enabled"] = String(fanOn);
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
