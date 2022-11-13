#include "fGMSModule.h"

fGMSModuleState fGMSModule::State = DISCONNECTED;
bool fGMSModule::err = false;
bool fGMSModule::fatal_err = false;
bool fGMSModule::working = false;

TwoWire fGMSModule::I2C(0);

DynamicJsonDocument fGMSModule::data(1024);
String fGMSModule::ModuleType = "Unknown";

uint8_t fGMSModule::I2C_Address;
unsigned long fGMSModule::I2C_LastCommsMillis;
bool fGMSModule::I2C_IsConnected = false;
bool fGMSModule::I2C_Enabled = false;

CircularBuffer<String, 100> fGMSModule::I2C_SendBuffer;
CircularBuffer<DynamicJsonDocument*, 8> fGMSModule::ReceivedJSONBuffer;
String fGMSModule::I2C_ToSend;
int fGMSModule::I2C_LastMsgID;
int fGMSModule::I2C_TransactionNum;
int fGMSModule::I2C_MessageFromMaster_CurrentPacket;

fGMS_I2CMessage* fGMSModule::I2C_CurrentMessage;
fGMS_I2CMessage* fGMSModule::I2C_MessageFromMaster;

int fGMSModule::LastQueryID;

fGMSQueryResponder* fGMSModule::Responders[32];
int fGMSModule::ResponderNum;