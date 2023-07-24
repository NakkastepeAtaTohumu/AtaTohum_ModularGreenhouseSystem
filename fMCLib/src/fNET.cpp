#include "fNET.h"

bool fNETTunnel::TunnelManager::Initialized = false;
int fNETTunnel::TunnelManager::tunnelNum = 0;
fNETTunnel* fNETTunnel::TunnelManager::tunnels[64];

bool IsValidMACAddress(String mac) {
    if (mac.length() < 17)
        return false;

    if (mac[2] != ':' || mac[5] != ':' || mac[8] != ':' || mac[11] != ':' || mac[14] != ':')
        return false;

    byte a = strtol(mac.substring(0, 2).c_str(), NULL, 16);
    byte b = strtol(mac.substring(3, 5).c_str(), NULL, 16);
    byte c = strtol(mac.substring(6, 8).c_str(), NULL, 16);
    byte d = strtol(mac.substring(9, 11).c_str(), NULL, 16);
    byte e = strtol(mac.substring(12, 14).c_str(), NULL, 16);
    byte f = strtol(mac.substring(15, 17).c_str(), NULL, 16);

    return a && b && c && d && e && f;
}

uint8_t* ToMACAddress(String mac) {
    if (mac[2] != ':' || mac[5] != ':' || mac[8] != ':' || mac[11] != ':' || mac[14] != ':')
        return nullptr;

    uint8_t* data = (uint8_t*)malloc(6);

    data[0] = strtol(mac.substring(0, 2).c_str(), NULL, 16);
    data[1] = strtol(mac.substring(3, 5).c_str(), NULL, 16);
    data[2] = strtol(mac.substring(6, 8).c_str(), NULL, 16);
    data[3] = strtol(mac.substring(9, 11).c_str(), NULL, 16);
    data[4] = strtol(mac.substring(12, 14).c_str(), NULL, 16);
    data[5] = strtol(mac.substring(15, 17).c_str(), NULL, 16);

    //Serial.println("HEX Mac: " + String(data[0], 16) + ":" + String(data[1], 16) + ":" + String(data[2], 16) + ":" + String(data[3], 16) + ":" + String(data[4], 16) + ":" + String(data[5], 16) + ":");

    return data;
}

String ToMACString(const uint8_t* mac) {
    String m;

    m += String((int)mac[0], 16) + ":";
    m += String((int)mac[1], 16) + ":";
    m += String((int)mac[2], 16) + ":";
    m += String((int)mac[3], 16) + ":";
    m += String((int)mac[4], 16) + ":";
    m += String((int)mac[5], 16);

    return m;
}

String fNET_ESPNOW::buffer;
fNET_ESPNOW::Connection_t* fNET_ESPNOW::Connection;
//CircularBuffer<DynamicJsonDocument*, 32> fNET_ESPNOW::event_queue;