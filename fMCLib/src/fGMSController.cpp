#include "fGMSController.h"

DynamicJsonDocument fGMSController::data(1024);

TwoWire fGMSController::I2C1(0);
TwoWire fGMSController::I2C2(1);

fGMSController::fGMSModuleData* fGMSController::Modules[32];
int fGMSController::ModuleCount;

long fGMSController::I2C_LastScanMs;

fGMSQueryResponder* fGMSController::Responders[32];
int fGMSController::ResponderNum;