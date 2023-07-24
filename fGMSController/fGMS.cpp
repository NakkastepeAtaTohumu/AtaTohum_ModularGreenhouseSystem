#include "fGMS.h"
int fGMS::HygrometerCount = 0;
fGMS::Hygrometer* fGMS::Hygrometers[1024];

int fGMS::HygrometerGroupCount = 0;
fGMS::HygrometerGroup* fGMS::HygrometerGroups[64];

int fGMS::HygrometerModuleCount = 0;
fGMS::HygrometerModule* fGMS::HygrometerModules[128];

int fGMS::ValveModuleCount = 0;
fGMS::ValveModule* fGMS::ValveModules[16];

int fGMS::SensorModuleCount = 0;
fGMS::SensorModule* fGMS::SensorModules[16];

fGMS::Greenhouse fGMS::greenhouse;

bool fGMS::serverEnabled = false;;

fNETConnection* fGMS::fNET = nullptr;

//WiFiUDP fGMS::ntp_udp;
//NTPClient fGMS::ntp_client(fGMS::ntp_udp, "europe.pool.ntp.org", 10800, 10000);

int fGMS::watering_period = 1;
int fGMS::watering_active_period = 0;
bool fGMS::AutomaticWatering = false;
