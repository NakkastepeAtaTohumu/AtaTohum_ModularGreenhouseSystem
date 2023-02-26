/*
 Name:		fGUI.cpp
 Created:	11/20/2022 10:55:25 AM
 Author:	Alp D
 Editor:	http://www.visualmicro.com
*/

#include "fGUILib.h"

TFT_eSPI fGUI::d;
TFT_eSprite* fGUI::sp = new TFT_eSprite(&fGUI::d);
fGUIMenu* fGUI::menus[32];
int fGUI::menuNum;

fGUIMenu* fGUI::currentMenu = nullptr;
fGUIMenu* fGUI::menuOnTop = nullptr;
int fGUI::openMenus[16];
int fGUI::openMenuNum;

ESP32Encoder* fGUI::e = nullptr;
int fGUI::ebp;
bool fGUI::encoder_click;

int fGUI::buttons[16];
int fGUI::numButtons;
bool fGUI::buttonDown;

long fGUI::lastInteractionMS;
long fGUI::lastDrawMS;

int fGUI::CurrentOpenMenu;

bool fGUI::initialized;

void DrawTextCentered(TFT_eSprite* d, String text, int x, int y) {
    int w = d->textWidth(text);
    d->setCursor(x - w / 2, y);
    d->print(text);
}

