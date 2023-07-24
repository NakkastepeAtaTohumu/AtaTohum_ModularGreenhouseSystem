#include <painlessMesh.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>
#include <WiFi.h>

//#include "fNET.h"
//#include "fNETLib.h"
painlessMesh mesh;

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received %u : %s", from, msg);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u", nodeId);
}

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    mesh.init("Ata_Tohum_MESH", "16777216", 11753);

    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);

    //fNETConnection* c = fNET_ESPNOW::Init();
    //fNETController::Init(c);

    //fNETController::Scan();
    //fNETModule::Init(c);

    //pinMode(2, OUTPUT);
    //digitalWrite(2, LOW);
}

// the loop function runs over and over again until power down or reset
void loop() {
    //delay(100);
    mesh.update();
    delay(1);
    //digitalWrite(5, fNETModule::comm_tunnel->IsConnected);
}
