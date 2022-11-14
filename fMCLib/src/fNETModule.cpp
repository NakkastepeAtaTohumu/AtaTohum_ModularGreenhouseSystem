#include "fNETModule.h"

fNETModuleState fNETModule::State = DISCONNECTED;
bool fNETModule::err = false;
bool fNETModule::fatal_err = false;
bool fNETModule::working = false;

TwoWire fNETModule::I2C(0);

DynamicJsonDocument fNETModule::data(1024);
String fNETModule::ModuleType = "Unknown";

uint8_t fNETModule::I2C_Address;
unsigned long fNETModule::I2C_LastCommsMillis;
bool fNETModule::I2C_IsConnected = false;
bool fNETModule::I2C_Enabled = false;

CircularBuffer<String, 100> fNETModule::I2C_SendBuffer;
CircularBuffer<DynamicJsonDocument*, 8> fNETModule::ReceivedJSONBuffer;
String fNETModule::I2C_ToSend;
int fNETModule::I2C_LastMsgID;
int fNETModule::I2C_TransactionNum;
int fNETModule::I2C_MessageFromMaster_CurrentPacket;

fNETMessage* fNETModule::I2C_CurrentMessage;
fNETMessage* fNETModule::I2C_MessageFromMaster;

int fNETModule::LastQueryID;

fGMSQueryResponder* fNETModule::Responders[32];
int fNETModule::ResponderNum;