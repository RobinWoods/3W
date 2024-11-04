#include "Captors.h"

#include <EEPROM.h>

void verifCaptors()
{
    if (errorCaptors.errorLum <= 1)
    {
        timeoutCounter = millis();
        do
        {
            luminosity = (analogRead(LUMINOSITY_CAPTOR)*100)/1023; // On prend le pourcentage de luminosité
        }
        while ((analogRead(LUMINOSITY_CAPTOR) == 1023) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000);

        luminosity = (analogRead(LUMINOSITY_CAPTOR) == 1023) ? 101: luminosity;
        errorCaptors.errorLum = (analogRead(LUMINOSITY_CAPTOR) == 1023) ? errorCaptors.errorLum + 1 : errorCaptors.errorLum;
    }

    //Humidity
    if (bme280.isConnected())
    {
        if (errorCaptors.errorHumidity <=1)
        {
            timeoutCounter = millis();
            do
            {
                humidity = bme280.getHumidity();
            }
            while((humidity < params.HYGR_MINT || humidity > params.HYGR_MAXT) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000);

            humidity = (humidity < params.HYGR_MINT || humidity > params.HYGR_MAXT) ? 101 : humidity;
            errorCaptors.errorHumidity = (humidity < params.HYGR_MINT || humidity > params.HYGR_MAXT) ? errorCaptors.errorHumidity + 1 : errorCaptors.errorHumidity;
        }

        //Pressure
        if (errorCaptors.errorPressure <=1)
        {
            timeoutCounter = millis();
            do
            {
                pressure = bme280.getPressure()/100;
            }
            while((pressure < params.PRESSURE_MIN || pressure > params.PRESSURE_MAX) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000);

            pressure = (pressure < params.PRESSURE_MIN || pressure > params.PRESSURE_MAX) ? NAN : pressure;
            errorCaptors.errorPressure = (pressure < params.PRESSURE_MIN || pressure > params.PRESSURE_MAX) ? errorCaptors.errorPressure + 1 : errorCaptors.errorPressure;
        }


        //Temperature
        if (errorCaptors.errorTemp <=1)
        {
            timeoutCounter = millis();
            do
            {
                temp = bme280.getTemperature() +41;
            }
            while ((temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000);

            temp= (temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) ? 0 : temp;
            errorCaptors.errorTemp =  (temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) ?  errorCaptors.errorTemp +1 : errorCaptors.errorTemp;
        }
    }
}

void getPosition()
{

    static bool goodTime;

    if (actualMod != ECO_MOD || (goodTime = !goodTime))
    {
        if (gpsSerial.available() && errorCaptors.errorGPS <= 1)
        {
            char buffer[27];
            timeoutCounter = millis();
            while (gpsSerial.available() && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000)
            {
                char c = gpsSerial.read();
                if (c == '$')
                {
                    for (byte i = 0; i <17; i++)
                    {
                        buffer[i] = gpsSerial.read();
                    }

                    if (buffer[2] == 'G' && buffer[3] == 'G' && buffer[4] == 'A')
                    {
                        for (byte i = 0; i <26; i++)
                        {
                            buffer[i] = gpsSerial.read();
                        }
                        gpsTrame = buffer;
                        //gpsTrame = (gpsTrame.substring(11, 12) == "S" ? "-" : "" ) + gpsTrame.substring(0, 4) + gpsTrame.substring(6,7) + (gpsTrame.substring(25, 26) == "W" ? "-" : "") + gpsTrame.substring(13, 19);
                        break;
                    }
                }
            }
        }
        errorCaptors.errorGPS++;
    }
}
