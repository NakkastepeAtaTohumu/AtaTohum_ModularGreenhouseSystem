#pragma once

#ifndef fNETStringFunctions_h
#define fNETStringFunctions_h

#include "Arduino.h"

String PaddedInt(int i, int len);

int ParsePaddedInt(String data);

#endif