#pragma once

#ifndef fNETStringFunctions_h
#define fNETStringFunctions_h

#include "Arduino.h"

String Padded(String data, int len);
String PaddedInt(int i, int len);

int ParsePaddedInt(String data);

#endif