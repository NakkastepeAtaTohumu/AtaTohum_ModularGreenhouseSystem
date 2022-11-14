#include "fGMSController.h"

DynamicJsonDocument fNETController::data(1024);

TwoWire fNETController::I2C1(0);
TwoWire fNETController::I2C2(1);

fNETController::fNETSlaveConnection* fNETController::Modules[32];
int fNETController::ModuleCount;

long fNETController::I2C_LastScanMs;

fGMSQueryResponder* fNETController::Responders[32];
int fNETController::ResponderNum;