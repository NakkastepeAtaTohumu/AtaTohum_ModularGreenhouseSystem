#include "fNETController.h"

bool fNETController::Module::ping_handler_added;

fNETConnection* fNETController::Connection;
fNETController::Module* fNETController::Modules[32];
int fNETController::ModuleCount;

Event fNETController::OnModuleConnected;
Event fNETController::OnModuleDisconnected;

String fNETController::status_d = "";