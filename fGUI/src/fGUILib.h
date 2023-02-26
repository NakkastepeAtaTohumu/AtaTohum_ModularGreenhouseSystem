#pragma once

#ifndef _fGUI_h
#define _fGUI_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <TFT_eSPI.h>
#include <ESP32Encoder.h>

void DrawTextCentered(TFT_eSprite* d, String text, int x, int y);

class fGUIMenu {
public:
    virtual void Initialize() {

    }

    virtual void Draw() {

    }

    virtual void OnEncoderTickForward() {

    }

    virtual void OnEncoderTickBackward() {

    }

    virtual void OnButtonClicked(int button) {

    }

    virtual void Exit() {

    }

    virtual void Enter() {

    }

    /*

protected:
    void AddSideButton(int i, void(*onClick)()) {

    }
    */

    TFT_eSprite* d = nullptr;
};

class fGUI {
public:
    static void Init() {
        d.init();

        sp->createSprite(d.width(), d.height());

        Serial.println(String(sp->width()) + " " + String(sp->height()));


        for (int i = 0; i < menuNum; i++) 
            menus[i]->Initialize();

        currentMenu->Enter();

        xTaskCreate(displayTask, "fGUI_DisplayTask", 16384, nullptr, 0, nullptr);

        initialized = true;
    }

    static void Init(ESP32Encoder* encoder, int button_pin) {
        e = encoder;
        ebp = button_pin;

        Init();
    }

    static int AddMenu(fGUIMenu* s) {
        s->d = sp;

        menus[menuNum] = s;

        if (openMenuNum == 0)
            OpenMenu(0);

        if (initialized)
            s->Initialize();

        return menuNum++;
    }

    static void OnEncoderTickForward() {
        lastInteractionMS = millis();
        if (millis() - lastInteractionMS > 60000)
            return;

        currentMenu->OnEncoderTickForward();
        if (menuOnTop != nullptr)
            menuOnTop->OnEncoderTickForward();
    }

    static void OnEncoderTickBackward() {
        lastInteractionMS = millis();
        if (millis() - lastInteractionMS > 60000)
            return;

        currentMenu->OnEncoderTickBackward();
        if (menuOnTop != nullptr)
            menuOnTop->OnEncoderTickBackward();
    }

    static void OnButtonClicked(int button) {
        lastInteractionMS = millis();
        if (millis() - lastInteractionMS > 60000)
            return;

        currentMenu->OnButtonClicked(button);
        if (menuOnTop != nullptr)
            menuOnTop->OnButtonClicked(button);
    }

    static void OnBackButtonClicked() {
        lastInteractionMS = millis();
        if (millis() - lastInteractionMS > 60000)
            return;

        /*
        if (menuOnTop != nullptr) {
            CloseMenuOnTop();
            return;
        }*/

        ExitMenu();
    }

    static void ExitMenu() {
        if (openMenuNum <= 1)
            return;

        fGUIMenu* m = currentMenu;

        int menuToOpen = openMenus[openMenuNum - 2];
        openMenus[openMenuNum - 1] = 0;
        openMenuNum -= 2;

        OpenMenu(menuToOpen);

        m->Exit();
    }

    static void OpenMenu(int index) {
        currentMenu = menus[index];
        openMenus[openMenuNum++] = index;

        CurrentOpenMenu = index;

        currentMenu->Enter();

        String s = "Open menus: ";

        for (int i = 0; i < openMenuNum; i++)
            s += String(openMenus[i]) + ", ";

        if (menuOnTop != nullptr)
            s += "On top";

        Serial.println(s);
    }

    static void OpenMenuOnTop(int index) {
        menuOnTop = menus[index];
        menuOnTop->Enter();

        String s = "Open menus: ";

        for (int i = 0; i < openMenuNum; i++)
            s += String(openMenus[i]) + ", ";

        if (menuOnTop != nullptr)
            s += "On top";

        Serial.println(s);
    }

    static void CloseMenuOnTop() {
        if (menuOnTop == nullptr)
            return;

        menuOnTop->Exit();
        menuOnTop = nullptr;
    }

    static void AttachButton(int pin) {
        pinMode(pin, INPUT_PULLUP);

        buttons[numButtons] = pin;
        numButtons++;
    }

    static int CurrentOpenMenu;

private:
    static TFT_eSPI d;
    static TFT_eSprite* sp;
    static fGUIMenu* menus[32];
    static int menuNum;

    static fGUIMenu* currentMenu;
    static fGUIMenu* menuOnTop;
    static int openMenus[16];
    static int openMenuNum;

    static ESP32Encoder* e;
    static int ebp;
    static long eCheckMs;
    static bool encoder_click;

    static int buttons[16];
    static int numButtons;
    static bool buttonDown;

    static long lastInteractionMS;
    static long lastDrawMS;

    static bool initialized;

    static void Display() {
        lastDrawMS = millis();

        if (openMenuNum == 0)
            return;

        if (millis() - lastInteractionMS > 60000) {
            d.fillScreen(TFT_BLACK);
            return;
        }

        //d.fillScreen(TFT_BLACK);

        currentMenu->Draw();

        if(menuOnTop != nullptr)
            menuOnTop->Draw();

        if (openMenuNum > 1)
            DrawBackButton();

        sp->pushSprite(0, 0);
    }

    static void DrawBackButton() {
        sp->fillRect(1, 0, 30, 12, TFT_SKYBLUE);

        sp->setTextColor(TFT_BLACK);
        sp->setTextFont(0);
        sp->setTextSize(1);
        DrawTextCentered(sp, "BACK", 16, 2);
    }

    static void encoder_update() {
        int d = e->getCount();
        e->clearCount();

        if (d > 0)
            OnEncoderTickForward();
        else if (d < 0)
            OnEncoderTickBackward();

        bool read = !digitalRead(ebp);

        if (read && !encoder_click)
            OnButtonClicked(-1);

        encoder_click = read;
    }

    static void btn_update() {
        for (int i = 0; i < numButtons; i++) {
            if (!digitalRead(buttons[i])) {
                if (!buttonDown) {
                    if (i != 0)
                        OnButtonClicked(i);
                    else
                        OnBackButtonClicked();
                }

                buttonDown = true;
                return;
            }
        }

        buttonDown = false;
    }

    static void io_update() {
        if (e != nullptr)
            encoder_update();

        btn_update();
    }

    static void displayTask(void* param)
    {
        while (true) {
            io_update();

            if(millis() - lastDrawMS > 200)
                Display();

            delay(5);
        }
    }
};

#endif