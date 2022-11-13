#pragma once

#ifndef fGMSMesssages_h
#define fGMSMessages_h

#include "Arduino.h"
#include "fGMSStringFunctions.h"

#include <ArduinoJson.hpp>
#include <ArduinoJson.h>

#ifndef fGMS_USE_CUSTOM_PINS

#define fGMS_SDA 25
#define fGMS_SCK 26

#define fGMS_SDA2 18
#define fGMS_SCK2 19

#define fGMS_PIN_INDICATOR_G 27
#define fGMS_PIN_INDICATOR_Y 33
#define fGMS_PIN_INDICATOR_R 32

#endif

struct fGMS_I2CPacket {
    String data;
    int messageID;
    int packetID;
    String content;

    fGMS_I2CPacket() {
        data = "0#######0###invalid";
    }

    fGMS_I2CPacket(String d, int pkg_id, int msg_id) {
        String msgIDData = PaddedInt(msg_id, 8);
        String packetIDData = PaddedInt(pkg_id, 4);

        String packetData = msgIDData + packetIDData + d;

        messageID = msg_id;
        packetID = pkg_id;
        data = packetData;
        content = d;
    }
};

struct fGMS_I2CMessage {
    String data;
    int messageID;
    fGMS_I2CPacket packets[32];
    int packetCount;

    fGMS_I2CMessage() {

    }

    fGMS_I2CMessage(String d, int msgID) {
        //Serial.println("Convert to msg: " + d);
        data = String(d);
        messageID = msgID;

        int packet_id = 0;
        while (data.length() > 64) {
            String dat = data.substring(0, 64);
            packets[packet_id] = fGMS_I2CPacket(dat, packet_id, msgID);

            data.remove(0, 64);
            packet_id++;

            //Serial.println("Packet: " + String(packet_id));
        }

        while (data.length() < 64)
            data.concat("#");

        packets[packet_id] = fGMS_I2CPacket(data, packet_id, msgID);

        packetCount = packet_id + 1;
    }
};

class fGMSQueryResponder {
public:
    DynamicJsonDocument(*Response)(DynamicJsonDocument);
    String queryType;


    fGMSQueryResponder(String query, DynamicJsonDocument(*response)(DynamicJsonDocument)) {
        queryType = query;
        Response = response;
    }
};

#endif