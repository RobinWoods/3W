#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <Seeed_BME280.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include "Parameters.h"

#include "SDUtils/SDUtils.h"
extern File file;
extern char fileName[13];
#include "ledMessages/ledMessages.h"
#include "Captors/Captors.h"

#define RED_BUTTON_PIN 3
#define GREEN_BUTTON_PIN 2
#define LUMINOSITY_CAPTOR A0

#define STANDARD_MOD 0
#define CONFIG_MOD 1
#define MAINTENANCE_MOD 2
#define ECO_MOD 3
#define MAGIC_WORD_DEFAULT 123
#define MAGIC_WORD_ACTUAL 213

#define TIME_FOR_STOP_CONFIG_MOD 1800000
#define VERSION 00001

RTC_DS1307 rtc;
SoftwareSerial gpsSerial(5,6);
BME280 bme280;

volatile byte actualMod;
volatile byte lastMod;
volatile byte varCompteur = 0;
volatile byte secondCounter = 0;
uint16_t overflowCounter;
bool flag = false;

void configMod();

void getEEPROMParams();

byte luminosity, humidity, temp;
float pressure;
String gpsTrame = "";
unsigned long timeoutCounter;

Parameters params;
ErrorCaptors errorCaptors;


void changeMod()
{
    cli();
    TIMSK2 = 0b00000001;      // Autoriser l'interruption locale du timer
    bitClear (TCCR2A, WGM20); // WGM20 = 0
    bitClear (TCCR2A, WGM21); // WGM21 = 0
    sei();
    varCompteur = 0;
    secondCounter = 0;
}

void setup() {
    Serial.begin(9600);
    gpsSerial.begin(9600);

    PORTD |= (1 << PD2); // Set pullUp resistance on pin 2
    PORTD |= (1 << PD3); // Set pullUp resistance on pin 3

    pinMode(A0, INPUT_PULLUP);
    //PORTC |= (1 << PC0); // Set pullUp resistance on pin A0

    cli(); // Stop interrupts

    TCCR2B = 0b00000110; // Clock / 256 soit 16 micro-s et WGM22 = 0

    TCCR1A = 0;              // Mode normal
    TCCR1B = (1 << CS12) | (1 << CS10); // Prescaler de 1024
    TCNT1 = 0;               // Compteur à 0
    TIMSK1 = (1 << TOIE1);   // Active l'interruption de débordement pour Timer1

    sei(); // Restart Interrupts

    attachInterrupt(digitalPinToInterrupt(2), changeMod, FALLING);
    attachInterrupt(digitalPinToInterrupt(3), changeMod, FALLING);

    bme280.init();
    rtc.begin();

    getEEPROMParams();

    if(digitalRead(3) == LOW)
    {
        actualMod = CONFIG_MOD;
        configMod();
    }
    actualMod = STANDARD_MOD;
    createFile();
    flag= true;
}



void loop() {

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
        //Serial.println("Captors ...");
        verifCaptors();
        //Serial.println("GPS ...");
        getPosition();
        //Serial.println("Writing ...");
        writeInFile();
        //Serial.println("Done");
        Serial.flush();
        overflowCounter = 0;
        flag = false;
    }
}

ISR(TIMER2_OVF_vect) {
    TCNT2 = 256 - 250; // 250 x 16 µS = 4 ms
    if (varCompteur++ > 250)
    {
        varCompteur = 0;
        secondCounter++;
        if (secondCounter >= 5)
        {
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
                    //file.close();
                    SD.end();
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
                    SD.begin(CHIP_SELECT);
                    file = SD.open(fileName, FILE_WRITE);
                }
            }
            cli();
            TIMSK2 = 0b00000000;
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
    else if (overflowCounter >= 15 *(actualMod == ECO_MOD ? 2* params.LOG_INTERVALL : params.LOG_INTERVALL)) // On attends x min ou x *2 min pour le mode eco
    {
        flag = true;
    }
}


void configMod()
{
    configModLed();
    overflowCounter = 0;

    Serial.flush();

    Serial.println("Config Mod");

    while (1)
    {
        EEPROM.write(0, MAGIC_WORD_ACTUAL);
        while (!Serial.available()){}
        String input = Serial.readStringUntil('\n');
        String paramName = input.substring(0, input.indexOf('='));
        int value = input.substring(input.indexOf('=') + 1).toInt();

        Serial.println(input);

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
            EEPROM.get(1, params);
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
            else if (paramName.equals("DAY"))
            {
            }

        }
        else if (paramName.equals("EXIT"))
        {
            Serial.flush();
            EEPROM.put(sizeof(Parameters) +1, params);
            break;
        }
        else
        {
            Serial.println(F("Unrecognised Command"));
        }
        Serial.flush();
        EEPROM.write(0, 213);
        overflowCounter = 0;
    }
}

void getEEPROMParams()
{
    EEPROM.get((EEPROM.read(0) == MAGIC_WORD_DEFAULT) ? 1 : 25, params);
}