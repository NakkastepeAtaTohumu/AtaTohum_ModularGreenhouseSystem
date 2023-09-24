#include "fGUI.h"
#include "ElementMenu.h"
#include "fNETController.h"
#include "fGMS.h"

class fGMSControllerMenu {
private:
    class TitleMenu : public ElementMenu {
    public:
        void InitializeElements() {
            AddElement(new TextElement("AtaTohum", width / 2, 20, 4, 1, TFT_GOLD));
            AddElement(new TextElement("MGMS Greenhouse", width / 2, 32, 0, 1, TFT_GOLD));

            /*AddElement(new Button("Monitor", width / 2, 74, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {fGUI::OpenMenu(1); }));
            AddElement(new Button("Config", width / 2, 107, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            AddElement(new Button("Debug", width / 2, 140, 96, 24, 2, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));*/

            AddElement(new Button("M", 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() { fGUI::OpenMenu(20); }, "Monitor"));
            AddElement(new Button("C", width / 2, 72, 32, 32, 2, 1, TFT_BLACK, TFT_ORANGE, []() { fGUI::OpenMenu(10); }, "Config"));
            AddElement(new Button("W", width - 24, 72, 32, 32, 2, 1, TFT_BLACK, TFT_BLUE, []() {fGUI::OpenMenu(25); }, "Watering"));
            AddElement(new Button("S", 24, 112, 32, 32, 2, 1, TFT_BLACK, TFT_GREEN, []() { /*fGUI::OpenMenu(23);*/ fGMS::serverEnabled = true; fGMS::Save(); ESP.restart(); }, "Enable Server"));
            AddElement(new Button("D", width / 2, 112, 32, 32, 2, 1, TFT_BLACK, TFT_DARKGREY, []() { fGUI::OpenMenu(1); }, "Debug"));

            AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }
    };

    class ConfigMenu : public ElementMenu {
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

    class DebugMenu : public ElementMenu {
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
        ModuleListElement(fNETController::Module* md) {
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
            d->println("Chip ID:");
            d->setCursor(x + 16, y + 30);
            d->println(" " + mac);

            d->setCursor(x + 16, y + 44);
            d->println("Type:");
            d->setCursor(x + 16, y + 52);
            d->println(" " + type);

            return 60;
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
            mac = mdl->MAC_Address;

            type = mdl->Config["ModuleType"].as<String>();
            name = mdl->Config["name"].as<String>();

            if (name == "null")
                name = "UNKNOWN";

            if (type == "null")
                type = "UNKNOWN";

            id = fNETController::GetModuleIndex(mdl);

            isOnline = mdl->isOnline;

            if (!mdl->valid)
                mm->RemoveModule();

            wasOnline = isOnline;
        }

        fNETController::Module* mdl;

        String mac;
        String type;
        String name;
        int id;

    private:
        bool isOnline;
        bool wasOnline;
    };

    class ModuleMenu : public ListMenu {
    public:
        void AddModule(fNETController::Module* c) {
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

        fNETController::Module* configured;
        ModuleListElement* configured_element;

    private:
        bool addedModules = false;
        bool modulePresent = false;
    };

    class ModuleMenuOverlay : public ElementMenu {
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
            ElementMenu::Draw();

            if (fGUI::CurrentOpenMenu != 2)
                fGUI::CloseMenuOnTop();
        }
    };

    class ModuleMenuManageOverlay : public ElementMenu {
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

    class ModuleSettingsMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("Module: MDLMDLM X", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);

            AddElement(new TextElement("Actions", width / 2, 12, 2, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);

            AddElement(new Button("Restart", width / 2, 70, 76, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {
                mm->configured->Restart();
                }, ""));

            //AddElement(new Button("Edit Config", width / 2, 110, 76, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {}, ""));
            AddElement(new Button("REMOVE", width / 2, 148, 76, 12, 0, 1, TFT_BLACK, TFT_RED, []() {
                if (mm->configured->isOnline)
                {
                    alertmenu->updateMessage("Can't remove", "Module is online.");
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
            ElementMenu::Enter();

            nameD->t = "Module: " + mm->configured_element->type + " " + String(mm->configured_element->id);
            macD->t = mm->configured->MAC_Address;

            stateD->t = mm->configured->isOnline ? "ONLINE" : "OFFLINE";
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
    };

    class ModuleStatsMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("Module: MDLMDLM X", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            pingD = new UncenteredTextElement("Ping: MSMSMS", 6, 70, 0, 1, TFT_WHITE);

            AddElement(new TextElement("Stats", width / 2, 12, 2, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(pingD);

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }

        void Enter() override {
            ElementMenu::Enter();

            Update();
        }

        void Update() {
            nameD->t = "Module: " + mm->configured_element->type + " " + String(mm->configured_element->id);
            macD->t = mm->configured->MAC_Address;

            stateD->t = mm->configured->isOnline ? "ONLINE" : "OFFLINE";

            pingD->t = "Avg poll: " + String(mm->configured->Ping * 1000) + " ms";
        }

        void Draw() override {
            ElementMenu::Draw();

            if (millis() - lastUpdateMillis > 100) {
                Update();
                lastUpdateMillis = millis();
            }
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        UncenteredTextElement* pingD;

        long lastUpdateMillis = 0;
    };

    class AlertMenu : public ElementMenu {
    public:
        AlertMenu(String m) {
            msg = m;
        }

        void Enter() override {
            ElementMenu::Enter();

            opened_millis = millis();
        }

        void Draw() override {
            ElementMenu::Draw();

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
            fGUI::OpenMenu(26);
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
            fGUI::OpenMenu(11);
        }

        void Update() {
            name = mdl->mdl->Config["name"].as<String>();
        }

        fGMS::ValveModule* mdl;

        String name;
    };

    class SensorModuleListElement : public ListElement {
    public:
        SensorModuleListElement(fGMS::SensorModule* md) {
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
            smm->configured = mdl;
            smm->configured_element = this;
        }

        void OnClick() override {
            fGUI::CloseMenuOnTop();
            fGUI::OpenMenu(22);
        }

        void Update() {
            name = mdl->mdl->Config["name"].as<String>();
        }

        fGMS::SensorModule* mdl;

        String name;
    };

    class GreenhouseModuleConfigTabsMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            AddElement(new PhysicalButton("Hyg", 1, width - 12, 30, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(9); }));
            AddElement(new PhysicalButton("Vlv", 2, width - 12, 65, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(12); }));
            AddElement(new PhysicalButton("Sns", 3, width - 12, 98, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(21); }));
            //AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

        }

        void Exit() override {
            ElementMenu::Exit();

            fGUI::CloseMenuOnTop();
        }

        void Enter() override {
            ElementMenu::Enter();

            fGUI::OpenMenuOnTop(9);
        }
    };

    class MonitorTabsMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            AddElement(new PhysicalButton("Hyg", 1, width - 12, 30, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(19); }));
            //AddElement(new PhysicalButton("Dev", 2, width - 12, 65, 24, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { fGUI::CloseMenuOnTop(); fGUI::OpenMenuOnTop(21); }));
            //AddElement(new PhysicalButton("C", 3, width - 12, 98, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));
            //AddElement(new PhysicalButton("L", 4, width - 12, 136, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() {}));

        }

        void Exit() override {
            ElementMenu::Exit();

            fGUI::CloseMenuOnTop();
        }

        void Enter() override {
            ElementMenu::Enter();

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
        }

        void RemoveModule(int i) {
            RemoveElement(i);
        }

        void InitializeElements() {
            uint16_t c = d->color24to16(0x303030);

            AddElement(new ListTextElement("No valve modules.", 2, 1, TFT_WHITE, TFT_BLACK));

            start_x = 16;
            bg = false;
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

    class SensorModulesMenu : public ListMenu {
    public:
        void AddModule(fGMS::SensorModule* c) {
            if (!modulePresent)
                RemoveElement(0);

            modulePresent = true;

            AddElement(new SensorModuleListElement(c));
        }

        void RemoveModule(int i) {
            RemoveElement(i);
        }

        void InitializeElements() {
            uint16_t c = d->color24to16(0x303030);

            AddElement(new ListTextElement("No sensor modules.", 2, 1, TFT_WHITE, TFT_BLACK));

            start_x = 16;
            bg = false;
        }

        void Enter() override {
            ListMenu::Enter();

            if (numElements == 0) {
                AddElement(new ListTextElement("No sensor modules.", 2, 1, TFT_WHITE, TFT_BLACK));
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
            for (int i = 0; i < fGMS::SensorModuleCount; i++) {
                fGMS::SensorModule* m = fGMS::SensorModules[i];

                if (modulePresent) {
                    bool skip = false;
                    for (int i = 0; i < numElements; i++)
                        if (((SensorModuleListElement*)elements[i])->mdl == m) {
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
                ((SensorModuleListElement*)elements[i])->Update();

            addedModules = true;
        }

        fGMS::SensorModule* configured;
        SensorModuleListElement* configured_element;

    private:
        bool addedModules = false;
        bool modulePresent = false;

        long lastUpdateMillis = 0;
    };

    class HygrometerModuleConfigMenu : public ElementMenu {
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
            ElementMenu::Enter();

            Update();
        }

        void Update() {
            nameD->t = hmm->configured_element->name;
            macD->t = hmm->configured->mdl->MAC_Address;

            stateD->t = hmm->configured->ok ? "OK" : "ERROR";

            if (!hmm->configured->ok) {
                stateD->t = "ERROR";

                if (!hmm->configured->mdl->isOnline)
                    valueD->t = "Module offline!";

                else if (!hmm->configured->tunnel->IsConnected)
                    valueD->t = "Tunnel disconnected!";
                
                else if (!hmm->configured->value_received)
                    valueD->t = "No data received!";

                channelsD->t = "";
            }
            else {
                stateD->t = "OK";


                String values = "Values: ";

                for (int i = 0; i < hmm->configured->Channels; i++)
                    values += String(hmm->configured->Values[i]) + ((i % 4 == 0 && i > 0) ? ",\n" : ", ");

                valueD->t = values;


                channelsD->t = String(hmm->configured->Channels);
            }
        }

        void Draw() override {
            ElementMenu::Draw();

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

    class ValveModuleConfigMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("VALVE XX", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            stateD2 = new UncenteredTextElement("Values: VAL, VAL, VAL, VAL", 6, 70, 0, 1, TFT_WHITE);

            stateInput = new NumberInputElement("Set state:", width / 2, 80, 0, 1, TFT_WHITE, TFT_DARKGREY);
            AddElement(new Button("1", width / 5, 128, 12, 12, 0, 1, TFT_WHITE, TFT_DARKGREY, []() {vmcm->toggle(0b0001); }, "Toggle Valve 1"));
            AddElement(new Button("2", 2 * width / 5, 128, 12, 12, 0, 1, TFT_WHITE, TFT_DARKGREY, []() {vmcm->toggle(0b0010); }, "Toggle Valve 2"));
            AddElement(new Button("3", 3 * width / 5, 128, 12, 12, 0, 1, TFT_WHITE, TFT_DARKGREY, []() {vmcm->toggle(0b0100); }, "Toggle Valve 3"));
            AddElement(new Button("4", 4 * width / 5, 128, 12, 12, 0, 1, TFT_WHITE, TFT_DARKGREY, []() {vmcm->toggle(0b1000); }, "Toggle Valve 4"));

            AddElement(new TextElement("Valve Module", width / 2, 12, 0, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(stateD2);
            AddElement(stateInput);
            //AddElement(new Button("Set state", width / 2, 92, 72, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {}, "Set state"));

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));

            //xTaskCreate(setstatetask, "valveStatusUpdater", 4096, nullptr, 0, nullptr);
        }

        void Enter() override {
            ElementMenu::Enter();

            Update();

            run = true;
        }

        void Update() {
            nameD->t = vmm->configured_element->name;
            macD->t = vmm->configured->mdl->MAC_Address;

            stateD->t = vmm->configured->ok ? "OK" : "ERROR";
            stateD2->t = String(vmm->configured->State);

            vmm->configured->NextState = stateInput->Value;
        }

        void Exit() override {
            ElementMenu::Exit();

            run = false;
        }
        /*
        static void setstatetask(void* param) {
            while (true) {
                delay(100);
                if (run) {
                    if (vmm->configured->State != stateInput->Value)
                        vmm->configured->SetState(stateInput->Value);
                }
            };
        }*/

        static void toggle(int valves) {
            stateInput->Value = stateInput->Value ^ valves;
        }

        void Draw() override {
            ElementMenu::Draw();

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
        static int state;

        long lastUpdateMillis = 0;
    };

    class SensorModuleConfigMenu : public ElementMenu {
    public:
        void InitializeElements() override {
            nameD = new TextElement("AIR XX", width / 2, 30, 0, 1, TFT_WHITE);
            macD = new TextElement("MACMACMACMACMACMA", width / 2, 40, 0, 1, TFT_WHITE);
            stateD = new TextElement("STATESTATESTATEST", width / 2, 50, 0, 1, TFT_WHITE);
            stateD2 = new UncenteredTextElement("Values: VAL, VAL, VAL, VAL", 12, 70, 0, 1, TFT_WHITE);
            stateD3 = new UncenteredTextElement("Values: VAL, VAL, VAL, VAL", 12, 110, 0, 1, TFT_WHITE);

            AddElement(new TextElement("Valve Module", width / 2, 12, 0, 1, TFT_WHITE));
            AddElement(nameD);
            AddElement(macD);
            AddElement(stateD);
            AddElement(stateD2);
            AddElement(new Button("Toggle fan", width / 2, 92, 64, 12, 0, 1, TFT_WHITE, TFT_DARKGREY, []() {smm->configured->SetFan(!smm->configured->FanState); fGMS::Save(); }, "Toggle fan"));
            AddElement(stateD3);
            //AddElement(new Button("Set state", width / 2, 92, 72, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() {}, "Set state"));

            //AddElement(new TooltipDisplay(width / 2, 152, 2, 1, TFT_BLACK, TFT_DARKCYAN));
        }

        void Enter() override {
            ElementMenu::Enter();

            Update();
        }

        void Update() {
            nameD->t = smm->configured_element->name;
            macD->t = smm->configured->mdl->MAC_Address;

            stateD->t = smm->configured->ok ? "OK" : "ERROR";
            if (!smm->configured->ok) {
                stateD2->t = "";
                stateD3->t = "";

                return;
            }

            stateD2->t = "Fan: " + String(smm->configured->FanState ? "On" : "Off");
            stateD3->t = "PPM: " + String(smm->configured->ppm) + ", Temp: " + String(smm->configured->temp) + ", %hum: " + String(smm->configured->humidity);
        }

        void Draw() override {
            ElementMenu::Draw();

            if (millis() - lastUpdateMillis > 100) {
                Update();
                lastUpdateMillis = millis();
            }
        }

        TextElement* nameD;
        TextElement* macD;
        TextElement* stateD;
        UncenteredTextElement* stateD2, * stateD3;

        long lastUpdateMillis = 0;
    };

    class HygrometerDisplayElement;
    class HygrometerPositionConfigMenu : public ElementMenu {
        void InitializeElements() override {
            AddElement(new BoxElement(width / 2, height / 2, 112, 144, d->color24to16(0x202020)));

            AddElement(new GreenhouseDisplayElement(width / 2, 80, 96, 96, d->color24to16(0x633e1a), TFT_LIGHTGREY));

            hde = new HygrometerDisplayElement(nullptr, (GreenhouseDisplayElement*)Elements[1]);
            AddElement(hde);

            x_in = new FloatInputElement("X: ", 4, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            y_in = new FloatInputElement("Y: ", width / 2 + 4, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);

            AddElement(x_in);
            AddElement(y_in);
        }

        void Enter() override {
            x_in->Value = hscm->selected->hyg->x;
            y_in->Value = hscm->selected->hyg->y;

            hde->hyg = hscm->selected->hyg;

            ElementMenu::Enter();
        }

        void Draw() override {
            ElementMenu::Draw();

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

    class GreenhouseSizeConfigMenu : public ElementMenu {
        void InitializeElements() override {
            AddElement(new GreenhouseDisplayElement(width / 2, 80, 96, 96, d->color24to16(0x633e1a), TFT_LIGHTGREY));
            ((GreenhouseDisplayElement*)Elements[0])->draw_size = true;
            x_in = new FloatInputElement("Width: ", 20, 140, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            y_in = new FloatInputElement("Height: ", 20, 152, 0, 1, TFT_BLACK, TFT_LIGHTGREY);

            AddElement(x_in);
            AddElement(y_in);
        }

        void Enter() override {
            x_in->Value = fGMS::greenhouse.x_size;
            y_in->Value = fGMS::greenhouse.y_size;

            ElementMenu::Enter();
        }

        void Draw() override {
            ElementMenu::Draw();

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

    class HygrometersConfigMenu : public ElementMenu {
        void InitializeElements() override {
            gde = new GreenhouseDisplayElement(width / 2, height / 2, 152, 152, d->color24to16(0x633e1a), TFT_LIGHTGREY);
            AddElement(gde);

            //AddElement(new Button("Add", 16, height - 8, 24, 12, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { hscm->AddHygrometer(); }, "Add new hygrometer"));
            AddElement(new PhysicalButton("+", 1, width - 6, 30, 12, 24, 0, 1, TFT_BLACK, TFT_SKYBLUE, []() { hscm->AddHygrometer(); }));
            AddElement(new PhysicalButton("-", 2, width - 6, 65, 12, 24, 0, 1, TFT_BLACK, TFT_RED, []() { hscm->RemoveHygrometer(); }));

        }

        void Enter() override {
            ElementMenu::Enter();
        }

        void Draw() override {
            ElementMenu::Draw();

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

                int index = AddElement(element);

                Highlight(index);
            }

            lastUpdateMS = millis();
        }

        GreenhouseDisplayElement* gde;
        HygrometerDisplayElement* displays[32];
        int numDisplays = 0;

        long lastUpdateMS = 0;

    public:
        HygrometerDisplayElement* selected = nullptr;

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

    class MoistureMonitorMenu : public ElementMenu {
        void InitializeElements() override {
            gde = new GreenhouseDisplayElement(width / 2, height / 2, 152, 152, d->color24to16(0x633e1a), TFT_LIGHTGREY);
            AddElement(gde);
        }

        void Enter() override {
            ElementMenu::Enter();
        }

        void Draw() override {
            ElementMenu::Draw();

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
        HygrometerDisplayElement* displays[32];
        int numDisplays = 0;

        long lastUpdateMS = 0;

    public:
        HygrometerDisplayElement* selected;
    };

    class HygrometerGroupConfigMenu : public ElementMenu {
        void InitializeElements() override {
            AddElement(new TextElement("Group XX", width / 2, 24, 0, 1, TFT_WHITE));

            module_names[0] = "S";
            moduleChoice = new TextChoiceElement(module_names, fGMS::ValveModuleCount, 4, 70, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(moduleChoice);

            moduleChoice->Value = 1;

            channelChoice = new NumberInputElement("Ch", 80, 70, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(channelChoice);

            mapMinInput = new FloatInputElement("Lower %", 4, 130, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMinInput);

            mapMaxInput = new FloatInputElement("Upper %", 4, 150, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMaxInput);

            valueDisplay = new TextElement("XX%", width / 2 + 40, 145, 2, 1, TFT_WHITE);
            AddElement(new TextElement("Value:", width / 2 + 40, 130, 0, 1, TFT_WHITE));
            AddElement(valueDisplay);
        }

        void Enter() {
            ElementMenu::Enter();

            UpdateModuleNames();

            edited = fGMS::GetHygrometerGroupByHygrometer(hscm->selected->hyg);

            ((TextElement*)Elements[0])->t = "Group " + String(edited->id);

            moduleChoice->Value = 1;
            for (int i = 0; i < fGMS::ValveModuleCount; i++)
                if (fGMS::ValveModules[i] == edited->mod) {
                    moduleChoice->Value = i + 2;
                    break;
                }

            channelChoice->Value = edited->channel;

            mapMinInput->Value = edited->map_min;
            mapMaxInput->Value = hscm->selected->hyg->map_max;
        }

        void Draw() override {
            ElementMenu::Draw();
            edited->channel = channelChoice->Value;
            if (moduleChoice->Value > 1)
                edited->mod = fGMS::ValveModules[moduleChoice->Value - 2];
            else
                edited->mod = nullptr;

            edited->map_min = mapMinInput->Value;
            edited->map_max = mapMaxInput->Value;


            valueDisplay->t = String((int)(edited->GetAverage() * 100)) + "%";

            if (valueDisplay->t == "-100%") { valueDisplay->t = "ERR"; }
        }

        void UpdateModuleNames() {
            module_names[0] = "Source";
            module_names[1] = "NONE";

            for (int i = 0; i < fGMS::ValveModuleCount; i++)
                module_names[i + 2] = fGMS::ValveModules[i]->mdl->Config["name"].as<String>();

            moduleChoice->UpdateTextNum(fGMS::ValveModuleCount + 2);
        }

        TextChoiceElement* moduleChoice;
        NumberInputElement* channelChoice;

        FloatInputElement* mapMinInput;
        FloatInputElement* mapMaxInput;
        TextElement* valueDisplay;

        String module_names[4];

        fGMS::HygrometerGroup* edited;
    };

    class HygrometerConfigMenu : public ElementMenu {
        void InitializeElements() override {
            AddElement(new TextElement("Hygrometer XX", width / 2, 24, 0, 1, TFT_WHITE));

            AddElement(new Button("Pos", width / 2 - 40, 40, 40, 12, 0, 1, TFT_BLACK, TFT_DARKGREY, []() { fGUI::OpenMenu(17); }, "Edit Pos"));
            AddElement(new Button("Rm", width / 2 + 24, 40, 60, 12, 0, 1, TFT_BLACK, TFT_RED, []() { fGUI::ExitMenu(); hscm->RemoveHygrometer(); }, "Remove"));
            AddElement(new Button("Edit Grp", width / 2 + 24, 110, 60, 12, 0, 1, TFT_BLACK, TFT_ORANGE, []() { if (fGMS::GetHygrometerGroupByHygrometer(hscm->selected->hyg) != nullptr) fGUI::OpenMenu(24); }, "Group Config"));

            module_names[0] = "S";
            moduleChoice = new TextChoiceElement(module_names, fGMS::HygrometerModuleCount, 4, 70, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(moduleChoice);

            moduleChoice->Value = 1;

            channelChoice = new NumberInputElement("Ch", 4, 90, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(channelChoice);

            groupChoice = new NumberInputElement("Grp", 4, 110, 0, 1, TFT_BLACK, TFT_LIGHTGREY);
            AddElement(groupChoice);

            mapMinInput = new FloatInputElement("Min", width / 2 + 8, 70, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMinInput);

            mapMaxInput = new FloatInputElement("Max", width / 2 + 8, 90, 0, 1, TFT_BLACK, TFT_DARKGREY);
            AddElement(mapMaxInput);

            voltageDisplay = new TextElement("X.XXV", width / 2 - 40, 145, 2, 1, TFT_WHITE);
            AddElement(new TextElement("Volts:", width / 2 - 40, 132, 0, 1, TFT_WHITE));
            AddElement(voltageDisplay);

            valueDisplay = new TextElement("XX%", width / 2 + 40, 145, 2, 1, TFT_WHITE);
            AddElement(new TextElement("Value:", width / 2 + 40, 132, 0, 1, TFT_WHITE));
            AddElement(valueDisplay);
        }

        void Enter() {
            ElementMenu::Enter();
            UpdateModuleNames();

            ((TextElement*)Elements[0])->t = "Hygrometer " + String(hscm->selected->hyg->id);

            moduleChoice->Value = 1;
            for (int i = 0; i < fGMS::HygrometerModuleCount; i++)
                if (fGMS::HygrometerModules[i] == hscm->selected->hyg->Module) {
                    moduleChoice->Value = i + 2;
                    break;
                }

            groupChoice->Value = -1;
            for (int i = 0; i < fGMS::HygrometerGroupCount; i++) {
                for (int j = 0; j < fGMS::HygrometerGroups[i]->numHygrometers; j++) {
                    if (fGMS::HygrometerGroups[i]->hygrometers[j] == hscm->selected->hyg) {
                        groupChoice->Value = i;
                        first_group = i;
                        break;
                    }
                }
            }

            channelChoice->Value = hscm->selected->hyg->Channel;

            mapMinInput->Value = hscm->selected->hyg->map_min;
            mapMaxInput->Value = hscm->selected->hyg->map_max;
        }

        void Draw() override {
            ElementMenu::Draw();

            hscm->selected->hyg->Channel = channelChoice->Value;
            if (moduleChoice->Value > 1)
                hscm->selected->hyg->Module = fGMS::HygrometerModules[moduleChoice->Value - 2];
            else
                hscm->selected->hyg->Module = nullptr;

            hscm->selected->hyg->map_min = mapMinInput->Value;
            hscm->selected->hyg->map_max = mapMaxInput->Value;


            voltageDisplay->t = String(hscm->selected->hyg->GetRaw()) + "V";
            valueDisplay->t = String((int)(hscm->selected->hyg->GetValue() * 100)) + "%";

            groupChoice->Value = max(-1, min(groupChoice->Value, fGMS::HygrometerGroupCount));

            if (voltageDisplay->t == "-1.00V") { voltageDisplay->t = "ERR"; }
            if (valueDisplay->t == "-100%") { valueDisplay->t = "ERR"; }
        }

        void Exit() override {
            ElementMenu::Exit();

            if (groupChoice->Value != first_group) {
                if (first_group != -1) {
                    fGMS::HygrometerGroups[first_group]->RemoveHygrometer(hscm->selected->hyg);

                    if (fGMS::HygrometerGroups[first_group]->numHygrometers == 0)
                        fGMS::RemoveHygrometerGroup(first_group);
                }

                if (groupChoice->Value >= fGMS::HygrometerGroupCount)
                {
                    fGMS::HygrometerGroup* g = fGMS::CreateHygrometerGroup();

                    g->AddHygrometer(hscm->selected->hyg);
                }
                else if (groupChoice->Value != -1) {
                    fGMS::HygrometerGroup* g = fGMS::HygrometerGroups[groupChoice->Value];

                    g->AddHygrometer(hscm->selected->hyg);
                }
            }
        }

        void UpdateModuleNames() {
            module_names[0] = "Source";
            module_names[1] = "NONE";

            for (int i = 0; i < fGMS::HygrometerModuleCount; i++)
                module_names[i + 2] = fGMS::HygrometerModules[i]->mdl->Config["name"].as<String>();

            moduleChoice->UpdateTextNum(fGMS::HygrometerModuleCount + 2);
        }
    public:
        TextChoiceElement* moduleChoice;
        NumberInputElement* channelChoice;
        NumberInputElement* groupChoice;

        int first_group = -1;

        FloatInputElement* mapMinInput;
        FloatInputElement* mapMaxInput;
        TextElement* valueDisplay;
        TextElement* voltageDisplay;

        String module_names[4];
    };

    class EnableServerMenu : public ElementMenu {
    public:
        void InitializeElements() {
            AddElement(new TextElement("Server Enabled", width / 2, height / 2, 2, 1, TFT_GOLD));
            AddElement(new TextElement("RESET to exit", width / 2, 32, 0, 1, TFT_GOLD));
        }

        void Enter() override {
            ElementMenu::Enter();

            xTaskCreate(task, "enableServerTask", 8192, nullptr, 0, nullptr);
        }

        static void task(void* param) {
            delay(5000);
            fGUI::Screensaver();
            delay(1000);
            fGMS::serverEnabled = true; fGMS::Save(); ESP.restart();
        }
    };

    class WateringConfigMenu : public ElementMenu {
        void InitializeElements() {
            AddElement(new NumberInputElement("Water", 16, 32, 0, 1, TFT_WHITE, TFT_DARKGREY));
            AddElement(new NumberInputElement("Every", 16, 48, 0, 1, TFT_WHITE, TFT_DARKGREY));

            AddElement(new Button("ENABLE", width / 2, 72, 96, 16, 1, 1, TFT_BLACK, TFT_RED, []() {fGMS::AutomaticWatering = !fGMS::AutomaticWatering; }, ""));

            ((NumberInputElement*)Elements[0])->postfix = " mins";
            ((NumberInputElement*)Elements[1])->postfix = " mins";

            AddElement(new UncenteredTextElement("Not watering.", 16, 100, 1, 1, TFT_WHITE));
            AddElement(new UncenteredTextElement("Next watering: ", 16, 120, 0, 1, TFT_WHITE));
            //AddElement(new UncenteredTextElement("Time: ", 16, 132, 0, 1, TFT_WHITE));
        }

        void Enter() override {
            ElementMenu::Enter();

            int* period = &((NumberInputElement*)Elements[1])->Value;
            int* active = &((NumberInputElement*)Elements[0])->Value;

            *period = fGMS::watering_period  / 60;
            *active = fGMS::watering_active_period / 60;
        }

        void Draw() override {
            ElementMenu::Draw();

            Button* b = (Button*)Elements[2];

            if (fGMS::AutomaticWatering)
            {
                b->t = "DISABLE";
                b->bgc = TFT_GREEN;
            }
            else {
                b->t = "ENABLE";
                b->bgc = TFT_RED;
            }

            int* period = &((NumberInputElement*)Elements[1])->Value;
            int* active = &((NumberInputElement*)Elements[0])->Value;

            if (*period == 0)
                *period = 1;

            if (*active > *period)
                *active = *period;

            fGMS::SetWatering(*period * 60, *active * 60);

            String* state_text = &((UncenteredTextElement*)Elements[3])->t;
            String* seconds_text = &((UncenteredTextElement*)Elements[4])->t;
            String* time_text = &((UncenteredTextElement*)Elements[5])->t;

            bool watering = fGMS::IsWatering();

            *state_text = (watering ? "Watering" : "Not watering");

            if (watering) {
                int secondsRemaining = fGMS::GetSecondsToWater();
                if (secondsRemaining >= 60)
                    *seconds_text = "Continue for: " + String(secondsRemaining / 60) + "mins.";
                else
                    *seconds_text = "Continue for: " + String(secondsRemaining) + "secs.";
            }
            else {
                int secondsRemaining = fGMS::GetSecondsUntilWatering();

                if (secondsRemaining >= 60)
                    *seconds_text = "Start in: " + String(secondsRemaining / 60) + "mins.";
                else
                    *seconds_text = "Start in: " + String(secondsRemaining) + "secs.";
            }

            //*time_text = "Time: " + fGMS::ntp_client.getFormattedTime();
        }

        void Exit() override {
            ElementMenu::Exit();

            fGMS::Save();
        }
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
        fGUI::AttachButton(27);


        //Serial.println("MM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("MM Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mm = new ModuleMenu();

        //Serial.println("MMO Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("MMO Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mmo = new ModuleMenuOverlay();

        //Serial.println("MMMO Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mmmo = new ModuleMenuManageOverlay();

        //Serial.println("MSM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        msm = new ModuleSettingsMenu();

        //Serial.println("MSTM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mstm = new ModuleStatsMenu();

        //Serial.println("SCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        scm = new SystemConfigMenu();

        //Serial.println("ICM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        icm = new ElementMenu();

        //Serial.println("HMM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        hmm = new HygrometerModulesMenu();

        //Serial.println("VMM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        vmm = new ValveModulesMenu();

        //Serial.println("VMCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        vmcm = new ValveModuleConfigMenu();

        //Serial.println("GSCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        gscm = new GreenhouseSizeConfigMenu();

        //Serial.println("GMCTM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        gmctm = new GreenhouseModuleConfigTabsMenu();

        //Serial.println("HSCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        hscm = new HygrometersConfigMenu();

        //Serial.println("HPCO Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        hpco = new HygrometerPositionConfigMenu();

        //Serial.println("HCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        hcm = new HygrometerConfigMenu();

        //Serial.println("MMM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mmm = new MoistureMonitorMenu();

        //Serial.println("MTM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        mtm = new MonitorTabsMenu();

        //Serial.println("SMM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        smm = new SensorModulesMenu();

        //Serial.println("SMCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        smcm = new SensorModuleConfigMenu();

        //Serial.println("HMCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        hmcm = new HygrometerModuleConfigMenu();

        //Serial.println("TitleMenu Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

        fGUI::AddMenu(new TitleMenu());                 //1

        //Serial.println("Debug Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new DebugMenu());                 //2

        //Serial.println("MM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mm);                              //3

        //Serial.println("MMO Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mmo);                             //4

        //Serial.println("MMMO Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mmmo);                            //5

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(msm);                             //6

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(scm);                             //7 <-?

        //Serial.println("MSTM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mstm);                            //8 <-

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(icm);                             //9 <-

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(hmm);                             //10 <-

        //Serial.println("ConfigM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new ConfigMenu());                //11 <-

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(vmcm);                            //12 <- 

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(vmm);                             //13 <-

        //Serial.println("VMCM2 Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new ElementMenu());     //14

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(gscm);                            //15

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(gmctm);                           //16

        //Serial.println("HSCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(hscm);                            //17

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(hpco);                            //18

        //Serial.println("HCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(hcm);                             //19

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mmm);                             //20

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(mtm);                             //21

        //Serial.println("SMM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(smm);                             //22

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(smcm);                            //23

        //Serial.println("ESM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new EnableServerMenu());          //24

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new HygrometerGroupConfigMenu()); //25

        //Serial.println("WCM Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(new WateringConfigMenu());        //26

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        fGUI::AddMenu(hmcm);                            //27

        //Serial.println("Init Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

        fGUI::Init(e, 34);

        //Serial.println("Heap left: " + String(ESP.getFreeHeap()));
        //Serial.println("Largest  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));

        alertmenu = new AlertMenu("");
        alertmenuid = fGUI::AddMenu(alertmenu);


        //xTaskCreate(updateGUITask, "cnt_gui_modules", 4096, nullptr, 0, nullptr);
        
        fNETController::OnModuleConnected.AddHandler(new EventHandlerFunc([](void* param) {
            fNETController::Module* connected = (fNETController::Module*)param;
            DisplayAlert(connected->Config["name"], "Connected.");
            }));

        fNETController::OnModuleDisconnected.AddHandler(new EventHandlerFunc([](void* param) {
            fNETController::Module* connected = (fNETController::Module*)param;
            DisplayAlert(connected->Config["name"], "Disconnected.");
            }));
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
    static ElementMenu* icm;
    static HygrometerModulesMenu* hmm;
    static ValveModulesMenu* vmm;
    static ValveModuleConfigMenu* vmcm;
    static GreenhouseSizeConfigMenu* gscm;
    static GreenhouseModuleConfigTabsMenu* gmctm;
    static HygrometersConfigMenu* hscm;
    static HygrometerPositionConfigMenu* hpco;
    static HygrometerConfigMenu* hcm;
    static MoistureMonitorMenu* mmm;
    static MonitorTabsMenu* mtm;
    static SensorModulesMenu* smm;
    static SensorModuleConfigMenu* smcm;
    static HygrometerModuleConfigMenu* hmcm;


    static void updateGUITask(void* param) {
        while (true) {
            delay(1000);

            //scm->SetConfig(fNETController::data);
            AddModules();
        }
    }

    static AlertMenu* alertmenu;
    static int alertmenuid;

    static void DisplayAlert(String title, String message) {
        if (fGUI::CurrentOpenMenu != alertmenuid)
            fGUI::OpenMenu(alertmenuid);

        alertmenu->updateMessage(title, message);
    }

    /*
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
            else
                continue;

            
        }
    }*/
};



class ServerGUI : public ElementMenu {
    void InitializeElements() {
        AddElement(new TextElement("AtaTohum", width / 2, 20, 4, 1, TFT_GOLD));
        AddElement(new TextElement("MGMS Greenhouse", width / 2, 32, 0, 1, TFT_GOLD));

        AddElement(new TextElement("Server Enabled", width / 2, height / 2 - 32, 0, 1, TFT_GREEN));

        AddElement(new Button("Disable", width / 2, height / 2, 64, 32, 1, 1, TFT_BLACK, TFT_YELLOW, []() {fGMS::serverEnabled = false; fGMS::Save(); }, "Disable"));

        AddElement(new UncenteredTextElement("", 4, 132, 0, 1, TFT_WHITE));
        AddElement(new UncenteredTextElement("", 4, 140, 0, 1, TFT_WHITE));
        AddElement(new UncenteredTextElement("", 4, 148, 0, 1, TFT_WHITE));
    }

    void Draw() override {
        ElementMenu::Draw();

        float ms = millis();
        float seconds = ms / 1000;
        float minutes = seconds / 60.0;
        float hours = minutes / 60.0;
        float days = hours / 24.0;

        String uptime = String(floor(days), 0) + "d" + String(floor(fmod(hours, 24)), 0) + "h" + String(floor(fmod(minutes, 60)), 0) + "m" + String(floor(fmod(seconds, 60)), 0) + "s";

        ((TextElement*)Elements[4])->t = "Up : " + String(uptime);
        ((TextElement*)Elements[5])->t = "Heap : " + String(ESP.getFreeHeap());
        ((TextElement*)Elements[6])->t = "IP: " + WiFi.localIP().toString();
    }
};