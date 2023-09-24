#include "fGMSControllerGUI.h"

fGMSControllerMenu::ModuleMenu* fGMSControllerMenu::mm;
fGMSControllerMenu::ModuleMenuOverlay* fGMSControllerMenu::mmo;
fGMSControllerMenu::ModuleMenuManageOverlay* fGMSControllerMenu::mmmo;
fGMSControllerMenu::ModuleSettingsMenu* fGMSControllerMenu::msm;
fGMSControllerMenu::ModuleStatsMenu* fGMSControllerMenu::mstm;
fGMSControllerMenu::SystemConfigMenu* fGMSControllerMenu::scm;
ElementMenu* fGMSControllerMenu::icm;
fGMSControllerMenu::HygrometerModulesMenu* fGMSControllerMenu::hmm;
fGMSControllerMenu::ValveModulesMenu* fGMSControllerMenu::vmm;
fGMSControllerMenu::ValveModuleConfigMenu* fGMSControllerMenu::vmcm;
fGMSControllerMenu::AlertMenu* fGMSControllerMenu::alertmenu;
fGMSControllerMenu::GreenhouseSizeConfigMenu* fGMSControllerMenu::gscm;
fGMSControllerMenu::HygrometersConfigMenu* fGMSControllerMenu::hscm;
fGMSControllerMenu::GreenhouseModuleConfigTabsMenu* fGMSControllerMenu::gmctm;
fGMSControllerMenu::HygrometerPositionConfigMenu* fGMSControllerMenu::hpco;
fGMSControllerMenu::HygrometerConfigMenu* fGMSControllerMenu::hcm;
fGMSControllerMenu::MoistureMonitorMenu* fGMSControllerMenu::mmm;
fGMSControllerMenu::MonitorTabsMenu* fGMSControllerMenu::mtm;
fGMSControllerMenu::SensorModulesMenu* fGMSControllerMenu::smm;
fGMSControllerMenu::SensorModuleConfigMenu* fGMSControllerMenu::smcm;
fGMSControllerMenu::HygrometerModuleConfigMenu* fGMSControllerMenu::hmcm;

NumberInputElement* fGMSControllerMenu::ValveModuleConfigMenu::stateInput;

bool fGMSControllerMenu::ValveModuleConfigMenu::run;

int fGMSControllerMenu::alertmenuid;