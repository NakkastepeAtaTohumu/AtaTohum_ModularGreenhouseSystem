#pragma once

#ifndef fGMS_h
#define fGMS_h

class fGMS_MoistureModule {
    class Hygrometer {
    public:
        float x = -1;
        float y = -1;

        int id = 0;

        bool ok = false;

        float value = -1;
    };

    class HygrometerGroup {
    public:
        int hygrometers[128];
        int numHygrometers = 0;

        int color = 0x404040;

        void AddHygrometer(int h) {
            hygrometers[numHygrometers++] = h;
        }
    };

    static Hygrometer* Hygrometers[1024];
    static int HygrometerCount;

    static HygrometerGroup* HygrometerGroups[16];
    static int HygrometerGroupCount;

    static Hygrometer* GetHygrometer(int id) {
        if (id < HygrometerCount)
            return Hygrometers[id];

        return nullptr;
    }

    static Hygrometer* CreateHygrometer() {
        Hygrometer* h = new Hygrometer();
        Hygrometers[HygrometerCount++] = h;

        return h;
    }

    static HygrometerGroup* CreateHygrometerGroup() {
        HygrometerGroup* g = new HygrometerGroup();
        HygrometerGroups[HygrometerGroupCount++] = g;
         
        return g;
    }

    static HygrometerGroup* GetHygrometerGroup(int id) {
        if (id < HygrometerGroupCount)
            return HygrometerGroups[id];

        return nullptr;
    }

    static DynamicJsonDocument data;

    static void Save() {
        Serial.println("[fGMS] Saving...");
        JsonArray hygrometersArray = data.createNestedArray("hygrometers");

        for (int i = 0; i < HygrometerCount; i++) {
            JsonObject o = hygrometersArray.createNestedObject();
            Hygrometer* h = Hygrometers[i];

            o["x"] = h->x;
            o["y"] = h->y;
            o["ok"] = h->ok;
        }

        JsonArray groupsArray = data.createNestedArray("groups");

        for (int i = 0; i < HygrometerGroupCount; i++) {
            JsonObject o = groupsArray.createNestedObject();
            HygrometerGroup* g = HygrometerGroups[i];

            JsonArray hygros = o.createNestedArray("hygrometers");

            for (int j = 0; j < g->numHygrometers; j++)
                hygros.add(g->hygrometers[j]);

            o["color"] = g->color;
        }

        String data_serialized;
        serializeJson(data, data_serialized);

        Serial.println("New serialized data: \n" + data_serialized);

        File data_file = LittleFS.open("/fGMS_Data.json", "w", true);
        data_file.print(data_serialized.c_str());
        data_file.close();

        Serial.println("[fGMS] Saved.");
    }

    static void ReadConfig() {
        bool mounted = LittleFS.begin();

        if (!mounted)
            return;

        File data_file = LittleFS.open("/fGMS_Data.json", "r");

        if (!data_file)
            return;

        String data_rawJson = data_file.readString();
        deserializeJson(data, data_rawJson);
        Serial.println("[fGMS] Read config: " + data_rawJson);


        JsonArray hygrometersArray = data["hygrometers"];
        HygrometerCount = 0;

        for (JsonObject o : hygrometersArray) {
            Hygrometer* h = CreateHygrometer();
            h->x = o["x"].as<float>();
            h->y = o["y"].as<float>();
            h->ok = o["ok"].as<bool>();
        }

        JsonArray groupsArray = data["groups"];
        HygrometerGroupCount = 0;

        for (JsonObject o : groupsArray) {
            HygrometerGroup* h = CreateHygrometerGroup();
            JsonArray idsArray = data["hygrometers"];

            for (int i : idsArray)
                h->hygrometers[h->numHygrometers++] = i;

            h->color = o["color"].as<int>();
        }

        Serial.println("[fGMS] Loaded");
    }
};

#endif // !fGMS_h
