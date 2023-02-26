#pragma once

#ifndef fGUIConfigMenu_h
#define fGUIConfigMenu_h

class fGUIMenu;

class fGUIConfigMenu : public fGUIMenu {
public:
    void Enter() {
        d->fillScreen(TFT_WHITE);
    }

    void Draw() {
        
    }
};
#endif