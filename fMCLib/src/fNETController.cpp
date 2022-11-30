#include "fNETController.h"

DynamicJsonDocument fNETController::data(1024);

TwoWire fNETController::I2C1(0);
TwoWire fNETController::I2C2(1);

bool fNETController::I2C_IsEnabled;
float fNETController::I2C_freq;

fNETController::fNETSlaveConnection* fNETController::Modules[32];
int fNETController::ModuleCount;

long fNETController::I2C_LastScanMs;

String fNETController::status_d = "";

fNETController::ControllerConnection* fNETController::Connection;