#include "fNETController.h"
fNETConnection* fNETController::Connection;
fNETController::Module* fNETController::Modules[32];
int fNETController::ModuleCount;

String fNETController::status_d = "";