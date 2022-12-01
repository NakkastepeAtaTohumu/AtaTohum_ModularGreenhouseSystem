#include "fGMSControllerGUI.h"

fGMSControllerMenu::ModuleMenu* fGMSControllerMenu::mm;
fGMSControllerMenu::ModuleMenuOverlay* fGMSControllerMenu::mmo;
fGMSControllerMenu::ModuleMenuManageOverlay* fGMSControllerMenu::mmmo;
fGMSControllerMenu::ModuleSettingsMenu* fGMSControllerMenu::msm;
fGMSControllerMenu::ModuleStatsMenu* fGMSControllerMenu::mstm;
fGMSControllerMenu::SystemConfigMenu* fGMSControllerMenu::scm;
fGMSControllerMenu::I2CConfigsMenu* fGMSControllerMenu::icm;
fGMSControllerMenu::HygrometerModulesMenu* fGMSControllerMenu::hmm;
fGMSControllerMenu::ValveModulesMenu* fGMSControllerMenu::vmm;
fGMSControllerMenu::AlertMenu* fGMSControllerMenu::alertmenu;


NumberInputElement* fGMSControllerMenu::ValveModuleConfigMenu::stateInput;

bool fGMSControllerMenu::ValveModuleConfigMenu::run;

int fGMSControllerMenu::alertmenuid;