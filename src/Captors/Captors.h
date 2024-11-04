#ifndef CAPTORS_H
#define CAPTORS_H
#include "Parameters.h"
#include <RTClib.h>
#include <Seeed_BME280.h>
#include <SoftwareSerial.h>

#define LUMINOSITY_CAPTOR A0
#define ECO_MOD 3

extern ErrorCaptors errorCaptors;
extern Parameters params;

extern RTC_DS1307 rtc;
extern SoftwareSerial gpsSerial;
extern BME280 bme280;

extern volatile byte actualMod;

extern byte luminosity, humidity, temp;
extern float pressure;
extern String gpsTrame;

extern unsigned long timeoutCounter;

void verifCaptors();
void getPosition();

#endif //CAPTORS_H
