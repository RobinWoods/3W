// Import des librairies
#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <Seeed_BME280.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Imports des fichiers pour les structures utilisées et les fonctions relatives à la carte SD, aux leds et capteurs
#include "Parameters.h"
#include "SDUtils/SDUtils.h"
#include "ledMessages/ledMessages.h"
#include "Captors/Captors.h"

//Déclaration de variables global d'un autre fichier pour résoudre le nom des variables pendant l'édition des liens
extern File file;
extern char fileName[13];

//Création de MACRO pour une meilleure lisibilité, on utilise des bytes pour par exemple définir le mode actuel
#define RED_BUTTON_PIN 3
#define GREEN_BUTTON_PIN 2
#define LUMINOSITY_CAPTOR A0

#define STANDARD_MOD 0
#define CONFIG_MOD 1
#define MAINTENANCE_MOD 2
#define ECO_MOD 3

// Clés pour identifier les paramètres stockés dans l'EEPROM
#define MAGIC_WORD_DEFAULT 123
#define MAGIC_WORD_ACTUAL 213

#define TIME_FOR_STOP_CONFIG_MOD 1800000
#define VERSION 00001

//Déclaration des objets relatifs à l'horloge, le gps et les capteurs pour pouvoir utiliser leurs méthodes

RTC_DS1307 rtc;
SoftwareSerial gpsSerial(5,6);
BME280 bme280;


// Déclaration des variables globales pour gérer les modes et compteurs
volatile byte actualMod; //variable
volatile byte lastMod;
volatile byte varCompteur = 0;
volatile byte secondCounter = 0;
uint16_t overflowCounter;
bool flag = false;

//Prototypes de fonctions pour pouvoir les définir après les appels
void configMod();
void getEEPROMParams();

// Déclarations des variables globales pour les capteurs et le compteur de timeout
byte luminosity, humidity, temp;
float pressure;
unsigned long timeoutCounter;

//Structures pour les paramètres et les erreurs
Parameters params;
ErrorCaptors errorCaptors;


void changeMod()
{
    cli();
    TIMSK2 = 0b00000001;      // Autoriser l'interruption locale du timer
    bitClear (TCCR2A, WGM20); // Réglage du mode du timer
    bitClear (TCCR2A, WGM21); // Réglage du mode du timer
    sei();
    //Compteur à 0
    varCompteur = 0;
    secondCounter = 0;
}

void setup() {
    Serial.begin(9600); //Démarrage de la Com Série
    gpsSerial.begin(9600); // Démarrage de la Com Série virtuelle pour le GPS

    PORTD |= (1 << PD2); // Résistance pull-up sur la broche 2
    PORTD |= (1 << PD3); // Résistance pull-up sur la broche 3
    PORTC |= (1 << PC0); // Résistance pull-up sur la broche A0

    cli(); // Stop interrupts

    TCCR2B = 0b00000110; // Configurer le timer avec une horloge de 256

    TCCR1A = 0;              // Mode normal
    TCCR1B = (1 << CS12) | (1 << CS10); // Prescaler de 1024
    TCNT1 = 0;               // Compteur à 0
    TIMSK1 = (1 << TOIE1);   // Active l'interruption de débordement pour Timer1

    sei(); // Restart Interrupts

    //Déclaration des interruptions externes sur les boutons
    attachInterrupt(digitalPinToInterrupt(2), changeMod, FALLING);
    attachInterrupt(digitalPinToInterrupt(3), changeMod, FALLING);

    //Initialisation des capteurs et de l'horloge
    bme280.init();
    rtc.begin();

    getEEPROMParams(); // Récupération des paramètres dans l'EEPROM

    if(digitalRead(3) == LOW) // Si le bouton rouge est préssé au démarrage on lance le mode configuration
    {
        actualMod = CONFIG_MOD;
        configMod();
    }
    actualMod = STANDARD_MOD; //On lance le mode standard soit après la fin du mode config ou au démarrage
    createFile(); // On créer le premier fichier pour enregistrer les données
    flag= true; // On lance un enregistrement au démarrage
}



void loop() {
    //On vérifie les erreurs et les accès pour les capteurs puis si tout est bon on affiche la couleur correspondant au mode actuel
    if (errorCaptors.errorLum >= 2 || errorCaptors.errorHumidity >= 2 || errorCaptors.errorPressure >= 2 || errorCaptors.errorTemp >= 2)
    {
        errorDataCaptorIllogical();
    }
    else if(analogRead(LUMINOSITY_CAPTOR) == 1023 || !bme280.isConnected())
    {
        errorAccessCaptor();
    }
    else if (!SD.isConnected() && actualMod != MAINTENANCE_MOD)
    {
        errorAccessOrWriteSD();
    }
    else if (!rtc.isrunning())
    {
        errorAccessRTC();
    }
    else if (!gpsSerial.available())
    {
        errorAccessGPS();
    }
    else if (actualMod == STANDARD_MOD)
    {
        standardModLed();
    }
    else if (actualMod == MAINTENANCE_MOD)
    {
        maintenanceModLed();
    }
    else if (actualMod == ECO_MOD)
    {
        ecoModLed();
    }


    if (flag)
    {
        verifCaptors(); // On récupère les valeurs des capteurs
        //Serial.println("GPS");
        getPosition(); // On récupère la position
        //Serial.println("Writing");
        writeInFile(); // On écrit
        //Serial.println("Done");
        Serial.flush();
        overflowCounter = 0; // On remet le timer et le flag à 0
        flag = false;
    }
}

ISR(TIMER2_OVF_vect) {
    TCNT2 = 256 - 250; // 250 x 16 µS = 4 ms
    if (varCompteur++ > 250) // On regarde si une seconde s'est écoulée (4ms * 250 = 1s)
    {
        varCompteur = 0; // On remet le compteur à 0
        secondCounter++; // On incrémente le compteur de secondes
        if (secondCounter >= 5)
        {
            //Si un des boutons est toujours appuyé après les 5s alors on change la variable du mode actuel
            if (digitalRead(GREEN_BUTTON_PIN) == LOW)
            {
                if (actualMod == STANDARD_MOD)
                {
                    actualMod = ECO_MOD;
                }
                else if (actualMod == ECO_MOD)
                {
                    actualMod = STANDARD_MOD;
                }
            }
            else if (digitalRead(RED_BUTTON_PIN) == LOW)
            {
                if (actualMod == STANDARD_MOD || actualMod == ECO_MOD)
                {
                    lastMod = actualMod;
                    SD.end(); // On ferme la carte SD au lancement du mode config
                    actualMod = MAINTENANCE_MOD;
                }
                else if (actualMod == MAINTENANCE_MOD)
                {
                    if (lastMod == STANDARD_MOD)
                    {
                        actualMod = STANDARD_MOD;
                    }
                    else
                    {
                        actualMod = ECO_MOD;
                    }
                    SD.begin(CHIP_SELECT); // On ré-ouvre la SD quand on sort du mode config
                    file = SD.open(fileName, FILE_WRITE);
                }
            }
            cli();
            TIMSK2 = 0b00000000; // On désactive les interruptions
            sei();
            secondCounter=0;
        }
    }
}

ISR(TIMER1_OVF_vect) {
    overflowCounter++;
    if (overflowCounter >= 375 && actualMod == CONFIG_MOD) // On attend 30 min
    {
        EEPROM.put(sizeof(Parameters) + 1 , params); // On écrit une seule fois dans l'EEPROM
        actualMod = STANDARD_MOD;
    }
    else if (overflowCounter >= 4 /*(actualMod == ECO_MOD ? 2* params.LOG_INTERVALL : params.LOG_INTERVALL)*/) // On attends x min ou x * 2 min pour le mode eco
    {
        flag = true; //Toutes les x mins on lève le flag ce qui lancera une séquence de mesures dans la boucle loop()
    }
}

// Mode configuration pour ajuster les paramètres
void configMod()
{
    configModLed();
    overflowCounter = 0;

    Serial.flush();

    Serial.println("Config Mod");

    while (1)
    {
        EEPROM.write(0, MAGIC_WORD_ACTUAL);// Indiquer que les paramètres sont modifiés
        while (!Serial.available()){}
        while (!Serial.available()){}
        String input = Serial.readStringUntil('\n');
        String paramName = input.substring(0, input.indexOf('='));
        int value = input.substring(input.indexOf('=') + 1).toInt();

        Serial.println(input);

        // Affectation des paramètres selon la commande reçue si les valeurs sont cohérentes
        if (paramName.equals("LUMIN") && (value == 0 || value == 1)) params.LUMIN = value;

        else if (paramName.equals("LUMIN_LOW") && (value >= 0 && value <= 1023)) params.LUMIN_LOW = value;

        else if (paramName.equals("LUMIN_HIGH") && (value >= 0 && value <= 1023)) params.LUMIN_HIGH = value;

        else if (paramName.equals("TEMP_AIR") && (value == 0 || value == 1)) params.TEMP_AIR = value;

        else if (paramName.equals("MIN_TEMP_AIR") && (value >= -40 && value <= 85)) params.MIN_TEMP_AIR = value;

        else if (paramName.equals("MAX_TEMP_AIR") && (value >= -40 && value <= 85)) params.MAX_TEMP_AIR = value;

        else if (paramName.equals("HYGR") && (value == 0 || value == 1)) params.HYGR = value;

        else if (paramName.equals("HYGR_MINT") && (value >= -40 && value <= 85)) params.HYGR_MINT = value;

        else if (paramName.equals("HYGR_MAXT") && (value >= -40 && value <= 85)) params.HYGR_MAXT = value;

        else if (paramName.equals("PRESSURE") && (value == 0 || value == 1)) params.PRESSURE = value;

        else if (paramName.equals("PRESSURE_MIN") && (value >= 300 && value <= 1100)) params.PRESSURE_MIN = value;

        else if (paramName.equals("PRESSURE_MAX") && (value >= 300 && value <= 1100)) params.PRESSURE_MAX = value;

        else if (paramName.equals("LOG_INTERVALL") && (value >= 1 && value <= 255)) params.LOG_INTERVALL = value;

        else if (paramName.equals("FILE_MAX_SIZE") && (value >= 1024 && value <= 8192)) params.FILE_MAX_SIZE = value;

        else if (paramName.equals("RESET"))
        {
            EEPROM.get(1, params);//On recharge les paramètres par defaut
            EEPROM.write(0, MAGIC_WORD_DEFAULT);
        }
        else if (paramName.equals("VERSION"))
        {
            Serial.print(F("Version : "));
            Serial.println(VERSION);
        }
        else if (paramName.equals("TIMEOUT")) params.TIMEOUT = (value >= 30 && value <= 120) ? value : params.TIMEOUT;

        else if (paramName.equals("CLOCK") || paramName.equals("DATE") || paramName.equals("DAY"))
        {
            String stringValue = input.substring(input.indexOf('=') + 1);

            if (paramName.equals("CLOCK"))
            {
                byte hour = stringValue.substring(0, stringValue.indexOf(':')).toInt();
                stringValue = stringValue.substring(stringValue.indexOf(':') + 1);
                byte minutes =  stringValue.substring(0, stringValue.indexOf(':')).toInt();
                stringValue = stringValue.substring(stringValue.indexOf(':') + 1);
                byte seconds =  stringValue.substring(0, stringValue.indexOf(':')).toInt();
                rtc.adjust(DateTime(rtc.now().year(),rtc.now().month(), rtc.now().day(), hour, minutes, seconds));
            }
            else if (paramName.equals("DATE"))
            {
                byte year = stringValue.substring(0, stringValue.indexOf(',')).toInt();
                stringValue = stringValue.substring(stringValue.indexOf(',') + 1);
                byte month =  stringValue.substring(0, stringValue.indexOf(',')).toInt();
                stringValue = stringValue.substring(stringValue.indexOf(',') + 1);
                byte day =  stringValue.substring(0, stringValue.indexOf(',')).toInt();
                rtc.adjust(DateTime(year, month, day, rtc.now().hour(), rtc.now().minute(), rtc.now().second()));

            }
        }
        else if (paramName.equals("EXIT"))
        {
            Serial.flush();
            EEPROM.put(sizeof(Parameters) +1, params);//On écrit les paramètres modifiées dans l'EEPROM
            break; // On sort du mode
        }
        else
        {
            Serial.println(F("Unrecognised Command"));
        }
        Serial.flush();
        overflowCounter = 0;
    }
}

void getEEPROMParams()
{
    EEPROM.get((EEPROM.read(0) == MAGIC_WORD_DEFAULT) ? 1 : 25, params); // En fonction de la valeur de la clé, on charge les paramètres par défault ou ceux modifié
}