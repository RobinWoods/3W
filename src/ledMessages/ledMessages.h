//
// Created by Robin Derenty on 31/10/2024.
//

#ifndef LEDMESSAGES_H
#define LEDMESSAGES_H

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

#endif //LEDMESSAGES_H
