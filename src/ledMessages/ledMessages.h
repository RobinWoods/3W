//
// Created by Robin Derenty on 31/10/2024.
//

#ifndef LEDMESSAGES_H
#define LEDMESSAGES_H

#define MAINTENANCE_MOD 2
#include <Arduino.h>
#include <SD.h>

void standardModLed();
void configModLed();
void ecoModLed();
void maintenanceModLed();

void errorAccessRTC();
void errorAccessGPS();
void errorAccessCaptor();
void errorDataCaptorIllogical();
void errorSDFull();
void errorAccessOrWriteSD();

extern volatile byte actualMod;
extern File file;

#endif //LEDMESSAGES_H
