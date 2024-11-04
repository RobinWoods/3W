#include <Arduino.h>
#include <EEPROM.h>

#define MAGIC_WORD 123 //On définit la clé par défault


struct Parameters { // On utilise la même structure que dans le code de la station météo
    byte LOG_INTERVALL = 10;
    int FILE_MAX_SIZE = 2048;

    bool LUMIN = 1;
    int LUMIN_LOW = 255;
    int LUMIN_HIGH = 768;

    bool TEMP_AIR = 1;
    int MIN_TEMP_AIR = -10;
    int MAX_TEMP_AIR = 60;

    bool HYGR = 1;
    int HYGR_MINT = 0;
    int HYGR_MAXT = 65;

    bool PRESSURE = 1;
    int PRESSURE_MIN = 850;
    int PRESSURE_MAX = 1080;

    uint8_t TIMEOUT = 30;
};

Parameters params;

void clearEEPROM() {
    for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0xFF);
    }
}

void setup() {
    clearEEPROM();//On clear l'EEPROM pour être sûr de ne pas avoir de problème de ré écriture
    Serial.begin(9600);
    Serial.println(F("Uploading..."));
    if (EEPROM.read(0) != MAGIC_WORD)
    {
        EEPROM.write(0, MAGIC_WORD);
        EEPROM.put(1, params);
    }
    Serial.println(F("Uploaded"));

    Parameters EEPROMParams;

    EEPROM.get(1, EEPROMParams); // On récupère les valeurs mise dans l'EEPROM pour vérifier l'écriture

    Serial.println((EEPROMParams.HYGR_MAXT == params.HYGR_MAXT) ? "Perfect" : "Bug"); // On vérifie avec une des valeurs
    Serial.println(sizeof(Parameters)+1); //On écrit la première adresse libre de l'EEPROM après l'écriture
}

void loop() {
}