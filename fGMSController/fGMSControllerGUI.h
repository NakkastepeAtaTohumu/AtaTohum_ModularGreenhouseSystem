#include "fGUILib.h"
#include "fGUIElementMenu.h"
#include "fNETController.h"

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

            AddElement(new Button("M", 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {}, "Monitor"));
            AddElement(new Button("C", width / 2, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {}, "Config"));
            AddElement(new Button("L", width - 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() {}, "Logs"));
            AddElement(new Button("D", width - 24, 112, 32, 32, 2, 1, TFT_BLACK, TFT_DARKGREY, []() { fGUI::OpenMenu(1); }, "Debug"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
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
            else
                d->setTextColor(TFT_WHITE, bgc, true);

            d->setTextSize(1);
            //DrawTextCentered(d, t, x, y - (d->fontHeight() / 2));
            d->setCursor(x, y);

            if (!isSelected) {
                d->print(type + " " + String(id) + String(isOnline ? " OK" : " DC"));
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
                fGUI::OpenMenu(alertmenuid);
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
            AddElement(new PhysicalButton("M", 1, width - 12, 30, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenu(4); }));
            AddElement(new PhysicalButton("T", 2, width - 12, 65, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

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
            totalTransD = new UncenteredTextElement("Average Transaction: MSMSMS", 6, 80, 0, 1, TFT_WHITE);
            failTransD = new UncenteredTextElement("Average Transaction: MSMSMS", 6, 80, 0, 1, TFT_WHITE);

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
            totalTransD->t = "Total trans: " + String(mm->configured->TotalTransactions) + " ms";
            failTransD->t = "Failed trans: " + String(mm->configured->FailedTransactions) + " ms";
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

public:
    static void Init() {
        ESP32Encoder* e = new ESP32Encoder();
        ESP32Encoder::useInternalWeakPullResistors = UP;
        e->attachSingleEdge(22, 21);
        pinMode(34, INPUT);

        fGUI::AttachButton(16);
        fGUI::AttachButton(17);

        title = new TitleMenu();
        dbm = new DebugMenu();
        mm = new ModuleMenu();
        mmo = new ModuleMenuOverlay();
        mmmo = new ModuleMenuManageOverlay();
        msm = new ModuleSettingsMenu();
        mstm = new ModuleStatsMenu();
        scm = new SystemConfigMenu();
        icm = new I2CConfigsMenu();

        fGUI::AddMenu(title);
        fGUI::AddMenu(dbm);
        fGUI::AddMenu(mm);
        fGUI::AddMenu(mmo);
        fGUI::AddMenu(mmmo);
        fGUI::AddMenu(msm);
        fGUI::AddMenu(scm);
        fGUI::AddMenu(mstm);
        fGUI::AddMenu(icm);

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
    static TitleMenu* title;
    static DebugMenu* dbm;
    static ModuleMenu* mm;
    static ModuleMenuOverlay* mmo;
    static ModuleMenuManageOverlay* mmmo;
    static ModuleSettingsMenu* msm;
    static ModuleStatsMenu* mstm;
    static SystemConfigMenu* scm;
    static I2CConfigsMenu* icm;

    static void updateGUITask(void* param) {
        while (true) {
            delay(1000);

            scm->SetConfig(fNETController::data);
            AddModules();
        }
    }

    static AlertMenu* alertmenu;
    static int alertmenuid;

    static void updateStatusTask(void* param) {
        String prev_status;

        bool alertsShown = false;
        while (true) {
            delay(100);

            if (fNETController::status_d == prev_status)
                continue;

            prev_status = fNETController::status_d;

            if (fNETController::status_d == "mount_fail")
                alertmenu->updateMessage("FS error!", "Filesystem mount fail!");
            else if (fNETController::status_d == "cfg_err")
                alertmenu->updateMessage("Config error!", "Can't read config!");
            else if (fNETController::status_d == "init" || fNETController::status_d == "setup_i2c" || fNETController::status_d == "load_mdl" || fNETController::status_d == "read_cfg")
                alertmenu->updateMessage("Startup", fNETController::status_d);
            else if (!fNETController::I2C_IsEnabled && !alertsShown)
                alertmenu->updateMessage("I2C Disabled", "I2C has been disabled.");
            else
                continue;

            if (fGUI::CurrentOpenMenu != alertmenuid)
                fGUI::OpenMenu(alertmenuid);

            alertsShown = true;
        }
    }
};