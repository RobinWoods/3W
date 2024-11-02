#include "Captors.h"

#include <EEPROM.h>

void verifCaptors()
{
    if (errorCaptors.errorLum <= 1) // This captor is on an analogPin so it always send a data, we can't use the timeout for this
    {
        timeoutCounter = millis();
        luminosity = 1023;
        while ((luminosity < params.LUMIN_LOW || luminosity > params.LUMIN_HIGH) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000){
            luminosity = analogRead(LUMINOSITY_CAPTOR); // On prend le pourcentage de luminosité
        }

        luminosity = (luminosity < 0 || luminosity >= 1023) ? 101 : luminosity;
        errorCaptors.errorLum = (luminosity < 0 || luminosity >= 1023) ? errorCaptors.errorLum + 1 : errorCaptors.errorLum;
    }

    //Humidity
    if (bme280.isConnected())
    {
        if (errorCaptors.errorHumidity <=1)
        {
            timeoutCounter = millis();
            humidity=101;
            while((humidity < params.HYGR_MINT || humidity > params.HYGR_MAXT) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000)
            {
                humidity = bme280.getHumidity();
            }
            if(humidity < params.HYGR_MINT || humidity > params.HYGR_MAXT)
            {
                humidity=101;
                errorCaptors.errorHumidity++;
            }
        }

        //Pressure
        if (errorCaptors.errorPressure <=1)
        {
            timeoutCounter = millis();
            pressure = 299;
            while((pressure < params.PRESSURE_MIN || pressure > params.PRESSURE_MAX) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000)
            {
                pressure = bme280.getPressure()/100;
            }
            if(pressure < params.PRESSURE_MIN || pressure > params.PRESSURE_MAX)
            {
                pressure = NAN;
                errorCaptors.errorPressure++;
            }
        }


        //Temperature
        if (errorCaptors.errorTemp <=1)
        {
            timeoutCounter = millis();
            temp=0;
            while ((temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000)
            {
                temp = bme280.getTemperature() +41;
            }
            temp= (temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) ? 0 : temp;
            errorCaptors.errorTemp =  (temp < params.MIN_TEMP_AIR +41 || temp > params.MAX_TEMP_AIR +41) ?  errorCaptors.errorTemp +1 : errorCaptors.errorTemp;
        }
    }
}

void getPosition()
{
    timeoutCounter = millis();
    while (!gpsSerial.available()) {}
    while (gpsSerial.available() && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000 )
    {
        gpsTrame = gpsSerial.readStringUntil('$');
        if (gpsTrame.startsWith("GNGLL", 0) && !gpsTrame.startsWith("BDGSV", 0)) break;
    }
    errorCaptors.errorGPS = !(gpsTrame.startsWith("GNGLL", 0) && !gpsTrame.startsWith("BDGSV", 0)) ? errorCaptors.errorGPS + 1 : errorCaptors.errorGPS;

}
