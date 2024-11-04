#include "SDUtils.h"
#include <Arduino.h>
#include "../ledMessages/ledMessages.h"
#include <SD.h>
#include <Captors/Captors.h>

File file;
char fileName[13];

void assignFileName()
{
    /*
    Prend l'adresse du premier élément d'un tableau
    Écrit dans ce tableau le nom du fichier au format 8.3 de telle façon :
    AAMMJJXX.CSV avec AA les deux derniers chiffres de l'année, MM le mois, JJ le jour et
    XX un indice de 0 à 99 qui est rénitialisé à chaque nouveau jour

    */


    if (!rtc.begin())
    {
        errorAccessRTC();
    }

    DateTime date = rtc.now();
    unsigned short int tmp;
    static byte index = 1; // ne dépasse pas 99
    static byte previousDay = 0; // ne dépasse pas 31
    // datage au format YYMMJJ grâce aux outils modulo "%" et division entière "/"
    tmp = date.year() % 100; // 2024 % 100 = 24
    fileName[0] = '0' + (tmp / 10); // "'0' + int" convertit le chiffre en ASCII
    fileName[1] = '0' + (tmp % 10);
    tmp = date.month();
    fileName[2] = '0' + (tmp / 10);
    fileName[3] = '0' + (tmp % 10);
    tmp = date.day();
    fileName[4] = '0' + (tmp / 10);
    fileName[5] = '0' + (tmp % 10);
    // indexage
    if (tmp != previousDay) {index = 1;} // rénitialise l'index si le jour a changé
    fileName[6] = '0' + (index / 10);
    fileName[7] = '0' + (index % 10);
    // .extension
    fileName[8] = '.';
    fileName[9] = 'C';
    fileName[10] = 'S';
    fileName[11] = 'V';
    fileName[12] = '\0'; // caractère de fin de chaîne
    // incrémente l'index et actualise le jour
    if (index == 99) {index = 1;} else {index += 1;}
    previousDay = tmp;
}
void createFile()
{
    /*
    Créé puis ouvre un fichier au nom de type AAMMJJXX.CSV et
    y écrit le header "date;lumin;temp;hygr;pressure;latitude;longitude"
    Renvoie une erreur si le fichier ne s'ouvre pas
    */
    SD.begin(CHIP_SELECT);

    assignFileName();
    if (SD.exists(fileName)) {SD.remove(fileName);} // écrase le potentiel fichier du même nom
    file = SD.open(fileName, FILE_WRITE); // ouvre le fichier
    if (!(file)) errorSDFull(); // renvoie une erreur si le fichier ne s'ouvre pas avec code LED carte SD pleine (car c'est probablement la raison pour laquelle ça s'ouvrira pas)

    file.print(F("date;lumin;hygr;pressure;temp;lat;lng\n")); //écrit l'en-tête
    file.flush();
}
void getDateAndTime(char *tab)
{
    /*
   Prend l'adresse du premier élément d'un tableau
  Écrit dans ce tableau la date et l'heure au format AAAA-MM-JJ HH:MN
  */
    if (!rtc.begin())
    {
        errorAccessRTC();
    }

    DateTime date = rtc.now();
    int tmp;
    tmp = date.year();

    tab[0] = '0' + (tmp / 1000) % 10;
    tab[1] = '0' + (tmp / 100) % 10;
    tab[2] = '0' + (tmp / 10) % 10;
    tab[3] = '0' + (tmp % 10);
    tab[4] = '-';
    tmp = date.month();
    tab[5] = '0' + (tmp / 10);
    tab[6] = '0' + (tmp % 10);
    tab[7] = '-';
    tmp = date.day();
    tab[8] = '0' + (tmp / 10);
    tab[9] = '0' +(tmp % 10);
    tab[10] = ' ';
    tmp = date.hour();
    tab[11] = '0' + (tmp / 10);
    tab[12] = '0' + (tmp % 10);
    tab[13] = ':';
    tmp = date.minute();
    tab[14] = '0' + (tmp / 10);
    tab[15] = '0' + (tmp % 10);
}

void writeInFile()
{
    /*
    Écrit dans le fichier ouvert la date et les données des capteurs
    */
    Print* output; // On créer une variable pointeur de type Print pour pouvoir écrire dans le Serial ou la carte SD avec le même code
    if (actualMod != MAINTENANCE_MOD || !SD.isConnected()) // On regarde si l'on doit et peut écire dans la carte
    {
        if (file.size() >= (params.FILE_MAX_SIZE - 50)) {
            file.close();
            createFile();
            file = SD.open(fileName, FILE_WRITE); // ouvre le fichier
        }
        output = &file; //On défini la variable comme le fichier ouvert
    }
    else
    {
        Serial.println("Maintenance Mod");
        output = &Serial; //On défini la variable comme le Serial
    }


    // écrit la date et l'heure
    char buffer[17]={'\0'};
    getDateAndTime(buffer); // On récupère la date avec l'horloge RTC
    output->print(buffer);
    output->print(";");

    output->flush();

    output->print(luminosity == 101 ? "NA" : String(luminosity));  //Si les capteurs sont dans leurs valeurs spéciale alors on écrit NA
    output->print(";");
    output->print(humidity == 101 ? "NA" : String(humidity));
    output->print(";");
    output->print(pressure==299 ? "NA" : String(pressure));
    output->print(";");
    output->print(temp == 0 ? "NA" : String(temp-41));
    output->print(";");
    output->print(bufferLat);
    output->print(";");
    output->print(bufferLng);
    output->print("\n");


    output->flush();
}