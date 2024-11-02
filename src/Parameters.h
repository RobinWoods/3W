#ifndef PARAMETERS_H
#define PARAMETERS_H
#include <Arduino.h>
#include <RTClib.h>
#include <Seeed_BME280.h>

struct Parameters {
    byte LOG_INTERVALL;
    int FILE_MAX_SIZE;

    bool LUMIN;
    int LUMIN_LOW;
    int LUMIN_HIGH;

    bool TEMP_AIR;
    int MIN_TEMP_AIR;
    int MAX_TEMP_AIR;

    bool HYGR;
    int HYGR_MINT;
    int HYGR_MAXT;

    bool PRESSURE;
    int PRESSURE_MIN;
    int PRESSURE_MAX;

    uint8_t TIMEOUT;
};

struct ErrorCaptors
{
    byte errorLum;
    byte errorPressure;
    byte errorHumidity;
    byte errorTemp;
    byte errorGPS;
};

#endif //PARAMETERS_H
