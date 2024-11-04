#include "Captors.h"

#include <EEPROM.h>

void verifCaptors()
{

    // On vérifie chaque capteurs de la même façon

    if (errorCaptors.errorLum <= 1) // On vérifie que le capteur n'est pas considéré en défault
    {
        timeoutCounter = millis(); // On initialise l'instant t de lancement de la lecture du capteur
        do // On fait tourner au moins une fois la lecture
        {
            luminosity = (analogRead(LUMINOSITY_CAPTOR)*100)/1023; // On prend le pourcentage de luminosité
        }
        while ((analogRead(LUMINOSITY_CAPTOR) == 1023) && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000); // Tant que la lecture ne correspond pas aux valeurs possibles et que le temps écoulé est inférieur au timeout alors on essaie de lire

        luminosity = (analogRead(LUMINOSITY_CAPTOR) == 1023) ? 101: luminosity; // Si la valeur est fausse on écrit une valeur spéciale
        errorCaptors.errorLum = (analogRead(LUMINOSITY_CAPTOR) == 1023) ? errorCaptors.errorLum + 1 : errorCaptors.errorLum; // On incrémente l'erreur du capteur
    }

    //Humidity
    if (bme280.isConnected()) // On vérifie que le capteur est branché pour prévenir des bugs
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

    static bool goodTime; // variable pour ne faire le traitement qu'une fois sur 2 en mode éco

    if (actualMod != ECO_MOD || (goodTime = !goodTime)) // On regarde si l'on n'est pas en mode eco, si oui alors on inverse la valeur de la variable, ce qui une fois sur 2 sera true et lancera l'exécution. Sinon on lance le code
    {
        if (gpsSerial.available() && errorCaptors.errorGPS <= 1) // On vérifie qu'on a accès au capteur et qu'il n'est pas en défaut
        {
            char buffer[27]; // On créer un buffer pour la trame
            timeoutCounter = millis();
            while (gpsSerial.available() && millis() - timeoutCounter <= (long)params.TIMEOUT * 1000)
            {
                char c = gpsSerial.read(); // on lit ce qu'envoi le GPS
                if (c == '$') //Si c'est le début d'une trame NMEA alors on regarde laquelle en écrivant dans le buffer le début de la trame
                {
                    for (byte i = 0; i <17; i++)
                    {
                        buffer[i] = gpsSerial.read();
                    }

                    if (buffer[2] == 'G' && buffer[3] == 'G' && buffer[4] == 'A') // On vérifie que c'est bien la trame GNGGA
                    {
                        for (byte i = 0; i <26; i++)
                        {
                            buffer[i] = gpsSerial.read(); // On réecris dans le buffer les informations de positions
                        }
                        gpsTrame = buffer; //On écrit la valeur du buffer dans une string
                        //gpsTrame = (gpsTrame.substring(11, 12) == "S" ? "-" : "" ) + gpsTrame.substring(0, 4) + gpsTrame.substring(6,7) + (gpsTrame.substring(25, 26) == "W" ? "-" : "") + gpsTrame.substring(13, 19);
                        break;//On sort de la boucle
                    }
                }
            }
        }
        errorCaptors.errorGPS++;
    }
}
