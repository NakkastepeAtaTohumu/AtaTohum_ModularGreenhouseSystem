#include "fNETModule.h"

fNETModuleState fNETModule::State = DISCONNECTED;
bool fNETModule::err = false;
bool fNETModule::fatal_err = false;
bool fNETModule::working = false;

String fNETModule::ControllerMAC, fNETModule::ConnectionMAC;
long fNETModule::LastConnectionAttemptMS, fNETModule::LastOKMS;

fNETConnection* fNETModule::Connection;
fNETTunnel* fNETModule::comm_tunnel;
DynamicJsonDocument fNETModule::data(1024);