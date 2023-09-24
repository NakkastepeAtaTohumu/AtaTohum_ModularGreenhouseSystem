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

class Menu {
public:
    virtual void Initialize() {

    }

    virtual void Deinitialize() {

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
    bool Initialized = false;
    TFT_eSprite* d = nullptr;
};

class fGUI {
public:
    static void Init() {
        d.init();

        //Serial.println("Sprite Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        
        InitSprite();
        Serial.println(String(sp->width()) + " " + String(sp->height()));

        /*for (int i = 0; i < menuNum; i++) {
            //Serial.println("Heap left: " + String(ESP.getFreeHeap()) + " Init menu: " + String(i));
            //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

            //menus[i]->Initialize();
        }*/

        OpenMenu(0);

        //Serial.println("Enter Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

        xTaskCreate(displayTask, "fGUI_DisplayTask", 4096, nullptr, 0, nullptr);

        //Serial.println("OK Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

        initialized = true;
    }

    static void InitSprite() {
        Serial.println("Init sprite");
        sp->setColorDepth(8);
        sp->createSprite(d.width(), d.height());

        SpriteInitialized = true;
    }

    static void DeleteSprite() {
        Serial.println("Deinit sprite");
        sp->deleteSprite();

        SpriteInitialized = false;
    }

    static void Init(ESP32Encoder* encoder, int button_pin) {
        e = encoder;
        ebp = button_pin;

        Init();
    }

    static int AddMenu(Menu* s) {
        s->d = sp;

        menus[menuNum] = s;
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

        Menu* m = currentMenu;

        int menuToOpen = openMenus[openMenuNum - 2];
        openMenus[openMenuNum - 1] = 0;
        openMenuNum -= 2;

        OpenMenu(menuToOpen);

        m->Exit();

        if (m->Initialized) {
            m->Deinitialize();
            m->Initialized = false;
        }
    }

    static void OpenMenu(int index) {
        Serial.println("open menu " + String(index));
        currentMenu = menus[index];
        openMenus[openMenuNum++] = index;

        CurrentOpenMenu = index;

        if (!currentMenu->Initialized) {
            currentMenu->Initialize();
            currentMenu->Initialized = true;
        }

        currentMenu->Enter();
    }

    static void OpenMenuOnTop(int index) {
        menuOnTop = menus[index];

        if (!menuOnTop->Initialized) {
            menuOnTop->Initialize();
            menuOnTop->Initialized = true;
        }

        menuOnTop->Enter();
    }

    static void CloseMenuOnTop() {
        if (menuOnTop == nullptr)
            return;

        menuOnTop->Exit();

        if (menuOnTop->Initialized) {
            menuOnTop->Deinitialize();
            menuOnTop->Initialized = false;
        }

        menuOnTop = nullptr;
    }

    static void AttachButton(int pin) {
        pinMode(pin, INPUT_PULLUP);

        buttons[numButtons] = pin;
        numButtons++;
    }
    static void ResetScreensaver() {
        lastInteractionMS = millis();
    }

    static void Screensaver() {
        lastInteractionMS = 0;
    }

    static bool IsScreensaverActive() {
        return millis() - lastInteractionMS > 60000;
    }

    static int CurrentOpenMenu;
    static TFT_eSprite* sp;

    static String DrawInstructions;

    static bool SpriteInitialized;

private:
    static TFT_eSPI d;
    static Menu* menus[32];
    static int menuNum;

    static Menu* currentMenu;
    static Menu* menuOnTop;
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

            if (SpriteInitialized)
                DeleteSprite();

            return;
        }

        if (!SpriteInitialized)
            InitSprite();

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