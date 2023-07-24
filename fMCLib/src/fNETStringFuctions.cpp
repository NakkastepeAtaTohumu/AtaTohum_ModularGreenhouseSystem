#include "fNETStringFunctions.h"

String Padded(String data, int len) {
    while (data.length() < len)
        data.concat("#");

    return data;
}

String PaddedInt(int i, int len) {
    return Padded(String(i), len);
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
