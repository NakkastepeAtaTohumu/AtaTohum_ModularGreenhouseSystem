#pragma once

#include <Vector.h>

enum fGMSLogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class fGMSLogEntry {
    String source;
    String log;
    fGMSLogLevel level;

    int sourceDeviceID;

    fGMSLogEntry() {
        sourceDeviceID = -1;
    }

    fGMSLogEntry(String src, String l, fGMSLogLevel lvl, int srcDevice) {
        source = src;
        log = l;
        level = lvl;
        sourceDeviceID = srcDevice;
    }
};

class fGMSLogs {
    static Vector<fGMSLogEntry> entries;

    static void addEntry(fGMSLogEntry e) {
        entries.push_back(fGMSLogEntry(e));

        if (onLogAdded != nullptr)
            onLogAdded(e);
    }

    static void(*onLogAdded)(fGMSLogEntry);
};