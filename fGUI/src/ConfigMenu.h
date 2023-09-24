#pragma once

#ifndef ConfigMenu_h
#define ConfigMenu_h

class Menu;

class ConfigMenu : public Menu {
public:
    void Enter() {
        d->fillScreen(TFT_WHITE);
    }

    void Draw() {
        
    }
};
#endif