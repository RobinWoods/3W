#ifndef SDUTILS_H
#define SDUTILS_H
#include "Parameters.h"

#define CHIP_SELECT 4
#define MAINTENANCE_MOD 2

extern RTC_DS1307 rtc;
extern volatile byte actualMod;

extern Parameters params;

extern byte luminosity, humidity, temp;
extern float pressure;
extern char bufferGPS[27];

void writeInFile();
void createFile();

#endif //SDUTILS_H
