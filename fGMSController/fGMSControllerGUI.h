#include "fGUILib.h"
#include "fGUIElementMenu.h"
#include "fNETController.h"
#include "fGMS.h"

class fGMSControllerMenu {
private:
    class TitleMenu : public fGUIElementMenu {
    public:
        void InitializeElements() {
            AddElement(new TextElement("fGMS", width / 2, 20, 4, 1, TFT_GOLD));
            AddElement(new TextElement("Greenhouse System", width / 2, 32, 0, 1, TFT_GOLD));

            //AddElement(new Button("Monitor", width / 2, 74, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {fGUI::OpenMenu(1); }));
            //AddElement(new Button("Config", width / 2, 107, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new Button("Debug", width / 2, 140, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

            AddElement(new Button("M", 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() { fGUI::OpenMenu(20); }, "Monitor"));
            AddElement(new Button("C", width / 2, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() { fGUI::OpenMenu(10); }, "Config"));
            AddElement(new Button("L", width - 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {}, "Logs"));
            AddElement(new Button("D", width - 24, 112, 32, 32, 2, 1, TFT_BLACK, TFT_DARKGREY, []() { fGUI::OpenMenu(1); }, "Debug"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }
    };

    class ConfigMenu : public fGUIElementMenu {
    public:
        void InitializeElements() {
            AddElement(new Button("Mdl", 24, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() { fGUI::OpenMenu(15); }, "Modules"));
            AddElement(new Button("Hyg", width / 2, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() { fGUI::OpenMenu(16); }, "Hygrometers"));
            //AddElement(new Button("HYG", width - 24, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() { fGUI::OpenMenu(13); }, "Hygrometers"));
            AddElement(new Button("GHS", 24, 112, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() { fGUI::OpenMenu(14); }, "Greenhouse Config"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_WHITE));

            bg_color = TFT_BLACK;
        }
    };

    class DebugMenu : public fGUIElementMenu {
    public:
        void InitializeElements() {
            AddElement(new Button("M", 24, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() { fGUI::OpenMenu(2); }, "Modules"));
            AddElement(new Button("SC", width / 2, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() {fGUI::OpenMenu(6); }, "System Config"));
            AddElement(new Button("I2C", width - 24, 40, 32, 32, 2, 1, TFT_BLACK, TFT_LIGHTGREY, []() {fGUI::OpenMenu(8); }, "I2C Config"));
            //AddElement(new Button("L", width - 24, 32, 32, 32, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {fGUI::OpenMenu(1); }, "Logs"));
            //AddElement(new Button("D", width - 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_DARKGREY, []() {fGUI::OpenMenu(1); }, "Debug"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_WHITE));

            bg_color = TFT_BLACK;
        }
    };

    class ModuleListElement : public ListElement {
    public:
        ModuleListElement(fNETController::fNETSlaveConnection* md) {
            mdl = md;
            isSelectable = true;

            Update();
        }

        int Draw(int x, int y) override {
            d->setTextFont(0);

            //uint16_t bgc = d->color24to16(0x303030);
            uint16_t bgc = isOnline ? d->color24to16(0x505050) : TFT_RED;// d->color24to16(0x303030);


            if (isHighlighted)
                d->setTextColor(TFT_WHITE, TFT_DARKGREY, true);
            else if (!isSelected && isOnline)
                d->setTextColor(TFT_WHITE, TFT_GREEN, true);
            else
                d->setTextColor(TFT_WHITE, bgc, true);

            d->setTextSize(1);
            //DrawTextCentered(d, t, x, y - (d->fontHeight() / 2));
            d->setCursor(x, y);

            if (!isSelected) {
                d->setTextFont(2);
                d->print(name);// +" " + String(id) + String(isOnline ? " OK" : " DC"));
                return d->fontHeight() + 2;
            }

            d->fillRect(x, y, 80, 116, bgc);
            d->print("I: ");

            d->setTextColor(TFT_WHITE, bgc, true);
            d->setCursor(x + 16, y + 10);
            d->println("ID: " + String(id));
            d->setCursor(x + 16, y + 20);
            d->println("Full mac:");
            d->setCursor(x + 16, y + 30);
            d->println(" " + mac1);
            d->setCursor(x + 16, y + 40);
            d->println(" " + mac2);

            d->setCursor(x + 16, y + 54);
            d->println("I2C Addr:");
            d->setCursor(x + 16, y + 62);
            d->println(" " + String(i2cAddr));

            d->setCursor(x + 16, y + 76);
            d->println("Type:");
            d->setCursor(x + 16, y + 84);
            d->println(" " + type);

            d->setCursor(x + 16, y + 98);
            d->println("Buffer:");
            d->setCursor(x + 16, y + 106);
            d->println(" " + String(queueC));

            return 118;
        }

        void OnSelect() override {
            fGUI::OpenMenuOnTop(3);

            mm->configured = mdl;
            mm->configured_element = this;
        }

        void OnDeselect() override {
            fGUI::CloseMenuOnTop();
        }

        void Update() {
            mac1 = mdl->MAC_Address.substring(0, 8);
            mac2 = mdl->MAC_Address.substring(9);

            type = mdl->Config["ModuleType"].as<String>();
            name = mdl->Config["name"].as<String>();

            if (name == "null")
                name = "UNKNOWN";

            if (type == "null")
                type = "UNKNOWN";

            id = fNETController::GetModuleIndex(mdl);

            isOnline = mdl->isOnline;
            i2cAddr = mdl->i2c_address;

            queueC = mdl->QueuedMessageCount();

            if (!mdl->valid)
                mm->RemoveModule();

            if (millis() > 10000) {
                if (isOnline && !wasOnline)
                {
                    alertmenu->updateMessage(type + " " + String(id), "Module connected.");
                    fGUI::OpenMenu(alertmenuid);
                }

                if (!isOnline && wasOnline) {
                    alertmenu->updateMessage(type + " " + String(id), "Module disconnected!");
                    fGUI::OpenMenu(alertmenuid);
                }
            }

            wasOnline = isOnline;
        }

        fNETController::fNETSlaveConnection* mdl;

        String mac1;
        String mac2;
        String type;
        String name;
        int id;

        int i2cAddr;

        int queueC;

    private:
        bool isOnline;
        bool wasOnline;
    };

    class ModuleMenu : public ListMenu {
    public:
        void AddModule(fNETController::fNETSlaveConnection* c) {
            if (!modulePresent)
                RemoveElement(0);

            modulePresent = true;

            for (int i = 0; i < numElements; i++)
                if (((ModuleListElement*)elements[i])->mdl == c)
                    return;

            AddElement(new ModuleListElement(c));

            if (addedModules) {
                alertmenu->updateMessage("New module added", c->MAC_Address);
                //fGUI::OpenMenu(alertmenuid);
            }
        }

        void RemoveModule() {
            RemoveElement(highlightedElementIndex);
        }

        void InitializeElements() {
            uint16_t c = d->color24to16(0x303030);

            AddElement(new ListTextElement("No modules.", 2, 1, TFT_WHITE, TFT_BLACK));

            start_x = 16;
        }

        void Exit() override {
            fGUI::CloseMenuOnTop();
        }

        void Enter() override {
            ListMenu::Enter();

            if (elements[highlightedElementIndex]->isSelected)
                fGUI::OpenMenuOnTop(3);

            if (numElements == 0) {
                AddElement(new ListTextElement("No modules.", 2, 1, TFT_WHITE, TFT_BLACK));
                modulePresent = false;
            }
        }

        void UpdateElements() {
            if (!modulePresent)
                return;

            for (int i = 0; i < numElements; i++)
                ((ModuleListElement*)elements[i])->Update();

            addedModules = true;
        }

        fNETController::fNETSlaveConnection* configured;
        ModuleListElement* configured_element;

    private:
        bool addedModules = false;
        bool modulePresent = false;
    };

    class ModuleMenuOverlay : public fGUIElementMenu {
    public:
        void InitializeElements() {
            AddElement(new PhysicalButton("I", 1, width - 12, 30, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenu(7); }));
            AddElement(new PhysicalButton("A", 2, width - 12, 65, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenu(5); }));
            //AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

            bg = false;
        }

        void Exit() {
            //fGUI::ExitMenu();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (fGUI::CurrentOpenMenu != 2)
                fGUI::CloseMenuOnTop();
        }
    };

    class ModuleMenuManageOverlay : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            AddElement(new BoxElement(width / 2, 32, 48, 24, d->color24to16(0x202020)));
            AddElement(new TextElement("Manage", width / 2, 32, 2, 1, TFT_WHITE));

            AddElement(new BoxElement(width / 2, height / 2, 96, 48, d->color24to16(0x202020)));
            AddElement(new Button("I", width / 2 - 24, height / 2, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {
                fGUI::ExitMenu();
                fGUI::CloseMenuOnTop();
                fGUI::OpenMenu(7);
                }, "Info"));

            AddElement(new Button("A", width / 2 + 24, height / 2, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {
                fGUI::ExitMenu();
                fGUI::CloseMenuOnTop();
                fGUI::OpenMenu(5);
                }, "Actions"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));

            bg = false;
        }
    };

    class ModuleSettingsMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("Module: MDLMDLM X", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);

            i2cAddrField = new NumberInputElement("I2C Addr", width / 2, 70, 0, 1, TFT_WHITE, TFT_DARKGREY);


            AddElement(new TextElement("Actions", width / 2, 12, 2, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);

            AddElement(i2cAddrField);
            AddElement(new Button("Set I2C Addr", width / 2, 90, 76, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {
                mm->configured->SetI2CAddr(msm->i2cAddrField->Value);
                }, ""));

            //AddElement(new Button("Edit Config", width / 2, 110, 76, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {}, ""));
            AddElement(new Button("REMOVE", width / 2, 148, 76, 12, 0, 1, TFT_BLACK, TFT_RED, []() {
                if (fNETController::I2C_IsEnabled && mm->configured->isOnline)
                {
                    alertmenu->updateMessage("Can't remove", "Disable I2C.");
                    fGUI::OpenMenu(alertmenuid);
                    return;
                }
                mm->RemoveModule();
                mm->configured->Remove();

                fNETController::Save();
                fGUI::ExitMenu(); }, ""));


            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            nameD->t = "Module: " + mm->configured_element->type + " " + String(mm->configured_element->id);
            macD->t = mm->configured->MAC_Address;
            i2cAddrField->Value = mm->configured->i2c_address;

            stateD->t = mm->configured->isOnline ? "ONLINE" : "OFFLINE";
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        NumberInputElement* i2cAddrField;
    };

    class ModuleStatsMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("Module: MDLMDLM X", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            avgPollD = new UncenteredTextElement("Average Poll: MSMSMS", 6, 70, 0, 1, TFT_WHITE);
            avgTransD = new UncenteredTextElement("Average Transaction: MSMSMS", 6, 80, 0, 1, TFT_WHITE);
            totalTransD = new UncenteredTextElement("Average Transaction: MSMSMS", 6, 100, 0, 1, TFT_WHITE);
            failTransD = new UncenteredTextElement("Average Transaction: MSMSMS", 6, 110, 0, 1, TFT_WHITE);

            AddElement(new TextElement("Stats", width / 2, 12, 2, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(avgPollD);
            AddElement(avgTransD);
            AddElement(totalTransD);
            AddElement(failTransD);

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            Update();
        }

        void Update() {
            nameD->t = "Module: " + mm->configured_element->type + " " + String(mm->configured_element->id);
            macD->t = mm->configured->MAC_Address;

            stateD->t = mm->configured->isOnline ? "ONLINE" : "OFFLINE";

            avgPollD->t = "Avg poll: " + String(mm->configured->AveragePollTimeMillis) + " ms";
            avgTransD->t = "Avg trans: " + String(mm->configured->AverageTransactionTimeMillis) + " ms";
            totalTransD->t = "Total trans: " + String(mm->configured->TotalTransactions);
            failTransD->t = "Failed trans: " + String(mm->configured->FailedTransactions);
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - lastUpdateMillis > 100) {
                Update();
                lastUpdateMillis = millis();
            }
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        UncenteredTextElement* avgPollD;
        UncenteredTextElement* avgTransD;
        UncenteredTextElement* totalTransD;
        UncenteredTextElement* failTransD;

        long lastUpdateMillis = 0;
    };

    class AlertMenu : public fGUIElementMenu {
    public:
        AlertMenu(String m) {
            msg = m;
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            opened_millis = millis();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - opened_millis > 2500)
                fGUI::ExitMenu();
        }

        void InitializeElements() override {
            e = new TextElement(msg, width / 2, height / 2 - 9, 2, 1, TFT_WHITE);
            e_small = new TextElement(msg, width / 2, height / 2 + 5, 0, 1, TFT_WHITE);

            AddElement(new BoxElement(width / 2, height / 2, width - 8, 36, TFT_DARKGREY));
            AddElement(new BoxElement(width / 2, height / 2, width - 12, 32, TFT_BLACK));
            AddElement(e);
            AddElement(e_small);

            bg = false;
            bg_color = TFT_TRANSPARENT;
        }

        void updateMessage(String m, String m_s) {
            e->t = m;
            e_small->t = m_s;
        }

        String msg;
        TextElement* e;
        TextElement* e_small;

        long opened_millis;
    };

    class SystemConfigMenu : public ListMenu {
    public:
        void InitializeElements() override {
            AddElement(new ListTextElement("", 0, 1, TFT_WHITE, TFT_TRANSPARENT));

            start_x = 16;
        }

        void SetConfig(DynamicJsonDocument conf) {
            String json_str;
            serializeJsonPretty(conf, json_str);

            if (json_str == prev_config)
                return;

            prev_config = json_str;

            for (int i = 0; i < numElements; i++)
                delete elements[i];

            numElements = 0;

            while (json_str.length() > 0) {
                int index = json_str.indexOf('\n');

                if (index == -1)
                    index = json_str.length();

                int rm_index = index + 1;

                if (index > 16) {
                    index = 16;
                    rm_index = 16;
                }

                String sec = json_str.substring(0, index);
                json_str.remove(0, rm_index);

                AddElement(new ListTextElement(sec, 0, 1, TFT_WHITE, TFT_BLACK));
            }
        }

        String prev_config;
    };

    class I2CConfigsMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            stateD = new TextElement("State: " + String(fNETController::I2C_IsEnabled ? "ENABLED" : "DISABLED"), width / 2, 40, 0, 1, TFT_WHITE);
            freqD = new TextElement("Frequency: " + String(fNETController::I2C_freq), width / 2, 50, 0, 1, TFT_WHITE);

            freqField = new NumberInputElement("Set freq:", width / 2, 70, 0, 1, TFT_WHITE, TFT_DARKGREY);
            freqField->Value = fNETController::I2C_freq / 100000;


            AddElement(new TextElement("I2C Config", width / 2, 12, 2, 1, TFT_WHITE));
            AddElement(freqD);
            AddElement(stateD);

            AddElement(freqField);
            AddElement(new Button("Set frequency", width / 2, 90, 80, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {
                fNETController::SetI2CState(fNETController::I2C_IsEnabled, icm->freqField->Value * 100000);
                }, ""));

            AddElement(new Button("Toggle I2C", width / 2, 110, 80, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {
                fNETController::SetI2CState(!fNETController::I2C_IsEnabled, fNETController::I2C_freq);
                }, ""));


            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }


        void Enter() override {
            fGUIElementMenu::Enter();

            freqD->t = "Frequency: " + String(fNETController::I2C_freq);
            stateD->t = "State: " + String(fNETController::I2C_IsEnabled ? "ENABLED" : "DISABLED");
        }
        TextElement* freqD;
        TextElement* stateD;
        NumberInputElement* freqField;
    };

    class HygrometerModuleListElement : public ListElement {
    public:
        HygrometerModuleListElement(fGMS::HygrometerModule* md) {
            mdl = md;
            isSelectable = false;

            Update();
        }

        int Draw(int x, int y) override {
            d->setTextFont(2);

            //uint16_t bgc = d->color24to16(0x303030);
            uint16_t bgc = mdl->ok ? d->color24to16(0x505050) : TFT_RED;


            if (isHighlighted)
                d->setTextColor(TFT_WHITE, TFT_DARKGREY, true);
            else if (!isSelected && mdl->ok)
                d->setTextColor(TFT_WHITE, TFT_GREEN, true);
            else
                d->setTextColor(TFT_WHITE, bgc, true);

            d->setTextSize(1);
            //DrawTextCentered(d, t, x, y - (d->fontHeight() / 2));
            d->setCursor(x, y);

            d->print(name);// +": " + (mdl->ok ? "OK" : "ERROR"));
            return d->fontHeight() + 2;
        }

        void OnHighlight() override {
            hmm->configured = mdl;
            hmm->configured_element = this;
        }

        void OnClick() override {
            fGUI::CloseMenuOnTop();
            fGUI::OpenMenu(11);
        }

        void Update() {
            name = mdl->mdl->Config["name"].as<String>();
        }

        fGMS::HygrometerModule* mdl;

        String name;
    };

    class ValveModuleListElement : public ListElement {
    public:
        ValveModuleListElement(fGMS::ValveModule* md) {
            mdl = md;
            isSelectable = false;

            Update();
        }

        int Draw(int x, int y) override {
            d->setTextFont(2);

            //uint16_t bgc = d->color24to16(0x303030);
            uint16_t bgc = mdl->ok ? d->color24to16(0x505050) : TFT_RED;


            if (isHighlighted)
                d->setTextColor(TFT_WHITE, TFT_DARKGREY, true);
            else if (!isSelected && mdl->ok)
                d->setTextColor(TFT_WHITE, TFT_GREEN, true);
            else
                d->setTextColor(TFT_WHITE, bgc, true);

            d->setTextSize(1);
            //DrawTextCentered(d, t, x, y - (d->fontHeight() / 2));
            d->setCursor(x, y);

            d->print(name);// +": " + (mdl->ok ? "OK" : "ERROR"));
            return d->fontHeight() + 2;
        }

        void OnHighlight() override {
            vmm->configured = mdl;
            vmm->configured_element = this;
        }

        void OnClick() override {
            fGUI::CloseMenuOnTop();
            fGUI::OpenMenu(13);
        }

        void Update() {
            name = mdl->mdl->Config["name"].as<String>();
        }

        fGMS::ValveModule* mdl;

        String name;
    };

    class GreenhouseModuleConfigTabsMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            AddElement(new PhysicalButton("Hyg", 1, width - 12, 30, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(9); }));
            AddElement(new PhysicalButton("Vlv", 2, width - 12, 65, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(12); }));
            //AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

        }

        void Exit() override {
            fGUIElementMenu::Exit();

            fGUI::CloseMenuOnTop();
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            fGUI::OpenMenuOnTop(9);
        }
    };

    class MonitorTabsMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            AddElement(new PhysicalButton("Hyg", 1, width - 12, 30, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(19); }));
            //AddElement(new PhysicalButton("Dev", 2, width - 12, 65, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(21); }));
            //AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

        }

        void Exit() override {
            fGUIElementMenu::Exit();

            fGUI::CloseMenuOnTop();
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            fGUI::OpenMenuOnTop(19);
        }
    };

    class HygrometerModulesMenu : public ListMenu {
    public:
        void AddModule(fGMS::HygrometerModule* c) {
            if (!modulePresent)
                RemoveElement(0);

            modulePresent = true;

            AddElement(new HygrometerModuleListElement(c));
        }

        void RemoveModule(int i) {
            RemoveElement(i);
        }

        void InitializeElements() {
            uint16_t c = d->color24to16(0x303030);

            AddElement(new ListTextElement("No hygrometer modules.", 2, 1, TFT_WHITE, TFT_BLACK));

            start_x = 16;

            bg = false;
        }

        void Enter() override {
            ListMenu::Enter();

            if (numElements == 0) {
                AddElement(new ListTextElement("No hygrometer modules.", 2, 1, TFT_WHITE, TFT_BLACK));
                modulePresent = false;
            }
        }

        void Draw() override {
            ListMenu::Draw();

            if (millis() - lastUpdateMillis > 1000) {
                UpdateElements();
                lastUpdateMillis = millis();
            }
        }

        void UpdateElements() {
            for (int i = 0; i < fGMS::HygrometerModuleCount; i++) {
                fGMS::HygrometerModule* m = fGMS::HygrometerModules[i];

                if (modulePresent) {
                    bool skip = false;
                    for (int i = 0; i < numElements; i++)
                        if (((HygrometerModuleListElement*)elements[i])->mdl == m) {
                            skip = true;
                            break;
                        }


                    if (skip)
                        continue;
                }

                AddModule(m);
            }

            if (!modulePresent)
                return;

            /*for (int i = 0; i < numElements; i++)
                if (!((HygrometerModuleListElement*)elements[i])->mdl->ok)
                    RemoveElement(i);*/


            for (int i = 0; i < numElements; i++)
                ((HygrometerModuleListElement*)elements[i])->Update();

            addedModules = true;
        }

        fGMS::HygrometerModule* configured;
        HygrometerModuleListElement* configured_element;

    private:
        bool addedModules = false;
        bool modulePresent = false;

        long lastUpdateMillis = 0;
    };

    class ValveModulesMenu : public ListMenu {
    public:
        void AddModule(fGMS::ValveModule* c) {
            if (!modulePresent)
                RemoveElement(0);

            modulePresent = true;

            AddElement(new ValveModuleListElement(c));

            bg = false;
        }

        void RemoveModule(int i) {
            RemoveElement(i);
        }

        void InitializeElements() {
            uint16_t c = d->color24to16(0x303030);

            AddElement(new ListTextElement("No valve modules.", 2, 1, TFT_WHITE, TFT_BLACK));

            start_x = 16;
        }

        void Enter() override {
            ListMenu::Enter();

            if (numElements == 0) {
                AddElement(new ListTextElement("No valve modules.", 2, 1, TFT_WHITE, TFT_BLACK));
                modulePresent = false;
            }
        }

        void Draw() override {
            ListMenu::Draw();

            if (millis() - lastUpdateMillis > 1000) {
                UpdateElements();
                lastUpdateMillis = millis();
            }
        }

        void UpdateElements() {
            for (int i = 0; i < fGMS::ValveModuleCount; i++) {
                fGMS::ValveModule* m = fGMS::ValveModules[i];

                if (modulePresent) {
                    bool skip = false;
                    for (int i = 0; i < numElements; i++)
                        if (((ValveModuleListElement*)elements[i])->mdl == m) {
                            skip = true;
                            break;
                        }


                    if (skip)
                        continue;
                }

                AddModule(m);
            }

            if (!modulePresent)
                return;
            /*
            for (int i = 0; i < numElements; i++)
                if (!((ValveModuleListElement*)elements[i])->mdl->ok)
                    RemoveElement(i);*/


            for (int i = 0; i < numElements; i++)
                ((ValveModuleListElement*)elements[i])->Update();

            addedModules = true;
        }

        fGMS::ValveModule* configured;
        ValveModuleListElement* configured_element;

    private:
        bool addedModules = false;
        bool modulePresent = false;

        long lastUpdateMillis = 0;
    };

    class HygrometerModuleConfigMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("HYGRO XX", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            valueD = new UncenteredTextElement("Values: VAL, VAL, VAL, VAL", 6, 70, 0, 1, TFT_WHITE);
            channelsD = new UncenteredTextElement("Channels: XX", 6, 90, 0, 1, TFT_WHITE);

            AddElement(new TextElement("Hygrometer Module", width / 2, 12, 0, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(valueD);

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            Update();
        }

        void Update() {
            nameD->t = hmm->configured_element->name;
            macD->t = hmm->configured->mdl->MAC_Address;

            stateD->t = hmm->configured->ok ? "OK" : "ERROR";

            String values = "Values: ";

            for (int i = 0; i < hmm->configured->Channels; i++)
                values += String(hmm->configured->Values[i]) + ((i % 4 == 0 && i > 0) ? ",\n" : ", ");

            valueD->t = values;

            channelsD->t = String(hmm->configured->Channels);
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - lastUpdateMillis > 100) {
                Update();
                lastUpdateMillis = millis();
            }
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        UncenteredTextElement* valueD;
        UncenteredTextElement* channelsD;

        long lastUpdateMillis = 0;
    };

    class ValveModuleConfigMenu : public fGUIElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("VALVE XX", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            stateD2 = new UncenteredTextElement("Values: VAL, VAL, VAL, VAL", 6, 70, 0, 1, TFT_WHITE);

            stateInput = new NumberInputElement("Set state:", width / 2, 80, 0, 1, TFT_WHITE, TFT_DARKGREY);

            AddElement(new TextElement("Valve Module", width / 2, 12, 0, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(stateD2);
            AddElement(stateInput);
            //AddElement(new Button("Set state", width / 2, 92, 72, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {}, "Set state"));

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));

            xTaskCreate(setstatetask, "valveStatusUpdater", 4096, nullptr, 0, nullptr);
        }

        void Enter() override {
            fGUIElementMenu::Enter();

            Update();

            run = true;
        }

        void Update() {
            nameD->t = vmm->configured_element->name;
            macD->t = vmm->configured->mdl->MAC_Address;

            stateD->t = vmm->configured->ok ? "OK" : "ERROR";
            stateD2->t = String(vmm->configured->State);
        }

        void Exit() override {
            fGUIElementMenu::Exit();

            run = false;
        }

        static void setstatetask(void* param) {
            while (true) {
                delay(100);
                if (run) {
                    if (vmm->configured->State != stateInput->Value)
                        vmm->configured->SetState(stateInput->Value);
                }
            };
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - lastUpdateMillis > 100) {
                Update();
                lastUpdateMillis = millis();
            }
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        UncenteredTextElement* stateD2;
        static NumberInputElement* stateInput;

        static bool run;

        long lastUpdateMillis = 0;
    };

    class HygrometerDisplayElement;
    class HygrometerPositionConfigMenu : public fGUIElementMenu {
        void InitializeElements() override {
            AddElement(new BoxElement(width / 2, height / 2, 112, 144, d->color24to16(0x202020)));

            AddElement(new GreenhouseDisplayElement(width / 2, 80, 96, 96, d->color24to16(0x633e1a), TFT_LIGHTGREY));

            hde = new HygrometerDisplayElement(nullptr, (GreenhouseDisplayElement*)Elements[1]);
            AddElement(hde);

            x_in = new FloatInputElement("X: ", width / 2 - 32, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            y_in = new FloatInputElement("Y: ", width / 2 + 32, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);

            AddElement(x_in);
            AddElement(y_in);
        }

        void Enter() override {
            x_in->Value = hscm->selected->hyg->x;
            y_in->Value = hscm->selected->hyg->y;

            hde->hyg = hscm->selected->hyg;

            fGUIElementMenu::Enter();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            hscm->selected->hyg->x = x_in->Value;
            hscm->selected->hyg->y = y_in->Value;
        }

        FloatInputElement* x_in, * y_in;
        HygrometerDisplayElement* hde;
    };

    class GreenhouseDisplayElement : public MenuElement {
    public:
        float wth, hgt;
        int x, y;
        int color, border_color;

        bool draw_size = false;

        GreenhouseDisplayElement(int x_p, int y_p, int w, int h, int c, int bc) {
            x = x_p;
            y = y_p;
            wth = w;
            hgt = h;
            color = c;
            border_color = bc;
        }

        void Draw() override {
            float ratio = fGMS::greenhouse.x_size / fGMS::greenhouse.y_size;

            x_size = wth;
            y_size = hgt;

            if (ratio < 1)
                x_size = wth * ratio;
            else if (ratio > 1)
                y_size = hgt / ratio;

            zoom_factor = x_size / fGMS::greenhouse.x_size;
            d->fillRect(x - (x_size + 8) / 2, y - (y_size + 8) / 2, x_size + 8, y_size + 8, border_color);
            d->fillRect(x - x_size / 2, y - y_size / 2, x_size, y_size, color);

            if (draw_size) {
                int s = d->textWidth(String(fGMS::greenhouse.x_size) + " m");
                d->setCursor(x - s / 2, y - d->fontHeight() * 1.5);
                d->print(String(fGMS::greenhouse.x_size) + " m");

                s = d->textWidth("X");
                d->setCursor(x - s / 2, y - d->fontHeight() * 0.5);
                d->print("X");

                s = d->textWidth(String(fGMS::greenhouse.y_size) + " m");
                d->setCursor(x - s / 2, y + d->fontHeight() * 0.5);
                d->print(String(fGMS::greenhouse.y_size) + " m");
            }
        }

        int x_size, y_size;
        int zoom_factor = 1;
    };

    class GreenhouseSizeConfigMenu : public fGUIElementMenu {
        void InitializeElements() override {
            AddElement(new GreenhouseDisplayElement(width / 2, 80, 96, 96, d->color24to16(0x633e1a), TFT_LIGHTGREY));
            ((GreenhouseDisplayElement*)Elements[0])->draw_size = true;
            x_in = new FloatInputElement("Width: ", width / 2, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            y_in = new FloatInputElement("Height: ", width / 2, 152, 0, 1, TFT_BLACK, TFT_LIGHTGREY);

            AddElement(x_in);
            AddElement(y_in);
        }

        void Enter() override {
            x_in->Value = fGMS::greenhouse.x_size;
            y_in->Value = fGMS::greenhouse.y_size;

            fGUIElementMenu::Enter();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            fGMS::greenhouse.x_size = x_in->Value;
            fGMS::greenhouse.y_size = y_in->Value;
        }

        void Exit() {
            fGMS::Save();
        }

        FloatInputElement* x_in, * y_in;
    };

    class HygrometerDisplayElement : public HighlightableMenuElement {
    public:
        HygrometerDisplayElement(fGMS::Hygrometer* h, GreenhouseDisplayElement* g) {
            hyg = h;
            gde = g;
        }

        void Draw() override {
            String text;
            int bgc = TFT_SKYBLUE;

            if (displayID)
                text = String(hyg->id);
            else if (displayValue) {
                text = String((int)(hyg->GetValue() * 100));
                if (hyg->GetValue() == -1) {
                    text = "!";
                    bgc = TFT_ORANGE;
                }
            }

            int w = d->textWidth(text);
            int h = d->fontHeight();

            int x = (gde->x - gde->x_size / 2) + (hyg->x * gde->zoom_factor) - w / 2;
            int y = (gde->y - gde->y_size / 2) + (hyg->y * gde->zoom_factor) + h / 2;

            d->fillRect(x - 2, y - 2, w + 4, h + 2, isHighlighted ? TFT_DARKGREY : bgc);
            d->setCursor(x, y);
            d->setTextColor(TFT_BLACK);
            d->print(text);
        }

        bool isHighlightable() override {
            return selectable;
        }

        void OnClick() override {
            fGUI::OpenMenu(18);
        }

        fGMS::Hygrometer* hyg;
        GreenhouseDisplayElement* gde;

        bool selectable = false;

        bool displayID = false;
        bool displayValue = true;
    };

    class HygrometersConfigMenu : public fGUIElementMenu {
        void InitializeElements() override {
            gde = new GreenhouseDisplayElement(width / 2, height / 2, 152, 152, d->color24to16(0x633e1a), TFT_LIGHTGREY);
            AddElement(gde);

            //AddElement(new Button("Add", 16, height - 8, 24, 12, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { hscm->AddHygrometer(); }, "Add new hygrometer"));
            AddElement(new PhysicalButton("+", 1, width - 6, 30, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { hscm->AddHygrometer(); }));
            AddElement(new PhysicalButton("-", 1, width - 6, 30, 12, 24, 0, 1, TFT_BLACK, TFT_RED, []() { hscm->RemoveHygrometer(); }));

        }

        void Enter() override {
            fGUIElementMenu::Enter();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - lastUpdateMS > 5000)
                Update();


            for (int i = 0; i < numDisplays; i++) {
                if (displays[i]->isHighlighted) {
                    selected = displays[i];

                    return;
                }
            }

            selected = nullptr;
        }

        void Exit() {
            fGMS::Save();
        }

        void Update() {
            for (int i = 0; i < fGMS::HygrometerCount; i++) {
                fGMS::Hygrometer* h = fGMS::Hygrometers[i];

                bool skip = false;
                for (int j = 0; j < numDisplays; j++) {
                    if (displays[j]->hyg == h) {
                        if (h->invalid) {
                            RemoveElement(j);
                            skip = true;
                            break;
                        }
                        else {
                            skip = true;
                            break;
                        }
                    }
                }

                if (skip)
                    continue;


                HygrometerDisplayElement* element = new HygrometerDisplayElement(h, gde);
                element->selectable = true;
                element->displayID = true;
                element->displayValue = false;

                displays[numDisplays++] = element;
                AddElement(element);
            }

            lastUpdateMS = millis();
        }

        GreenhouseDisplayElement* gde;
        HygrometerDisplayElement* displays[1024];
        int numDisplays = 0;

        long lastUpdateMS = 0;

    public:
        HygrometerDisplayElement* selected;

        void AddHygrometer() {
            fGMS::Hygrometer* h = fGMS::CreateHygrometer();

            if (selected != nullptr) {
                h->x = selected->hyg->x;
                h->y = selected->hyg->y;
            }
            else {
                h->x = 1;
                h->y = 1;
            }

            HygrometerDisplayElement* element = new HygrometerDisplayElement(h, gde);
            element->selectable = true;

            displays[numDisplays++] = element;
            Highlight(AddElement(element));
            selected = element;
            fGUI::OpenMenu(17);

            Update();
        }

        void RemoveHygrometer() {
            fGMS::RemoveHygrometer(selected->hyg->id);
            RemoveElement(HighlightedElement);
        }
    };

    class MoistureMonitorMenu : public fGUIElementMenu {
        void InitializeElements() override {
            gde = new GreenhouseDisplayElement(width / 2, height / 2, 152, 152, d->color24to16(0x633e1a), TFT_LIGHTGREY);
            AddElement(gde);
        }

        void Enter() override {
            fGUIElementMenu::Enter();
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            if (millis() - lastUpdateMS > 5000)
                Update();


            for (int i = 0; i < numDisplays; i++) {
                if (displays[i]->isHighlighted) {
                    selected = displays[i];

                    return;
                }
            }

            selected = nullptr;
        }

        void Exit() {
            fGMS::Save();
        }

        void Update() {
            for (int i = 0; i < fGMS::HygrometerCount; i++) {
                fGMS::Hygrometer* h = fGMS::Hygrometers[i];

                bool skip = false;
                for (int j = 0; j < numDisplays; j++) {
                    if (displays[j]->hyg == h) {
                        if (h->invalid) {
                            RemoveElement(j);
                            skip = true;
                            break;
                        }
                        else {
                            skip = true;
                            break;
                        }
                    }
                }

                if (skip)
                    continue;


                HygrometerDisplayElement* element = new HygrometerDisplayElement(h, gde);

                displays[numDisplays++] = element;
                AddElement(element);
            }

            lastUpdateMS = millis();
        }

        GreenhouseDisplayElement* gde;
        HygrometerDisplayElement* displays[1024];
        int numDisplays = 0;

        long lastUpdateMS = 0;

    public:
        HygrometerDisplayElement* selected;
    };

    class HygrometerConfigMenu : public fGUIElementMenu {
        void InitializeElements() override {
            AddElement(new TextElement("Hygrometer XX", width / 2, 24, 0, 1, TFT_WHITE));

            AddElement(new Button("Pos", width / 2 - 40, 40, 40, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() { fGUI::OpenMenu(17); }, "Edit Pos"));
            AddElement(new Button("Remove", width / 2 + 28, 40, 64, 12, 0, 1, TFT_BLACK, TFT_RED, []() { fGUI::ExitMenu(); hscm->RemoveHygrometer(); }, "Remove"));

            module_names[0] = "S";
            moduleChoice = new TextChoiceElement(module_names, fGMS::HygrometerModuleCount, 4, 70, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(moduleChoice);

            moduleChoice->Value = 1;

            channelChoice = new NumberInputElement("Ch", 4, 90, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(channelChoice);

            mapMinInput = new FloatInputElement("Min", width / 2 + 8, 70, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMinInput);

            mapMaxInput = new FloatInputElement("Max", width / 2 + 8, 90, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMaxInput);

            voltageDisplay = new TextElement("X.XXV", width / 2 - 40, 128, 2, 1, TFT_WHITE);
            AddElement(new TextElement("Volts:", width / 2 - 40, 115, 0, 1, TFT_WHITE));
            AddElement(voltageDisplay);

            valueDisplay = new TextElement("XX%", width / 2 + 40, 128, 2, 1, TFT_WHITE);
            AddElement(new TextElement("Value:", width / 2 + 40, 115, 0, 1, TFT_WHITE));
            AddElement(valueDisplay);
        }

        void Enter() {
            fGUIElementMenu::Enter();
            UpdateModuleNames();

            ((TextElement*)Elements[0])->t = "Hygrometer " + String(hscm->selected->hyg->id);

            moduleChoice->Value = 1;
            for (int i = 0; i < fGMS::HygrometerModuleCount; i++)
                if (fGMS::HygrometerModules[i] == hscm->selected->hyg->Module) {
                    moduleChoice->Value = i + 2;
                    break;
                }

            channelChoice->Value = hscm->selected->hyg->Channel;

            mapMinInput->Value = hscm->selected->hyg->map_min;
            mapMaxInput->Value = hscm->selected->hyg->map_max;
        }

        void Draw() override {
            fGUIElementMenu::Draw();

            hscm->selected->hyg->Channel = channelChoice->Value;
            if (moduleChoice->Value > 1)
                hscm->selected->hyg->Module = fGMS::HygrometerModules[moduleChoice->Value - 2];
            else
                hscm->selected->hyg->Module = nullptr;

            hscm->selected->hyg->map_min = mapMinInput->Value;
            hscm->selected->hyg->map_max = mapMaxInput->Value;


            voltageDisplay->t = String(hscm->selected->hyg->GetRaw()) + "V";
            valueDisplay->t = String((int)(hscm->selected->hyg->GetValue() * 100)) + "%";

            if (voltageDisplay->t == "-1.00V") { voltageDisplay->t = "ERR"; }
            if (valueDisplay->t == "-100%") { valueDisplay->t = "ERR"; }
        }

        void UpdateModuleNames() {
            module_names[0] = "Source";
            module_names[1] = "NONE";

            for (int i = 0; i < fGMS::HygrometerModuleCount; i++)
                module_names[i + 2] = fGMS::HygrometerModules[i]->mdl->Config["name"].as<String>();

            moduleChoice->UpdateTextNum(fGMS::HygrometerModuleCount + 2);
        }

        TextChoiceElement* moduleChoice;
        NumberInputElement* channelChoice;

        FloatInputElement* mapMinInput;
        FloatInputElement* mapMaxInput;
        TextElement* valueDisplay;
        TextElement* voltageDisplay;

        String module_names[128];
    };

public:
    static void Init() {
        ESP32Encoder* e = new ESP32Encoder();
        ESP32Encoder::useInternalWeakPullResistors = UP;
        e->attachSingleEdge(22, 21);
        pinMode(34, INPUT);

        fGUI::AttachButton(16);
        fGUI::AttachButton(17);
        fGUI::AttachButton(19);

        mm = new ModuleMenu();
        mmo = new ModuleMenuOverlay();
        mmmo = new ModuleMenuManageOverlay();
        msm = new ModuleSettingsMenu();
        mstm = new ModuleStatsMenu();
        scm = new SystemConfigMenu();
        icm = new I2CConfigsMenu();
        hmm = new HygrometerModulesMenu();
        vmm = new ValveModulesMenu();
        gscm = new GreenhouseSizeConfigMenu();
        gmctm = new GreenhouseModuleConfigTabsMenu();
        hscm = new HygrometersConfigMenu();
        hpco = new HygrometerPositionConfigMenu();
        hcm = new HygrometerConfigMenu();
        mmm = new MoistureMonitorMenu();
        mtm = new MonitorTabsMenu();

        fGUI::AddMenu(new TitleMenu());
        fGUI::AddMenu(new DebugMenu());
        fGUI::AddMenu(mm);
        fGUI::AddMenu(mmo);
        fGUI::AddMenu(mmmo);
        fGUI::AddMenu(msm);
        fGUI::AddMenu(scm);
        fGUI::AddMenu(mstm);
        fGUI::AddMenu(icm);
        fGUI::AddMenu(hmm);
        fGUI::AddMenu(new ConfigMenu());
        fGUI::AddMenu(new HygrometerModuleConfigMenu());
        fGUI::AddMenu(vmm);
        fGUI::AddMenu(new ValveModuleConfigMenu());
        fGUI::AddMenu(gscm);
        fGUI::AddMenu(gmctm);
        fGUI::AddMenu(hscm);
        fGUI::AddMenu(hpco);
        fGUI::AddMenu(hcm);
        fGUI::AddMenu(mmm);
        fGUI::AddMenu(mtm);

        fGUI::Init(e, 34);

        alertmenu = new AlertMenu("");
        alertmenuid = fGUI::AddMenu(alertmenu);


        xTaskCreate(updateStatusTask, "cnt_gui_status", 4096, nullptr, 0, nullptr);
        xTaskCreate(updateGUITask, "cnt_gui_modules", 4096, nullptr, 0, nullptr);
    }

    static void AddModules() {
        for (int i = 0; i < fNETController::ModuleCount; i++)
            mm->AddModule(fNETController::Modules[i]);

        mm->UpdateElements();
    }

private:
    static ModuleMenu* mm;
    static ModuleMenuOverlay* mmo;
    static ModuleMenuManageOverlay* mmmo;
    static ModuleSettingsMenu* msm;
    static ModuleStatsMenu* mstm;
    static SystemConfigMenu* scm;
    static I2CConfigsMenu* icm;
    static HygrometerModulesMenu* hmm;
    static ValveModulesMenu* vmm;
    static GreenhouseSizeConfigMenu* gscm;
    static GreenhouseModuleConfigTabsMenu* gmctm;
    static HygrometersConfigMenu* hscm;
    static HygrometerPositionConfigMenu* hpco;
    static HygrometerConfigMenu* hcm;
    static MoistureMonitorMenu* mmm;
    static MonitorTabsMenu* mtm;


    static void updateGUITask(void* param) {
        while (true) {
            delay(1000);

            //scm->SetConfig(fNETController::data);
            AddModules();
        }
    }

    static AlertMenu* alertmenu;
    static int alertmenuid;

    static void updateStatusTask(void* param) {
        delay(500);

        String prev_status;

        bool alertsShown = false;
        while (true) {
            delay(100);

            if (fNETController::status_d == prev_status)
                continue;

            prev_status = fNETController::status_d;

            if ((!fNETController::I2C_IsEnabled) && !alertsShown) {
                alertmenu->updateMessage("I2C Disabled", "I2C has been disabled.");
                alertsShown = true;
            }
            else if (fNETController::status_d == "mount_fail")
                alertmenu->updateMessage("FS error!", "Filesystem mount fail!");
            else if (fNETController::status_d == "cfg_err")
                alertmenu->updateMessage("Config error!", "Can't read config!");
            else if (fNETController::status_d == "init" || fNETController::status_d == "setup_i2c" || fNETController::status_d == "load_mdl" || fNETController::status_d == "read_cfg")
                alertmenu->updateMessage("Startup", fNETController::status_d);
            else
                continue;

            if (fGUI::CurrentOpenMenu != alertmenuid)
                fGUI::OpenMenu(alertmenuid);
        }
    }
};