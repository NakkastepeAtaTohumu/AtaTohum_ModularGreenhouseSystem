#pragma once

#ifndef fGMSStringFunctions_h
#define fGMSStringFunctions_h

#include "Arduino.h"

String PaddedInt(int i, int len);

int ParsePaddedInt(String data);

#endif