#include "fGMS.h"

DynamicJsonDocument fGMS::data(4096);

int fGMS::HygrometerCount = 0;
fGMS::Hygrometer* fGMS::Hygrometers[1024];

int fGMS::HygrometerGroupCount = 0;
fGMS::HygrometerGroup* fGMS::HygrometerGroups[16];

int fGMS::HygrometerModuleCount = 0;
fGMS::HygrometerModule* fGMS::HygrometerModules[128];

int fGMS::ValveModuleCount = 0;
fGMS::ValveModule* fGMS::ValveModules[128];

fNETConnection* fGMS::fNET = nullptr;
