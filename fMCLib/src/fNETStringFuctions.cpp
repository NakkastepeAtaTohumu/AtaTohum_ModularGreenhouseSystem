#include "fNETStringFunctions.h"

String PaddedInt(int i, int len) {
    String data = String(i);

    while (data.length() < len)
        data.concat("#");

    return data;
}

int ParsePaddedInt(String data) {
    String d;
    for (int i = 0; i < data.length(); i++) {
        if (data[i] == '#')
            break;

        d += data[i];
    }

    return atoi(d.c_str());
}
