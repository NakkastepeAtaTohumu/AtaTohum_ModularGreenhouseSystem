#include "fNET.h"

bool fNETTunnel::TunnelManager::Initialized = false;
int fNETTunnel::TunnelManager::tunnelNum = 0;
fNETTunnel* fNETTunnel::TunnelManager::tunnels[64];