/////////////////////////////////////////////////////////////////
//////////////                                     //////////////
//////////////        Wortuhr Projektarbeit        //////////////
//////////////        Daniel Rehberg 2BKI2         //////////////
//////////////                                     //////////////
/////////////////////////////////////////////////////////////////

#include <RTClib.h>             //https://github.com/adafruit/RTClib
#include <DCF77.h>              //https://github.com/thijse/Arduino-DCF77
#include <Time.h>               //https://github.com/PaulStoffregen/Time
#include <TimeLib.h>            //https://github.com/PaulStoffregen/Time
#include <String.h>             //https://github.com/arduino/ArduinoCore-API/blob/master/api/String.h
#include <Adafruit_GFX.h>       //https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_NeoMatrix.h> //https://github.com/adafruit/Adafruit_NeoMatrix
#include <Adafruit_NeoPixel.h>  //https://github.com/adafruit/Adafruit_NeoPixel
#include <EEPROM.h>             //https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/EEPROM


//////////////////
///Definitionen///
//////////////////
#define DCF77_PIN 2       //Definition des PINs vom DCF77 Empfänger
#define Matrix_PIN 6      //Definition des PINs von der LED Matrix
#define LPT80Pin A3       //Definition des PINs vom Lichtsensor
#define matrixX 11        //Definition der Höhe der LED Matrix
#define matrixY 11        //Definition der Weite der LED Matrix

///////////////////////////////////
///Gloable Variablen Deklaration///
///////////////////////////////////
unsigned char brightness = 25;        //Helligkeit der LEDs (Default: 25)
unsigned char colorvar = 0;           //Welche Farbe gerade aktiv ist mithilfe des farben Arrays
unsigned int valueLPT = 100;          //Wert des Lichtsensors
String Uhrzeit = "";                  //Variable für Uhrzeit Ausgabe auf Seriellen Monitor
long dcfintervall = 120000;           //Intervall von 2 Minuten zum updaten des DCF77 Empfängers
long dcfintervallsub = 0;             //Variable um die Millisekunden von der Systemzeit abzuziehen um zu schauen ob 2 Minuten vergangen sind
char eeacddress = 0;
char eeatddress = 1;

//Array Variable für die LED Matrix zum ansteuern
unsigned char matrix[matrixX][matrixY] =
{
// E S K I S T A F Ü N F
  {0,0,0,0,0,0,0,0,0,0,0}, //Erste Reihe
// Z E H N Z W A N Z I G
  {0,0,0,0,0,0,0,0,0,0,0}, //Zweite Reihe
// D R E I V I E R T E L
  {0,0,0,0,0,0,0,0,0,0,0}, //Dritte Reihe
// V O R F U N K N A C H
  {0,0,0,0,0,0,0,0,0,0,0}, //Vierte Reihe
// H A L B A E L F Ü N F
  {0,0,0,0,0,0,0,0,0,0,0}, //Fünfte Reihe
// E I N S X Ä M Z W E I
  {0,0,0,0,0,0,0,0,0,0,0}, //Sechste Reihe
// D R E I A U J V I E R
  {0,0,0,0,0,0,0,0,0,0,0}, //Siebte Reihe
// S E C H S N L A C H T
  {0,0,0,0,0,0,0,0,0,0,0}, //Achte Reihe
// S I E B E N Z W Ö L F
  {0,0,0,0,0,0,0,0,0,0,0}, //Neunte Reihe
// Z E H N E U N K U H R
  {0,0,0,0,0,0,0,0,0,0,0}, //Zehnte Reihe
//       - - - -
  {0,0,0,0,0,0,0,0,0,0,0}  //Elfte Reihe
};

///////////////////////
///Objekte Erzeugung///
///////////////////////
DCF77 DCF = DCF77(DCF77_PIN, 0);    //DCF77 Objekt erstellen
RTC_DS3231 rtc;                     //RTC Objekt erstellen

//Adafruit_NeoMatrix Objekt erstellen
//Mithilfe dieser Seite erstellt: https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library
Adafruit_NeoMatrix LEDmatrix = Adafruit_NeoMatrix(matrixX, matrixY, Matrix_PIN,      //Weite, Höhe und der PIN der LEDs
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +                                             //Die erste LED ist oben links deshalb TOP und Left
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,                                            //Die LED Strips sind in Reihen aufgeklent und im ZigZag Muster
  NEO_GRB            + NEO_KHZ800);                                                  //GRB ist die Reihenfolge der Daten für die 3 Grundfarben und KHZ800 ist die Frequenz

//Konstante Array Variable für verschiedene Farben
//Mithilfe dieser Seite: https://www.farb-tabelle.de/de/farbtabelle.htm
const unsigned int farben[] =
{
  LEDmatrix.Color(255, 255, 255),    //Weiß (Standard Farbe)
  LEDmatrix.Color(255, 0, 0),        //Rot
  LEDmatrix.Color(0, 255, 0),        //Grün
  LEDmatrix.Color(0, 0, 255),        //Blau
  LEDmatrix.Color(25, 25, 112),      //Mitternachtsblau
  LEDmatrix.Color(160, 32, 240),     //Lila
  LEDmatrix.Color(255, 105, 180)     //Pink
};

////////////////////
///Setup Funktion///
////////////////////
void setup()
{
  Serial.begin(9600);                                                    //Seriellen output aktivieren

  DCF.Start();                                                           //DCF77 Empfänger initialisieren
  rtc.begin();                                                           //Real Time Clock Modul initialisieren
  LEDmatrix.begin();                                                     //LED matrix intialiseren
  LEDmatrix.setBrightness(brightness);                                   //LED Helligkeit setzen (25)

  char eeseconds = "";
  EEPROM.get(eeatddress, eeseconds);
  EEPROM.get(eeacddress, colorvar);
  if (rtc.now().second() - eeseconds < 5)
  {
    colorvar = (colorvar + 1) % 7;
  }
  EEPROM.put(eeatddress, rtc.now().second());
  EEPROM.put(eeacddress, colorvar);

  if (! rtc.begin())                                                     //Es wird geprüft ob das RTC Modul läuft
  {
    rtc.adjust(DateTime(__DATE__, __TIME__));                            //RTC wird zu der Zeit des kompilieren dieses Sketches gesetzt
  }
}

///////////////////
///Loop Funktion///
///////////////////
void loop()
{
  Serial.println();                            //Neue Zeile im Seriellen Monitor

  LEDmatrix.clear();                           //LEDMatrix zurücksetzen
  
  valueLPT = analogRead(LPT80Pin);             //Wert vom Lichtsensor auslesen
  HelligkeitsRechner(valueLPT);            //HelligkeitsRechner Methode ausführen

  DCF77ZeitEmpfangen();                        //Empfangen der Uhrzeit vom DCF77 Modul und auf Systemzeit und RTC setzen

  DateTime time = rtc.now();                   //Uhrzeit vom RTC auf die Variable time setzen

  UhrzeitAnzeigen(time.hour(), time.minute()); //Wörter auf Wortuhr anzeigen

  SetLEDMatrix();                              //Die gesetzten LEDs aus der LEDMatrix Variable auf die LED-Matrix übertragen

  LEDmatrix.show();                            //LED Matrix zeigen mit den ganzen Einstellungen
  
  delay(5000);                                 //Delay von 5000ms -> 5 Sekunden
}

//Methode um die Helligkeit der LEDs mithilfe des Lichtsensors auszurechnen
void HelligkeitsRechner(unsigned int val)
{
  Serial.print("Lichtsensor: ");
  Serial.println(val);
  if (val < 10)               //Prüfen ob der Lichtsensor wert unter 50 und über 5 liegt
  {
    brightness = val+10;                  //Helligkeit Rechnung mit Lichtsensor wert
    Serial.print("dunkel");
  }
  else if (val > 10)
  {
    brightness = val+25;
    Serial.print("hell");
  }
  else
  {
    brightness = 25;                     //Default Helligkeit
  }
  Serial.println();
  LEDmatrix.setBrightness(brightness);   //LED Matrix Helligkeit zum ausgerechneten Wert setzen
}

//Methode um die Uhrzeit vom DCF77 zu empfangen und auf den RTC zu setzen 
//Mithilfe von: https://github.com/thijse/Arduino-DCF77/blob/master/examples/InternalClockSync/InternalClockSync.ino
void DCF77ZeitEmpfangen()
{
  time_t DCFtime = DCF.getTime();                   // Schauen ob es eine neue Uhrzeit vom DCF77 Empfänger gibt
  if (DCFtime!=0)                                   //Wenn die Uhrzeit vom DCF77 nicht 0 ist
  {
    setTime(DCFtime);                               //Systemzeit wird zur DCF Zeit gesetzt
    if(dcfintervall < (millis() - dcfintervallsub)) //Schauen ob der Intervall von 2 Minuten erreicht wurde
    {
      rtc.adjust(DCFtime);                          //Uhrzeit vom RTC wird zur DCF Zeit gesetzt
      dcfintervallsub = millis();                   //Variable zu Millisekunden von der Systemzeit setzen
    }
  }
}

//Methode um die position einer LED aus der Matrix HIGH zu setzen
void SetMatrixHIGH(unsigned char y, unsigned char x)
{
  matrix[x][y] = 1; //Position höhe y weite x zu HIGH setzen in der LED Matrix
}

//Methode um alle LEDs auszuschalten
void SetAllMatrixLOW()
{
  for(char x=0; x < matrixX; x++)   //for-Schleife für Weite
  {
    for(char y=0; y < matrixY; y++) //for-Schleie für Höhe
    {
      matrix[x][y] = 0;             //LED an position weite: x höhe: y wird zu LOW gesetzt
    }
  }
}

//Methode um die LEDs zu setzen aus der LEDmatrix Variable
void SetLEDMatrix()
{
  for(char x=0; x < matrixX; x++)                   //for-Schleife für Weite
  {
    for(char y=0; y < matrixY; y++)                 //for-Schleie für Höhe
    {
      if(matrix[x][y] != 0)                         //Ist die LED an position (weite: x) und (höhe: y) HIGH
      {
        LEDmatrix.drawPixel(x,y,farben[colorvar]);  //LED setzen in der LEDmatrix
      }
    }
  }
}

//Methode für die Minuten
void minutetxt(unsigned char txt)
{
  switch(txt)
  {
    case (char)'HALB':
      //HALB
      Uhrzeit += "HALB ";
      SetMatrixHIGH(4,0);  //H
      SetMatrixHIGH(4,1);  //A
      SetMatrixHIGH(4,2);  //L
      SetMatrixHIGH(4,3);  //B
      break;
    case (char)'VOR':
      //VOR
      Uhrzeit += "VOR ";
      SetMatrixHIGH(3,0);  //V
      SetMatrixHIGH(3,1);  //O
      SetMatrixHIGH(3,2);  //R
      break;
    case (char)'NACH':
      //NACH
      Uhrzeit += "NACH ";
      SetMatrixHIGH(3,7);  //N
      SetMatrixHIGH(3,8);  //A
      SetMatrixHIGH(3,9);  //C
      SetMatrixHIGH(3,10); //H
      break;
    case (char)'VIERTEL':
      //VIERTEL
      Uhrzeit += "VIERTEL ";
      SetMatrixHIGH(2,4);  //V
      SetMatrixHIGH(2,5);  //I
      SetMatrixHIGH(2,6);  //E
      SetMatrixHIGH(2,7);  //R
      SetMatrixHIGH(2,8);  //T
      SetMatrixHIGH(2,9);  //E
      SetMatrixHIGH(2,10); //L
      break;
    case (char)'FUENF':
      //FÜNF
      Uhrzeit += "FUENF ";
      SetMatrixHIGH(0,7);  //F
      SetMatrixHIGH(0,8);  //Ü
      SetMatrixHIGH(0,9);  //N
      SetMatrixHIGH(0,10); //F
      break;
    case (char)'ZEHN':
      //ZEHN
      Uhrzeit += "ZEHN ";
      SetMatrixHIGH(1,0);  //Z
      SetMatrixHIGH(1,1);  //E
      SetMatrixHIGH(1,2);  //H
      SetMatrixHIGH(1,3);  //N
      break;
    case (char)'ZWANZIG':
      //ZWANZIG
      Uhrzeit += "ZWANZIG ";
      SetMatrixHIGH(1,4);  //Z
      SetMatrixHIGH(1,5);  //W
      SetMatrixHIGH(1,6);  //A
      SetMatrixHIGH(1,7);  //N
      SetMatrixHIGH(1,8);  //Z
      SetMatrixHIGH(1,9);  //I
      SetMatrixHIGH(1,10); //G
      break;
  }
}

//Methode für die Stunden
void stunde(unsigned char hour)
{
  switch(hour)
  {
    case 12:
      //ZWÖLF
      Uhrzeit += "ZWOELF ";
      SetMatrixHIGH(8,6);  //Z
      SetMatrixHIGH(8,7);  //W
      SetMatrixHIGH(8,8);  //Ö
      SetMatrixHIGH(8,9);  //L
      SetMatrixHIGH(8,10); //F
      break;
    case 1:
      //EINS
      Uhrzeit += "EINS ";
      SetMatrixHIGH(5,0);  //F
      SetMatrixHIGH(5,1);  //Ü
      SetMatrixHIGH(5,2);  //N
      SetMatrixHIGH(5,3);  //F
      break;
    case 2:
      //ZWEI
      Uhrzeit += "ZWEI ";
      SetMatrixHIGH(5,7);  //Z
      SetMatrixHIGH(5,8);  //W
      SetMatrixHIGH(5,9);  //E
      SetMatrixHIGH(5,10); //I
      break;
    case 3:
      //DREI
      Uhrzeit += "DREI ";
      SetMatrixHIGH(6,0);  //D
      SetMatrixHIGH(6,1);  //R
      SetMatrixHIGH(6,2);  //E
      SetMatrixHIGH(6,3);  //I
      break;
    case 4:
      //VIER
      Uhrzeit += "VIER ";
      SetMatrixHIGH(6,7);  //V
      SetMatrixHIGH(6,8);  //I
      SetMatrixHIGH(6,9);  //E
      SetMatrixHIGH(6,10); //R
      break;
    case 5:
      //FÜNF
      Uhrzeit += "FUENF ";
      SetMatrixHIGH(4,7);  //F
      SetMatrixHIGH(4,8);  //Ü
      SetMatrixHIGH(4,9);  //N
      SetMatrixHIGH(4,10); //F
      break;
    case 6:
      //SECHS
      Uhrzeit += "SECHS ";
      SetMatrixHIGH(7,0);  //S
      SetMatrixHIGH(7,1);  //E
      SetMatrixHIGH(7,2);  //C
      SetMatrixHIGH(7,3);  //H
      SetMatrixHIGH(7,4);  //S
      break;
    case 7:
      //SIEBEN
      Uhrzeit += "SIEBEN ";
      SetMatrixHIGH(8,0);  //S
      SetMatrixHIGH(8,1);  //I
      SetMatrixHIGH(8,2);  //E
      SetMatrixHIGH(8,3);  //B
      SetMatrixHIGH(8,4);  //E
      SetMatrixHIGH(8,5);  //N
      break;
    case 8:
      //ACHT
      Uhrzeit += "ACHT ";
      SetMatrixHIGH(7,7);  //A
      SetMatrixHIGH(7,8);  //C
      SetMatrixHIGH(7,9);  //H
      SetMatrixHIGH(7,10); //T
      break;
    case 9:
      //NEUN
      Uhrzeit += "NEUN ";
      SetMatrixHIGH(9,3);  //N
      SetMatrixHIGH(9,4);  //E
      SetMatrixHIGH(9,5);  //U
      SetMatrixHIGH(9,6);  //N
      break;
    case 10:
      //ZEHN
      Uhrzeit += "ZEHN ";
      SetMatrixHIGH(9,0);  //Z
      SetMatrixHIGH(9,1);  //E
      SetMatrixHIGH(9,2);  //H
      SetMatrixHIGH(9,3);  //N
      break;
    case 11:
      //ELF
      Uhrzeit += "ELF " ;
      SetMatrixHIGH(4,5);  //E
      SetMatrixHIGH(4,6);  //L
      SetMatrixHIGH(4,7);  //F
      break;
  }
}

//Methode um die Uhrzeit auf der Wortuhr anzuzeigen
void UhrzeitAnzeigen(unsigned char hour, unsigned char minute)
{
  //Rechnung für 12h Format
  if (hour > 12)        //Wenn es später als 12 Uhr Mittags ist, wird der Uhrzeit Variable 12 abgezogen
  {
    hour = hour-12;
  }
  else if (hour == 0)   //Wenn es 0 Uhr ist wird die Uhrzeit Variable zu 12 gesetzt
  {
    hour = 12;
  }
  SetAllMatrixLOW();    //Alle LEDs ausschalten

  //ES IST
  Uhrzeit += "ES IST ";
  SetMatrixHIGH(0,0);  //E
  SetMatrixHIGH(0,1);  //S
  SetMatrixHIGH(0,3);  //I
  SetMatrixHIGH(0,4);  //S
  SetMatrixHIGH(0,5);  //T

  if (minute >= 55)
  {
    minutetxt('FUENF');
    minutetxt('VOR');
    hour++;
  }
  else if (minute >= 50)
  {
    minutetxt('ZEHN');
    minutetxt('VOR');
    hour++;
  }
  else if (minute >= 45)
  {
    minutetxt('VIERTEL');
    minutetxt('VOR');
    hour++;
  }
  else if (minute >=40)
  {
    minutetxt('ZWANZIG');
    minutetxt('VOR');
    hour++;
  }
  else if (minute >= 35)
  {
    minutetxt('FUENF');
    minutetxt('NACH');
    minutetxt('HALB');
    hour++;
  }
  else if (minute >= 30)
  {
    minutetxt('HALB');
    hour++;
  }
  else if (minute >= 25)
  {
    minutetxt('FUENF');
    minutetxt('VOR');
    minutetxt('HALB');
    hour++;
  }
  else if (minute >= 20)
  {
    minutetxt('ZEHN');
    minutetxt('VOR');
    minutetxt('HALB');
    hour++;
  }
  else if (minute >= 15)
  {
    minutetxt('VIERTEL');
    minutetxt('NACH');
  }
  else if (minute >= 10)
  {
    minutetxt('ZEHN');
    minutetxt('NACH');
  }
  else if (minute >= 5)
  {
    minutetxt('FUENF');
    minutetxt('NACH');
  }
  else
  {
    Uhrzeit += "UHR ";
    //UHR
    SetMatrixHIGH(9,8);  //U
    SetMatrixHIGH(9,9);  //H
    SetMatrixHIGH(9,10); //R
  }

  stunde(hour);

  //4 Punkte
  switch(minute%5)
  {
      case 1:
        //1 Minute
        Uhrzeit += " + 1 MINUTE";
        SetMatrixHIGH(10,0);  //1. Minute
        break;
      case 2:
        //2 Minuten
        Uhrzeit += " + 2 MINUTEN";
        SetMatrixHIGH(10,0);  //1. Minute
        SetMatrixHIGH(10,1);  //2. Minute
        break;
      case 3:
        //3 Minuten
        Uhrzeit += " + 3 MINUTEN";
        SetMatrixHIGH(10,0);  //1. Minute
        SetMatrixHIGH(10,1);  //2. Minute
        SetMatrixHIGH(10,2);  //3. Minute
        break;
      case 4:
        //4 Minuten
        Uhrzeit += " + 4 MINUTEN";
        SetMatrixHIGH(10,0);  //1. Minute
        SetMatrixHIGH(10,1);  //2. Minute
        SetMatrixHIGH(10,2);  //3. Minute
        SetMatrixHIGH(10,3);  //4. Minute
        break;
  }
  
    Serial.print(Uhrzeit);
    Uhrzeit = "";

    Serial.println();
}