#include "ledMessages.h"
#include <ChainableLEDDIY.h>

ChainableLED led(8, 9);

void standardModLed(){
    led.setColorRGB(0, 255, 0); // Green
}

void configModLed()
{
    //led.setColorRGB(255, 255, 0); // Yellow
    led.setColorRGB(255, 0, 0);//Parce que Robin ne voit rien
}

void ecoModLed()
{
    led.setColorRGB(0, 0, 255); // Blue
}

void maintenanceModLed()
{
    led.setColorRGB(255, 60, 0); // Orange
}

void errorAccessRTC()
{
    int cycle = millis() % 1000;
    led.setColorRGB((cycle > 500) ? 255 : 0, 0,(cycle > 500) ? 0 : 255 );
}

void errorAccessGPS()
{
    int cycle = millis() % 1000; // On créer un cycle d'une seconde
    led.setColorRGB(255, (cycle > 500) ? 0 : 255, 0);
}

void errorAccessCaptor()
{
    int cycle = millis() % 1000;
    led.setColorRGB((cycle > 500) ? 255 : 0, (cycle > 500) ? 0 : 255, 0);
}

void errorDataCaptorIllogical()
{
    int cycle = millis() % 1000;
    led.setColorRGB((cycle < 333) ? 255 : 0, (cycle < 333) ? 0 : 255, 0);
}

void errorSDFull()
{
    while(!file && actualMod != MAINTENANCE_MOD)
    {
        int cycle = millis() % 1000;
        led.setColorRGB(255, (cycle > 500) ? 255 : 0, (cycle > 500) ? 255 : 0);
    }
}

void errorAccessOrWriteSD()
{
    int cycle = millis() % 1000;
    led.setColorRGB(255, (cycle > 333) ? 255 : 0, (cycle > 333) ? 255 : 0);
}