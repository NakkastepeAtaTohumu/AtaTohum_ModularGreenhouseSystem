#include <Wire.h>

int addr = 0x01;

long hb_ms;
bool hb = false;

void setup() {
    Serial.begin(115200);
    Wire.begin(25, 26);
    Serial.println("addr: " + addr);

    hb_ms = millis();
}

void loop() {
    if (Serial.available()) {
        String read = Serial.readString();
        if (read[0] == 's') {
            String toSend = read.substring(2);
            Serial.println(("sending data: \"" + String(toSend) + "\""));

            Wire.beginTransmission(addr);
            char buf[64];
            toSend.toCharArray(buf, 64);
            Wire.write(buf);

            int err = Wire.endTransmission();

            delay(50);
            Wire.requestFrom(0x01, 128);

            while (Wire.available())
                Serial.print((char)Wire.read());

            Serial.println();

            if (err == 0)
                Serial.println("ok");
            else
                Serial.println("err: " + String(err));
        }
        else if (read[0] == 'r') {
            Serial.println("requesting data...");
            Wire.requestFrom(addr, 128);

            String read_str = "";
            while (Wire.available()) {
                read_str += (char)Wire.read();;
            }
            Serial.println("received: \"" + read_str + "\"");
        }
        else if (read[0] == 'a') {
            int addr = read.substring(2).toInt();
            Serial.println("addr changed to: " + String(addr));
        }
        else if (read[0] == 'h') {
            Serial.println("enabled hb");
            hb = true;
        }
        else if (read[0] == 'j') {
            Serial.println("disabled hb");
            hb = false;
        }
    }

    if (hb && millis() - hb_ms > 100) {
        hb_ms = millis();

        Wire.beginTransmission(addr);
        Wire.write("ping");
        Wire.endTransmission();
    }
}
