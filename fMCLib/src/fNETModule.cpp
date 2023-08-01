#include "fNETModule.h"

fNETModuleState fNETModule::State = DISCONNECTED;
bool fNETModule::err = false;
bool fNETModule::fatal_err = false;
bool fNETModule::working = false;
bool fNETModule::config_sent = false;

String fNETModule::ControllerMAC, fNETModule::ConnectionMAC;
long fNETModule::LastConnectionAttemptMS, fNETModule::LastOKMS, fNETModule::LastConfigSentMS;

fNETConnection* fNETModule::Connection;
fNETTunnel* fNETModule::comm_tunnel;
DynamicJsonDocument fNETModule::data(1024);