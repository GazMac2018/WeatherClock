/*
  WeatherWall and Clock Program by G.McAlister - Started Aug 2022

  Board Management : LOLIN D32
  Partition Scheme : Minimal Spiffs (Large Apps with OTA)

 ****  Edit <User_Setup_Select.h> to INCLUDE <User_Setup_ESP32_IL19341.h> WHICH holds tft PINs etc

  My Limits with OpenWeatherMap.org
  60 calls/minute
  1,000,000 calls/month

  Pins Used for TFT Screen
                 GPIO  TFT     ESP32 (Location)
  #define TFT_MISO 19    9  -->  12
  #define TFT_MOSI 23    6  -->  18
  #define TFT_SCLK 18    7  -->  11
  #define TFT_CS   15    3  -->   4        Chip select control pin
  #define TFT_DC    2    5  -->   5        Data Command control pin
  #define TFT_RST   4    4  -->   7        Reset pin (could connect to RST pin)

  #define TOUCH_CS 16   11  -->   8    // CS_PIN 16 for Clocks - Pin 5 for Testing - Gaz

  //  Fad the LED Screen for Clocks
  ScreenLED = 17;       8   -->   9    // For Screen pin 17 for Clocks - Pin 22 for Testing

  #define ILI9341_DRIVER

  #define DHTPIN 21       LOC 14   // Digital pin connected to the DHT sensor

  #define pResLDR 36 // ESP32 pin GIOP36 (ADC0)  Using 82k Resistor

 *****************************************************************

    BEWARE THIS SKETCH USES THE FOLLOWING LIBRARY FILES:

    /home/gazman/Arduino/libraries/TFT_eSPI/User_Setup.h

    /home/gazman/Arduino/libraries/TFT_eSPI/User_Setup_Select.h

    NOTE:  THESE FILES HAVE BEEN CHANGED TO READ ONLY

    and

    #include <User_Setups/Setup42_ILI9341_ESP32.h> // Setup file for ESP32 and SPI ILI9341 240x320

 ******************************************************************

  VERSION TIME STAMPS

  V-1  Started August 2022

  V-2  Added LDR for Night Time
      Main Menu for
          Location variations
          WiFi Manager
          Night Time Brightness setting
          Added Version & Time Stamp to Main Menu

       Tried Dual CPU, spent several weeks on that idea
       and it crashed repeatly.

  V-3   Added NonClockDelay - Replaced all delay(xxx) with millis() delay and
          during the NonClockDelay process, the Clock will Update..

  V-4   From Main Menu Toggle from Large Day Display in Lower Weather
          display Area or Back to Weather Forecasts...

          Added the Feature that from the BigDay Screen press the centre area
          and the Temperature and Daily Forecasts appear for 5 seconds
          During this 5 sec pediod, the 5 Day Forecast can also be selected * If you are quick *
  
  V-5     Clock brightens up a little bit at SunRise until Sunset if in a dark room
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"


// #include <TFT_eSPI_320_240.h>

#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include <time.h>

#include <Arduino_JSON.h>  //  Gaz - If Arduino_JSON.h is placed BEFORE #include <TFT_eSPI.h>  ==  MAJOR ERRORS

// https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>

#include <EEPROM.h>

//  For Touch Prog
#include <XPT2046_Touchscreen.h>

//#include <Adafruit_Sensor.h>
//#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 21      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302)

//  For Touch Prog
#define TOUCH_CS 16  //  CS_PIN 16 for Clocks - Pin 5 for Testing - Gaz

DHT_Unified dht(DHTPIN, DHTTYPE);

#include "soc/soc.h"           //  only used to avoid BrownOut issue and used with..
#include "soc/rtc_cntl_reg.h"  //  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // prevent brownouts by silencing them

int SensorDelay = 2500;  //  Minimum Default Delay for Temperature Sensor
uint32_t LastSensorDelay = millis();
float Calibrate;  // used for the temperature calbration

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

//const char*
String ssid;  //= "Telstra9E2B8C";    //  = "GazOppoA91"   = "iiNetA06F87"
//const char*
String password;  // = "GazGibson2017";  //  = "twinhammer"   = "NZHZ9PGZTHJ79F5"

String TimeDate;

int TimeOut;
boolean InternetFail = false;
int FailAttempt = 0;
boolean NoSelection = true;
boolean SelectionYesNo;
boolean FirstBoot = true;

//  EEPROM variables
String esid = "";
String epass = "";
String eLocalCity = "";
String eCountryCode = "";
boolean SaveEEPROM = false;

String DOT = ".";
String UpDateReason;
uint32_t UpdateMillis = millis();  //  for updating Forecast if InternetFail = true;

// Use your City and Country Code
//String LocalCity = "Bongaree";  //  Bongaree, AU

//  Multi Dimensional Array Size == 10 but 1st Array == [0] & 10th Array == [9]
String LocalCityArray[10][2] = {
  "Keperra",
  "AU",
  "Brisbane",
  "AU",
  "Caloundra",
  "AU",
  "Sydney",
  "AU",
  "Melbourne",
  "AU",
  "Adelaide",
  "AU",
  "Darwin",
  "AU",
  "Toowoomba",
  "AU",
  "London",
  "GB",
  "Los Angeles",
  "US",
};
int DispCity = 0;

String LocalCity = LocalCityArray[DispCity][0];  //  "Brisbane";
String CountryCode = LocalCityArray[0][1];       //   "AU";

//  Clock ID  i.e. (1) & (2) == First Clock - Mum's or Office Clock - Blue/Green Clock
String ClockID;

String APIKEY;
//String APIKEY = "b436ff4fe79571ac2dda4c049cbaa556";  //  1st Clock - Mum's or Office Clock - Blue/Green Clock
//String APIKEY = "6e20ed5373db527ae41b44e066f4887f";  //  2nd Clock - Mum's or Office Clock - Blue/Green Clock
//String APIKEY = "c834c5045583e40972f8ae334fef0e66";  //  3rd Clock - Bedroom Clock - White/Blue Clock

//   Time Clock Variables
const char* ntpServer = "pool.ntp.org";  //  Internet Time website
const long gmtOffset_sec = 36000;        //  +10 * 60 * 60  ==  10 hours before GMT
const int daylightOffset_sec = 0;        //  0 == NO,  3600 == YES
int Hour;
int Minute;
double Second;
int Year;
int Month;
int TwHour;
int TwMin;
int TwSec;
int DispHour;
String StrDispHour;  // For displying 12 instead of 0
boolean Silent = false;
boolean ShowDayDate = true;
boolean ChangeDisplay = false;
boolean CheckTime = true;

boolean BigDayDisplay = false;
boolean DoBigDay = false;
String BigDayToggle;
boolean TempWeatherDisp = false;
boolean FirstTime;
uint32_t TempTimer;

String StrMin;
String StrSec;
String StrYear;
String StrMonth;
String StrDay;
String AmPm;
String TimeCalc;
String DateCalc;
int ScreenColour;
int LastHour;
int LastDay;

int NextDay;
int wDay;    //  day of week [0,6] (Sunday = 0)
int nDay;    //  Day Date ==  1 to 28, 30 or 31
int LstDay;  //  for Time update at Midnight
String ShortTime;
int DayTxtY = 10;

boolean AfterMidnight = true;  //  should be false but want to test
String MidNightTime = "None Yet";
String AfterMidNightTime = "Not Yet";  // these 2 time should be very close. Displayed on MainMenu.

int TimeSync = false;

int i;
float LastTemp;

//  Added to convert timeinfo.tm_wday to String == Day of the Week
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char monthsOfYear[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//int MonthDays[12] =          {31,   28,    31,     30,   31,    30,    31,    31,    30,    31,    30,    31};

//  Setup Graphics (Converted  at: https://www.mischianti.org/images-to-byte-array-online-converter-cpp-arduino/
extern uint8_t Graphic_WIFI[];  //  Graphics displayed when connecting to the Internet
extern uint8_t clearsky[];      //  Graphics for Clear Sky or Sunny
extern uint8_t fewclouds[];
extern uint8_t scatteredclouds[];
extern uint8_t brokenclouds[];
extern uint8_t overcastclouds[];
extern uint8_t lightrain[];
extern uint8_t moderaterain[];
extern uint8_t heavyrain[];
extern uint8_t rain[];
extern uint8_t thunderstorm[];
extern uint8_t hail[];

extern uint8_t sunrise[];
extern uint8_t sunset[];

extern uint8_t Graphic_Monday[];
extern uint8_t Graphic_Tuesday[];
extern uint8_t Graphic_Wednesday[];
extern uint8_t Graphic_Thursday[];
extern uint8_t Graphic_Friday[];
extern uint8_t Graphic_Saturday[];
extern uint8_t Graphic_Sunday[];

//extern uint8_t keysbd[];

String jsonBuffer;
JSONVar myObject = JSON.parse(jsonBuffer);
int temperature;

int NextForecastTime;
const char* CheckForeCastMain;  //  check for Thunderstorm and Drizzle
const char* ForeCast1stSlot;    //  string
String ForeCast1stSlotStr;
const char* ForeCast2ndSlot;
String ForeCast2ndSlotStr;
const char* City;

String TestTime;

//  SunRise and SunSet Variables
long GetSunRiseUnix;
long SunRiseLocal;
long GetSunSetUnix;
long SunSetLocal;
long TimezoneOffset;
String OutputHour;
String OutputMin;
String SunRiseHour;
String SunRiseMin;
String SunSetHour;
String SunSetMin;

boolean DailyUpdateCall = false;

int TimeSlot1st;  //  00  03  06  09  12  15  18  21
int TimeSlot2nd;

int XX;  //  for blacking out Icons
int YY;
int Xw;
int Yh;

int xTime1Slot = 162;  //  text above Weather Icons
int yTime1Slot = 160;
int xTime2Slot = 268;
int yTime2Slot = 160;

boolean GoForcast = true;
boolean GetForcast = true;  //  used to get the forecast after first Boot time
boolean DoOnce = true;      //  for testing 7pm or 19:00
//boolean Done10am = false;
//boolean Done4pm = false;
boolean DoneHourlyCheck = false;
boolean Testing;

unsigned long FirstMillis;  //  This respresents zero second for CountSec not real time second
unsigned long CountSec;

XPT2046_Touchscreen ts(TOUCH_CS);  // was CS_PIN  Param 2 - NULL - No interrupts

//  Touch variables
int PX;
int MaxPX;
int MinPX = 10000;
int PY;
int MaxPY;
int MinPY = 10000;
boolean TouchDone = false;
boolean TouchAnywhere = false;

//  Fad the LED Screen
int ScreenLED = 17;  // For Screen pin 17 for Clocks - Pin 22 for Testing

// setting PWM properties
//  const int PWMFreq = 5000; /* 5 KHz */  Not used anymore -GAZ- Since Jan 2025
//  const int PWMChannel = 0;   //  Not used anymore -GAZ- Since Jan 2025
const int PWMResolution = 10;
int IntScnBrightness = 600;
int MaxBrightness = 750;  // (int)(pow(2, PWMResolution) - 1);  //  1023
int Brightness = 375;
int BarNumb = 9;
uint32_t BrightCntDwn = millis();  // Hold display of Brightness display bars
uint32_t BrightnessBarsStart = millis();
boolean DelayTime = true;
int MinBright = 10;
boolean MinBrightSw = true;

//  LDR settings - Using 82k Resistor to detect light from adjacent room light.
#define pResLDR 36  // ESP32 pin GIOP36 (ADC0)   Using 82k Resistor
int ReadLDR;
int LdrDelay = 10000;  //  10 sec, delay when lights go out - Reset Default Delay for LDR Delay
uint32_t LastLdrDelay = millis();
int LdrDimmer = 375;
int SetLdrDimmer = 0;

//  Get MAC Address to set Temp Sensor Calibration for each Clock..
String MACaddress;

#define __FILENAME__ (strrchr("/"__FILE__, '/') + 1)

//******************************************************************************************

//  This is for the Openweather --- jsonbuffer???
String httpGETRequest(const char* serverName)  //  *****************************************
{
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
    {
      //Serial.print("HTTP Response code: ");  //  200 == OK
      //Serial.println(httpResponseCode);
      payload = http.getString();
    }
  else
    {
      //Serial.print("Error code: ");
      //Serial.println(httpResponseCode);   //  error code == -1  ==  no internet
      //Serial.println("String httpGETRequest -- Try Reconnect Later");
      CheckInternet();
    }
  // Free resources
  http.end();

  return payload;
}

void setup(void)  //  ******************************************************************
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // prevent brownouts by silencing them

  Serial.begin(115200);
  delay(1000);

  char* GetTime = __TIME__;
  char* GetDate = __DATE__;

  TimeDate = "V-5 ";
  TimeDate = TimeDate + GetTime + " : " + GetDate;
  Serial.println();
  Serial.println();
  Serial.println("*********************************");
  Serial.println("Starting Setup for");
  Serial.println(__FILENAME__);  //  Printsout /Dir/Filename
  Serial.println(TimeDate);      //  Shows Time & Date when Last Compiled
  Serial.println("by G.McAlister - 21 September 2022");
  Serial.println("**********************************");

  delay(100);

  //  Change here when Testing finished..
  Testing = false;

  if (Testing)  //  Currently Testing for SunRise (Key word) options
    {
      Serial.println("*** TESTING MODE - Edit or remove the following Lines  ***");
      Serial.println("*** #define TOUCH_CS 5  // CS_PIN 16 for Clocks - Pin 5 for Testing - Gaz");
      Serial.println("*** int ScreenLED = 22; // For Screen pin 17 for Clocks - Pin 22 for Testing");
      Serial.println("*** //  For Testing without a LDR Line 1320 - GAZ");
      Serial.println("*** ReadLDR = 100;");
      Serial.println("**********************************");
    }

  //  28 characters max for ssid and password each - hopefully
  //Initialasing EEPROM  (0) 'Y' or NULL, (1) Length of ssid, (2~30) ssid,
  //(31)Length of Password (32~60) password
  //  & eLocalCity (16 61~77) & eCountryCode (2 81~82)
  //  & MinBright (1 84)
  EEPROM.begin(90);
  delay(100);

  //  read and display all EEPROMs for testing only
  /*  for (i = 0; i < 90; i++)
    {
      Serial.print("@ i: ");
      Serial.print(i);
      Serial.print("  Reads #: ");
      Serial.print(EEPROM.read(i));
      Serial.print("  or char: ");
      Serial.println(char(EEPROM.read(i)));
    }
    Serial.println("***********************");*/


  /*
    int Row;
    int Col = 0;  //  Show CityName

    //  Print out Stored Array for City Name and Country Codes for testing only
    for (Row = 0; Row < 10; Row++)
    {
      Serial.print(Row);
      Serial.print(", ");
      Serial.print(Col);
      Serial.print(" ");
      Serial.print(LocalCityArray[Row][Col]);
      Serial.print(" ");
      Col++;  //  Show CountryCode
      Serial.println(LocalCityArray[Row][Col]);

      Col = 0;  //  Show CityName
    }
    Serial.println("**********************************");
  */

  /*  May be needed sometimes to reset the ESP32 EEPROM
    Serial.println("Clearing EEPROM");
    for (int i = 0; i < 84; i++)
    {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
  */
  /*  Next Lines were for Older Version..  Changed sometime around Jan 2025
  ledcSetup(PWMChannel, PWMFreq, PWMResolution);
  // Attach the LED PWM Channel to the GPIO Pin
  ledcAttachPin(ScreenLED, PWMChannel);
  ledcWrite(PWMChannel, Brightness );  //  Turn on LED to Inital Brightness  
  */

  //  Set initial Screen Brightness here
  ledcAttach(ScreenLED, 50, PWMResolution);
  ledcWrite(ScreenLED, IntScnBrightness);  //  Turn on SCREEN to Inital Brightness

  //Serial.print("MaxBrightness = ");
  //Serial.println(MaxBrightness);

  dht.begin();
  delay(100);
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delay(100);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  LastTemp = event.temperature;

  if (Testing)
    {
      LastTemp = 15;
      Serial.println("Temporary Temperature input here..  line 494");
    }

  //Serial.print("Initial Temperature Test =");
  //Serial.println(LastTemp);

  //  For Touch Prog
  pinMode(TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);  //  was CS_PIN   Pin 16
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);  //  was CS_PIN

  pinMode(pResLDR, INPUT);  // LDR input   Using 82k Resistor

  //  For Touch Prog
  ts.begin();
  delay(100);

  // Set delay between temperature sensor readings based on sensor details.
  SensorDelay = 2500;  //  Default Delay for Temperature Sensor
  LastSensorDelay = millis();

  //  TFT initialised etc
  tft.init();
  delay(100);

  tft.setRotation(1);
  tft.invertDisplay(0);
  tft.fillScreen(TFT_BLACK);

  LdrDelay = 0;  //  This ensures there is no delay to Dim the Screen if the Clock reboots at night
  GoLDR();

  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();

  //  Get MAC Address...
  MACaddress = String(WiFi.macAddress());

  delay(100);

  /*  for Testing only
  Serial.print("MAC = ");
  Serial.println(MACaddress);
  Serial.println("********************************");
  */

  delay(100);

  //  Setup Calibration for USB connected Clock Unit *** calibration +#.#1  or -#.#9
  if (MACaddress == "7C:9E:BD:4C:3F:80")
    {                     //  "M"
      Calibrate = -1.99;  //  1st Clock - Mum's or Office Clock - Blue/Green Clock
      APIKEY = "b436ff4fe79571ac2dda4c049cbaa556";
      ClockID = "(1)";
      tft.invertDisplay(1);  // was 1
      tft.setRotation(1);
      MaxBrightness = 750;

      //  Serial.println("Clock is 1st Clock - Mum's or Office Clock - Blue/Green Clock");
    }

  if (MACaddress == "94:B9:7E:C3:A1:18")  //  2nd Clock - Mum's or Office Clock - Blue/Green Clock
    {                                     //  "N"
      Calibrate = -1.39;
      APIKEY = "6e20ed5373db527ae41b44e066f4887f";
      ClockID = "(2)";
      tft.invertDisplay(1);

      MaxBrightness = 850;
      //  Serial.println("Clock is 2nd Clock - Mum's or Office Clock - Blue/Green Clock");
    }

  if (MACaddress == "D4:8A:FC:A5:21:34")  // 3rd clock - Bedroom Clock - White/Blue Clock
    {                                     //  "Q"
      Calibrate = -1.5;
      APIKEY = "6e20ed5373db527ae41b44e066f4887f";
      ClockID = "(3)";
      tft.invertDisplay(0);

      MaxBrightness = 750;
      //  Serial.println("Clock is 3rd clock - Bedroom Clock - White/Blue Clock");
    }

  if (ClockID == "")  //  was null
    {
      //Serial.print("MAC Address NEW!!  MAC = ");
      //Serial.println(MACaddress);
      APIKEY = "6e20ed5373db527ae41b44e066f4887f";  //  For testing
      String LocalCity = "Brisbane";
      String CountryCode = "AU";
      //Serial.println("MAC Address not asigned :: This is a Test Unit");
    }

  //  TEST APIKEY
  //APIKEY = "b436ff4fe79571ac2dda4c049cbaa556";  //  First Clock - Mum's or Office Clock - Blue/Green Clock
  //APIKEY = "6e20ed5373db527ae41b44e066f4887f";  //  2nd Clock   Mum's or Office Clock - Blue/Green Clock
  //APIKEY = "c834c5045583e40972f8ae334fef0e66";  //  3rd Clock - White Bedroom Clock

  GoInternet();  //  Connect to WiFi

  GetWWWTime();

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);

  //  Debugging :::: To do a Time check - change Hour and Minute here..
  /*
    Hour = 20;
    Minute = 59;
    Second = 45;
    SensorDelay = millis();
    LastSensorDelay = millis();
  */

  //  Get LocalCity and CountryCode from EEPROM
  //Serial.println("Get LocalCity and CountryCode from EEPROM");

  //  Check for "#"  if no "#" then assume Brisbane, AU
  boolean LocTest = false;
  char TestStrLoc;
  eLocalCity = "";
  eCountryCode = "";

  for (i = 61; i < 77; i++)
    {
      TestStrLoc = char(EEPROM.read(i));

      /*    Serial.print("@ i: ");
        Serial.print(i);
        Serial.print("  Read: ");
        Serial.println(TestStrLoc);  */

      if (TestStrLoc == '#')
        {
          LocTest = true;
          i = 78;  //  to end for() loop
        }
      else
        {
          eLocalCity += TestStrLoc;

          //Serial.print("Progressive eLocalCity == ");
          //Serial.println(eLocalCity);
        }
    }

  if (LocTest)
    {
      LocalCity = eLocalCity;

      //Serial.print("LocalCity == ");
      //Serial.print(LocalCity);

      //  Get CountryCode
      for (i = 81; i < 83; i++)
        {
          eCountryCode += char(EEPROM.read(i));
          //Serial.print("Read: ");
          //Serial.println(char(EEPROM.read(i)));
        }
      CountryCode = eCountryCode;
      //Serial.print(",");
      //Serial.println(CountryCode);
    }

  // Get MinBright from EEPROM(83)
  MinBright = EEPROM.read(83);
  if (MinBright < 1) MinBright = 10;

  if (MinBright > 10)
    {
      MinBright = 10;
      MinBrightSw = true;  //  MinBrightness Switch is OFF
    }

  // Get Brightness from EEPROM(84)
  BarNumb = EEPROM.read(84);
  if (BarNumb < 1) BarNumb = 5;

  if (BarNumb > 10)
    {
      BarNumb = 5;
    }
  Brightness = map(BarNumb, 1, 10, 100, 750);

  // read BigDayDisplay from EEPROM(89)
  BigDayDisplay = EEPROM.read(89);  //  0 == OFF  1 == ON

  DoBigDay = false;

  if (BigDayDisplay)
    {
      DoBigDay = true;
      GetForcast = true;
    }
}

void TouchPxYx()  //  ********************************************************
{
  if (ts.touched() && !TouchDone)
    {
      //serial.println("void TouchPxYx()");

      //Serial.print("ts.touched() == ");
      //Serial.println(ts.touched());
      //Serial.print("TouchDone == ");
      //Serial.println(TouchDone);

      TS_Point p = ts.getPoint();
      //Serial.print("Pressure = ");
      //Serial.print(p.z);
      //Serial.print(", x = ");
      //Serial.print(p.x);
      PX = p.x;
      //Serial.print(", y = ");
      //Serial.print(p.y);
      PY = p.y;
      delay(250);
      //Serial.println();

      //  MaxPX MaxPY etc below ONLY used to determine the limits of the Touch Screen
      //  if( MaxPX < PX) MaxPX = PX;  //  3931
      //  if( MaxPY < PY) MaxPY = PY;  //  3783
      //  if( MinPX > PX) MinPX = PX;  //  245
      //  if( MinPY > PY) MinPY = PY;  //  285

      //Serial.print("MaxPX = ");
      //Serial.print(MaxPX);
      //Serial.print("  MaxPY = ");
      //Serial.print(MaxPY);
      //Serial.print("  MinPX = ");
      //Serial.print(MinPX);
      //Serial.print("  MinPY = ");
      //Serial.println(MinPY);

      PX = map(PX, 245, 3931, 0, 350);
      PY = map(PY, 285, 3783, 0, 240);

      //Serial.print("PX = ");
      //Serial.print(PX);
      //Serial.print("   PY = ");
      //Serial.println(PY);

      TouchDone = true;  //  stop multiple taps on screen

      // if( PY > 80 && PY < 175)  //  draw keyboard
      //  {
      //    GetWiFiInfo();                                    //  KeyboardKeys();
      //  }
    }
}

void TouchSpot()  //  For Touch Prog *************************************************
{
  TouchDone = false;

  TouchPxYx();

  if (TouchDone)
    {
      //  #########################################################################

      // Decrease Brightness <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PX > 220 && PY > 130)  //  Tap on Lower RHS
        {
          BarNumb--;

          if (BarNumb < 1) BarNumb = 1;

          Brightness = map(BarNumb, 1, 10, 100, 750);

          ledcWrite(ScreenLED, Brightness);  //  Turn on LED Down in Brightness

          BrightnessDisplay();

          // write Brightness to EEPROM(84)
          Serial.println("Writting to EEPROM");
          EEPROM.write(84, BarNumb);
          EEPROM.commit();
          delay(50);

          //Serial.print("Saved BarNumb in EEPROM(84) ");
          //Serial.print(BarNumb);
          //Serial.print("  --  Saved Value in EEPROM(84) ");
          //Serial.print(EEPROM.read(84));
          //Serial.print("  Brightness == ");
          //Serial.println(Brightness);
        }

      //  #########################################################################

      // Increase Brightness <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PX > 220 && PY < 110)  //  Tap on Upper RHS
        {
          BarNumb++;

          if (BarNumb > 10) BarNumb = 10;

          Brightness = map(BarNumb, 1, 10, 100, 750);

          //    Serial.print("BarNumb = ");
          //      Serial.print(BarNumb);
          //      Serial.print("  Brightness = ");
          //       Serial.println(Brightness);


          ledcWrite(ScreenLED, Brightness);  //  Turn on LED Up in Brightness

          BrightnessDisplay();

          // write Brightness to EEPROM(84)
          Serial.println("Writting to EEPROM");
          EEPROM.write(84, BarNumb);
          EEPROM.commit();
          delay(50);

          //Serial.print("Saved BarNumb in EEPROM(84) ");
          //Serial.print(BarNumb);
          //Serial.print("  --  Saved Value in EEPROM(84) ");
          //Serial.print(EEPROM.read(84));
          //Serial.print("  Brightness == ");
          //Serial.println(Brightness);
        }

      //  #########################################################################

      //  Temporary Display of Temp and Weather Forcast i.e.9am & 3pm..  ONLY IF BigDayDisplay
      if (PX > 70 && PX < 220 && PY > 155)
        {
          //  show TouchSpot
          // tft.drawRoundRect(70, 155, (220 - 70),(240-155), 4, TFT_WHITE);
          // delay(2000);

          if (BigDayDisplay)
            {
              TempWeatherDisp = true;  //  being true will trigger a timer in loop()
              FirstTime = true;
            }
        }



      //  #########################################################################

      if (!BigDayDisplay)  //  Only allow Weather if BigDayDisplay == false
        {
          //  Update Today's Forecast Tap Lower Middle Forecast Box  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX > 108 && PX < 217 && PY > 155)
            {
              //  show TouchSpot
              //tft.drawRoundRect(108, 155, (217 - 108), (240 - 155), 4, TFT_WHITE);
              //delay(2000);
              GetForcast = true;
              //Serial.println(">>>> GetForcast = true  -- Line 459");
              UpDateReason = "Update Forecast";
              CheckTime = false;
              UpDateGraphics(UpDateReason);

              //  No need to CheckTime here
              CheckTime = false;
              GoWeather();
            }

          //  Current Outside Temp      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX < 108 && PY > 155)  //  Tap Bottom Left Cnr over Inside Temperture
            {
              GetOutsideTemp();
            }

          //  Get Each Daily Forcast for next 4-5 days   Tap Top Left Cnr over the Day Name  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (PX < 160 && PY < 60)
            {
              //  show TouchSpot
              // tft.drawRoundRect(0, 0, 160, 60, 4, TFT_WHITE);
              // delay(2000);

              EachDayWeather();
            }
        }

      //  #########################################################################

      //  Main Menu for All Settings including Start WiFiManager      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      if (PY > 70 && PY < 150 && PX > 35 && PX < 110)  //  Tap over the Second Time Number from the Left
        {

          //  show TouchSpot
          // tft.drawRoundRect(35, 70, 110, 150, 4, TFT_WHITE);
          // delay(2000);

          MainMenu();

          if (SelectionYesNo)
            {
              WiFiSetup();  //  YES
            }

          tft.fillScreen(TFT_BLACK);
          CleanTimeSlot();
          DrawLines();
          //ShowTime();
          SensorDelay = 0;  //  Avoid Delay for Temperature Sensor
          LastSensorDelay = millis();

          DailyUpdate();
          TemperatureSensor();
        }

      //  #########################################################################
    }
}

void GoSaveLocation()  //  *******************************************************
{
  //  eLocalCity (16 61~77) & eCountryCode (2 81~82)
  int LocCityLength = LocalCity.length();

  Serial.println("Writting to EEPROM");

  //Serial.print("LocalCity ==");
  //Serial.println(LocalCity);

  for (i = 1; i < LocCityLength + 1; i++)
    {
      EEPROM.write(i + 60, LocalCity[i - 1]);

      //Serial.print("@ i: ");
      //Serial.print(i + 60);
      //Serial.print("  Wrote: ");
      //Serial.println(char(EEPROM.read(i + 60)));
    }

  EEPROM.write(i + 60, '#');  //  Write "#" at end of String

  //Serial.print("@ i: ");
  //Serial.print(i + 60);
  //Serial.print("  Wrote: ");
  //Serial.println(char(EEPROM.read(i + 60)));

  for (i = 1; i < 3; i++)
    {
      EEPROM.write(i + 80, CountryCode[i - 1]);

      //Serial.print("@ i: ");
      //Serial.print(i + 80);
      //Serial.print("  Wrote: ");
      //Serial.println(char(EEPROM.read(i + 80)));
    }
  EEPROM.commit();
}

void MainMenu()  //  ***************************************************************
{
  boolean OK = false;
  TouchDone = false;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);

  //  Build Date Part of Menu
  //Serial.println(TimeDate);
  tft.drawCentreString(TimeDate, 160, 10, 1);  //  Build Version i.e. V# Time & Date

  int LnSpc = 40;

  //  #########################################################################

  //  City Location Part Of Menu  **********************
  int LTx = 10;
  int LTy = 10 + LnSpc;                  //  <<<<<<<
  tft.drawString("City:", LTx, LTy, 1);  //  5c (+1c) chracters == 6c x 12p = 72p

  //  if(PX > LRx && PX < (LRx + 158) && PY > LRy && PY < (LRy + 30)
  int LRx = LTx + 70;  //  == 80
  int LRy = LTy - 8;   //  8 pixels from Top of Box to Top of Text ** 35 - 8 = 27  ** 27 + 30 = 57

  tft.drawRoundRect(LRx, LRy, 158, 30, 4, TFT_WHITE);  //  Large Text Box

  //if(PX > (LRx + 160) && PY > LRy && PY < (LRy + 30)
  //if(PX > LocOKx && PY > LocOKy && PY < LocOKy + 30
  int LocOKx = LRx + 166;
  int LocOKy = LRy;

  tft.drawRoundRect(LocOKx, LocOKy, 30, 30, 4, TFT_WHITE);  //  Small OK Text Box  LRx + 160 = 290

  LTx = LRx + 6;
  tft.drawString(LocalCity, LTx, LTy, 1);
  tft.drawString("OK", LTx + 164, LTy, 1);

  //  #########################################################################

  //  WiFi part of Menu  ************************
  int WTx = 10;
  int WTy = LTy + LnSpc;                           //  <<<<<<<  +40
  tft.drawString("Start WiFi Menu", WTx, WTy, 1);  //  15c (+1c) chracters == 16c x 12p = 192p

  int WRx = WTx + 190;
  int WRy = WTy - 8;                                  //  35 - 8 = 27  >> 27 + 30 = 57
  tft.drawRoundRect(WRx, WRy, 30, 30, 4, TFT_WHITE);  // tft.drawRect(XRect, YRect, XRL, YRH, TFT_WHITE);
  tft.drawString("GO", WTx + 194, WTy, 1);

  //  #########################################################################

  //  Min Brightness part of Menu  ****************************
  int BTx = 10;
  int BTy = WTy + LnSpc;  //  <<<<<<<  +40 for eack line

  tft.drawString("Min Bright:", BTx, BTy, 1);  //  11c (+1c) chracters == 12c x 12p = 144p

  int BRx = BTx + 144;                                //  == 154
  int BRy = BTy - 8;                                  //  35 - 8 = 27  ** 27 + 30 = 57
  tft.drawRoundRect(BRx, BRy, 90, 30, 4, TFT_WHITE);  //  Large Text Box

  int BrOKx = BRx + 98;
  int BrOKy = BTy - 8;
  tft.drawRoundRect(BrOKx, BrOKy, 30, 30, 4, TFT_WHITE);  //  Small OK Text Box  LRx + 160 = 290
  tft.drawString("OK", BrOKx + 4, BTy, 1);

  String MinBrightStr = "- " + String(MinBright) + " +";
  tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

  //  #########################################################################

  //  Big Day Display part of Menu  **************************** BigDayDisplay
  int BDDx = 10;
  int BDDy = BrOKy + LnSpc + 2;  //  <<<<<<<  +40 for eack line

  tft.drawString("Day/Weather:", BDDx, BDDy, 1);  //  11c (+1c) chracters == 12c x 12p = 144p

  int BDRx = BDDx + 144;
  int BDRy = BDDy - 8;                                   //  35 - 8 = 27  ** 27 + 30 = 57
  tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_WHITE);  //  Large Text Box

  if (BigDayDisplay)  //  ON
    {
      BigDayToggle = "Show Big Day";
    }
  else  //  OFF
    {
      BigDayToggle = "Show Weather";
    }

  tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);

  //  #########################################################################

  //  RETURN part of Menu  ************************
  int RnTx = 116;
  int RnTy = 200;
  int RnRx = 108;
  int RnRy = RnTy - 8;                                   //  35 - 8 = 27  >> 27 + 30 = 57
  tft.drawRoundRect(RnRx, RnRy, 120, 30, 4, TFT_WHITE);  // tft.drawRect(XRect, YRect, XRL, YRH, TFT_WHITE);
  tft.drawString(" RETURN ", RnTx + 4, RnTy, 1);

  //  #########################################################################

  unsigned long TimeOut = millis();

  //*******************************************************
  while (!OK)
    {
      TouchPxYx();

      //  #########################################################################

      //  OK box  for  Location Part Of Menu  **********************
      if (PX > LocOKx && PY > LocOKy && PY < LocOKy + 30 && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(LRx + 166 + 4, LTy - 8 + 4, 22, 22, 4, TFT_CYAN);

          //Serial.print("Selected LocalCity == ");
          //Serial.println(LocalCity);

          GoSaveLocation();  //  Save in EEPROM eLocalCity (16 61~77) & eCountryCode (2 81~82)

          GetForcast = true;
          GoWeather();
          TouchDone = false;
        }

      //  #########################################################################

      //  Rotate Location Names  for  Location Part Of Menu  **********************
      if (PX > LRx && PX < (LRx + 158) && PY > LRy && PY < (LRy + 30) && TouchDone)
        {
          DispCity++;
          if (DispCity > 9)
            {
              DispCity = 0;  //  Back to Top of List
            }

          LocalCity = LocalCityArray[DispCity][0];
          CountryCode = LocalCityArray[DispCity][1];

          //  clear text box
          //tft.drawRect(LRx, LRy, 158, 30, TFT_WHITE); //  Large Text Box
          tft.fillRoundRect(LRx + 1, LRy + 1, 156, 28, 4, TFT_BLUE);  //  Blank Large Text Box
          tft.drawString(LocalCity, LTx, LTy, 1);
        }

      //  #########################################################################

      //  StartWiFiMenu()  ***********************
      if (PY > WRy && PY < (WRy + 30) && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(WRx + 4, WRy + 4, 22, 22, 4, TFT_CYAN);
          TouchDone = false;
          delay(500);
          StartWiFiMenu();
        }

      //  #########################################################################

      //  MinBright part of Menu  ****************************
      // MinBright UP
      if (PX > BRx + 45 && PY > BRy && PX < BRx + 90 && PY < BRy + 30 && TouchDone)
        {
          MinBright++;

          //Serial.print("MinBright ");
          //Serial.println(MinBright);

          if (MinBright > 98)
            {
              MinBright = 99;       //  Set Max MinBright at 99
              MinBrightSw = false;  // MinBrightness Switched OFF
            }
          else
            {
              MinBrightSw = true;  // MinBrightness Switched ON
            }
          tft.fillRoundRect(BRx + 1, BRy + 1, 88, 28, 4, TFT_BLUE);  //  Blank Out Text Box

          MinBrightStr = "- " + String(MinBright) + " +";
          tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

          //tft.drawNumber(MinBright, BRx + 25, BTy, 1);
          if (MinBrightSw)
            {
              ledcWrite(ScreenLED, MinBright);  //  Turn on LED Up in Brightness
            }
          else
            {
              ledcWrite(ScreenLED, Brightness);  //  Turn on LED to Normal Brightness
            }
        }

      //  #########################################################################

      // MinBright DN
      if (PX > BRx && PY > BRy && PX < BRx + 45 && PY < BRy + 30 && TouchDone)
        {
          MinBright--;
          MinBrightSw = true;  // MinBrightness Switched ON

          //Serial.print("MinBright ");
          //Serial.println(MinBright);

          if (MinBright < 1)
            {
              MinBright = 1;
            }
          tft.fillRoundRect(BRx + 1, BRy + 1, 88, 28, 4, TFT_BLUE);  //  Blank Out Text Box

          MinBrightStr = "- " + String(MinBright) + " +";
          tft.drawString(MinBrightStr, BRx + 10, BTy, 1);

          //tft.drawNumber(MinBright, BRx + 25, BTy, 1);

          ledcWrite(ScreenLED, MinBright);  //  Turn on LED Dn in Brightness
        }

      //  #########################################################################

      // MinBright OK
      if (PX > BrOKx && PY > BrOKy && PX < BrOKx + 30 && PY < BrOKy + 30 && TouchDone)
        {
          OK = true;
          tft.fillRoundRect(BrOKx + 4, BrOKy + 4, 22, 22, 4, TFT_CYAN);

          // write MinBright to EEPROM(83)
          Serial.println("Writting to EEPROM");
          EEPROM.write(83, MinBright);
          EEPROM.commit();
          //Serial.print("Saved MinBright in EEPROM(83) ");
          delay(500);
        }

      //  #########################################################################

      //  Big Day Display part of Menu  **************************** BigDayDisplay
      if (PX > BDRx && PY > BDRy && PX < BDRx + 150 && PY < BDRy + 30 && TouchDone)  // CANCEL
        {
          if (BigDayDisplay)  //  Toggle OFF
            {
              BigDayDisplay = false;
              //Serial.println(">>>> BigDayDisplay == false  -- Line 712");
              DoBigDay = false;

              //  Dispaly Mode ==> Weather
              tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_WHITE);  //  Large Text Box
              BigDayToggle = "Show Weather";
              tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);


              // write BigDayDisplay to EEPROM(89)
              Serial.println("Writting to EEPROM");
              EEPROM.write(89, 0);  //  0 == OFF
              EEPROM.commit();
              delay(50);
            }
          else  //  Toggle ON
            {
              BigDayDisplay = true;
              ShowDayDate = true;

              //  Dispaly Mode ==> Big Day Display
              tft.drawRoundRect(BDRx, BDRy, 150, 30, 4, TFT_WHITE);  //  Large Text Box
              BigDayToggle = "Show Big Day";
              tft.drawString(BigDayToggle, BDRx + 4, BDDy, 1);

              //Serial.println(">>>> BigDayDisplay == true  -- Line 723");
              DoBigDay = true;
              // GetForcast = true;  // Gaz - This is not needed here

              // write BigDayDisplay to EEPROM(89)
              Serial.println("Writting to EEPROM");
              EEPROM.write(89, 1);  //  1 == ON
              EEPROM.commit();
              delay(50);
            }
        }

      //  #########################################################################

      //  RETURN part of Menu  ************************
      if (PX > RnRx && PY > RnRy && PX < RnRx + 120 && PY < RnRy + 30 && TouchDone)  // CANCEL
        {
          OK = true;
          tft.fillRoundRect(RnRx + 4, RnRy + 4, 112, 22, 4, TFT_CYAN);
          tft.setTextColor(TFT_BLUE, TFT_CYAN);  //  Reverse Colours >BLUE< Text and >CYAN< Background
          tft.drawString(" RETURN ", RnTx + 4, RnTy, 1);
          delay(500);
        }

      //  #########################################################################

      if (TouchDone)
        {
          TimeOut = millis();
        }

      if (millis() > TimeOut + 5000)
        {
          OK = true;  // exit While Loop after 5 seconds
        }

      TouchDone = false;
    }
}

void StartWiFiMenu()  //  *****************************************************
{
  //Serial.println("void StartWiFiMenu()");

  int Tx = 160;  //  Middle of screen
  int Ty = 35;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);

  tft.drawCentreString(">>>  YES  <<<", Tx, Ty, 1);

  Ty = 110;
  tft.drawCentreString("***  WiFiManager?  ***", Tx, Ty, 1);

  Ty = 240 - 45;
  tft.drawCentreString(">>>  NO  <<<", Tx, Ty, 1);

  NoSelection = true;
  TouchDone = false;

  int WiFiMenuDelay = 2000;  //  only display access to WiFiManager for 2 seconds
  uint32_t LastMenuDelay = millis();

  while (millis() - LastMenuDelay < WiFiMenuDelay)  //  true will LOOP  NoSelection
    {
      TouchPxYx();

      if (PY < 110 && TouchDone)
        {
          //  YES
          NoSelection = false;    //  Selection Made
          SelectionYesNo = true;  //  Yes go to WiFiManager
          //Serial.println("Pressed YES");
          TouchDone = false;
        }
      else if (PY > 130 && TouchDone)
        {
          //  NO
          NoSelection = false;     //  Selection Made
          SelectionYesNo = false;  //  NO
          //FailAttempt = 0;
          //Serial.println("Pressed NO");
          TouchDone = false;
        }
    }
}

void BrightnessDisplay()  //  *************************************************
{
  //Serial.println("void BrightnessDisplay()");

  tft.fillRect(280, 43, 40, 111, TFT_BLACK);  //  154 - 43 = 111

  //Serial.print("BarNumb == ");
  //Serial.println(BarNumb);

  for (i = 1; i < BarNumb + 1; i++)
    {
      //  Draw lines between X = 145 ~ 50
      int Ly = map(i, 1, 10, 145, 50);
      tft.drawLine(285, Ly, 315, Ly, TFT_WHITE);
    }

  BrightCntDwn = 750;  //  Hold display bars for 0.75 Sec
  BrightnessBarsStart = millis();
  DelayTime = true;
}

void GoLDR()  //  *******************************************************
{
  if (millis() - LastLdrDelay > LdrDelay)  //  Avoid millis() overflow
    {
      LdrDelay = 10000;  //  10 sec delay - if your hand shadows the LDR the screen won't dim straight away
      LastLdrDelay = millis();

      double SumLDR;
      for (i = 0; i < 10; i++)
        {
          SumLDR = SumLDR + analogRead(pResLDR);  //  Using 82k Resistor
          delay(5);
        }

      ReadLDR = SumLDR / 10;
      //Serial.print("ReadLDR == ");
      //Serial.println(ReadLDR);

      if (Testing)
        {
          //  For Testing without LDR connected- GAZ
          Serial.println("Testing without LDR * LdrDimmer set to 100 - Line 1338");
          ReadLDR = 100;
        }

      LdrDimmer = ReadLDR;

      //Serial.print("LdrDimmer = ");
      //Serial.println(LdrDimmer);

      // tft.drawString(String(LdrDimmer), 140, 89, 2);  //  only used for testing

      // Set NightTime Brightness
      if (LdrDimmer >= 9)  //  Turn UP screen  was 10
        {
          SetLdrDimmer = Brightness;
          LdrDelay = 10000;  // was 10000 == 10 sec - if your hand shadows the LDR the screen won't dim
        }
      else  //  Turn DOWN Screen
        {
          SetLdrDimmer = MinBright;
          LdrDelay = 500;  //  1/2 sec - Brightens quickly if Light is turned ON

          //  Test for SunRise plus 60mins  atoi(String); //converts string to int
          if (Hour > SunRiseHour.toInt() && Minute > SunRiseMin.toInt() && Hour < SunSetHour.toInt())  // Check for whole day is case bad weather
            {
              if (Testing)
                {
                  Serial.println("Testing SetDimmer Settings for SunRise - Line 1365");
                }

              SetLdrDimmer = SetLdrDimmer + 9;  //  Make Screen slightly brighter than min
            }
        }

      ledcWrite(ScreenLED, SetLdrDimmer);  // No Lights on -- Turn down LED to LdrDimmer setting

      //Serial.print("ReadLDR == ");
      //Serial.print(ReadLDR);
      //Serial.print("   SetLdrDimmer == ");
      //Serial.println(SetLdrDimmer);
    }
}

void DisplayOutTemp(int ListNumb)  //  ********************************************
{
  //Serial.print("Outside Temperature: ");
  //Serial.println(myObject["list"][ListNumb]["main"]["temp"]);
  //Serial.println("*********************");

  double TempC = myObject["list"][ListNumb]["main"]["temp"];

  int DecTemp = abs((TempC - (int)(TempC)) * 10.00);  //  Use with Deg plus Decimal Deg

  int xT = 15;
  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Outside Temp", xT, yT, 2);

  yT = yT + 20;
  tft.setTextFont(4);
  tft.drawNumber(TempC, xT, yT, 6);  // drawNumber == int  ::  drawFloat == float number
  tft.setCursor(xT + 55, yT - 4);    //  -4 for Decimal Rereadout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void GetOutsideTemp()  //  ***************************************************
{
  // Display Current Outside Temp

  DisplayOutTemp(0);  //  Goto void DisplayOutTemp() where 0 == ListNumb

  //  Show Sunset and Sunrise Icons and Times
  //  For SunRise --> Slot 1   For SunSet --> Slot 2
  ShowForecast("SunRise", "SunSet");  //  Go get Forecast Icons for each Time Slot

  //  SunRise or SunSet Icons Times as Text
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  String SunRiseTime = SunRiseHour + ":" + SunRiseMin +"am";

  tft.drawCentreString(SunRiseTime, xTime1Slot, yTime1Slot, 2);

  String SunSetTime = "0" + String(SunSetHour.toInt() - 12) + ":" + SunSetMin + "pm";

  tft.drawCentreString(SunSetTime, xTime2Slot, yTime2Slot, 2);

  DailyUpdateCall = true;  //  Clears Sunrise amd Sunset Icons and replaces them with Forecasts.
                           //  Is cleared in TemperatureSensor()

  SensorDelay = 5000;  //  This maintains the display of the Outside Temp and Sun Rise/Set for 5 sec

  LastSensorDelay = millis();
}

void CleanTimeSlot()
{
  //Serial.println("void CleanTimeSlot()");
  //  clear Area before new update display
  tft.fillRect(0, 42, 320, 110, TFT_BLACK);
  ShowTime();
}

void UpDateGraphics(String TextDisplay)  //  ******************************************************************
{
  //Serial.print("void UpDateGraphics(String TextDisplay) == ");
  //Serial.println(TextDisplay);

  //  Blackout Rect    //  Time Area  0, 42, (w) 320, (h) 110
  //  Most Warnings or Info Notices are Displayed in the Bottom Section
  int Xlng = TextDisplay.length() * 14;  //190;
  int Xtc = 217 - Xlng / 2;              //217 == centre of Text to be Displayed
  int Ytc = 180;                         //  down in the Forecast Area

  if (TextDisplay == " Forecasts ")  //  This Display in Top Section Only
    {
      Xtc = 164;
      Ytc = 0;
    }

  int Ylng = 40;

  int XRect = Xtc + 5;  //  1st White Box around Text
  int YRect = Ytc + 5;
  int XRL = Xlng - 10;
  int YRH = Ylng - 10;

  int Xtxt = XRect + 8;  //  Text
  int Ytxt = YRect + 8;

  tft.fillRect(Xtc, Ytc, Xlng, Ylng, TFT_BLUE);

  //  Draw 2 White Boxes - 1 box is not thick enough
  tft.drawRect(XRect, YRect, XRL, YRH, TFT_WHITE);
  tft.drawRect(XRect + 1, YRect + 1, XRL - 2, YRH - 2, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setTextFont(1);
  tft.setCursor(Xtxt, Ytxt);
  tft.print(TextDisplay);  //  Update Forcast
}

void WiFiGraphics()  //  ************UpDateGraphics("Checking Internet");***********************************************************
{
  //Serial.println("void WiFiGraphics()");

  //  Print some Info to TFT_Screen
  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setTextFont(1);

  int MenuY = 10;

  tft.drawCentreString("Gazza's Time and Weather", 160, MenuY, 1);  //  was 2

  MenuY = MenuY + 23;

  tft.drawCentreString(ClockID + "  Connecting to", 160, MenuY, 1);  //  + 23  was 25

  MenuY = MenuY + 25;
  drawBitmap(96, MenuY, Graphic_WIFI, 128, 89, TFT_CYAN);  //  + 27  was 50

  MenuY = MenuY + 96;
  tft.drawCentreString(ssid, 160, MenuY, 1);  //   was 148

  MenuY = MenuY + 24;
  tft.drawCentreString(TimeDate, 160, MenuY, 1);  //  + 24  was  172

  MenuY = MenuY + 28;
  tft.drawCentreString("Created August 2022", 160, MenuY, 1);  //  + 28  was 200
}

//  ****************************************************************************************
void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color)
{
  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte;

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          if (i & 7) byte <<= 1;
          else byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
          if (byte & 0x80) tft.drawPixel(x + i, y + j, color);
        }
    }
}

void Read1stEEPROM()  //  ****************************************************
{
  //Serial.println("void Read1stEEPROM()");

  //Serial.println("Reading EEPROM 1st position == " );
  //Serial.println(char(EEPROM.read(0)));

  if (char(EEPROM.read(0)) == 'Y')
    {
      ReadEEPROM();
    }
  else
    {
      //Serial.println(char(EEPROM.read(0)));
      //Serial.println("EEPROM empty -->> Goto Wifi Manager");
    }
}

void ReadEEPROM()  //  ****************************************************
{
  /*  28 characters max for ssid and password each - hopefully
     Initialasing EEPROM  (0) 'Y' or NULL,
     Length of ssid > (1), ssid > (2~30)
     Length of Password > (31), password > (32~60)
     EEPROM.begin(64);
  */
  // Read EEPROM for ssid and password
  //Serial.println("Reading EEPROM ssid");

  //  Read ssid.Length
  int ssidLength = EEPROM.read(1);

  for (i = 2; i < 2 + ssidLength; i++)
    {
      ssid += char(EEPROM.read(i));
    }

  //Serial.println();
  //Serial.print("SSID: ");
  //Serial.println(ssid);

  //Serial.println("Reading EEPROM password");
  //  Read pswd.Length
  int pswdLength = EEPROM.read(31);

  for (i = 32; i < 32 + pswdLength; i++)
    {
      password += char(EEPROM.read(i));
    }
  //Serial.print("Password: ");
  //Serial.println(password);
}

void GoWiFiManager()  //  ***********************************************
{
  //Serial.println("void GoWiFiManager()");

  int Tx = 160;  //  Middle of screen
  int Ty = 35;

  tft.fillScreen(TFT_BLUE);  //  clears screen to blue
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);

  tft.drawCentreString("*** New  WiFi? ***", Tx, Ty, 1);

  Ty = Ty + 35;
  tft.drawCentreString("Connect to", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString(">>>  WiFiManager  <<<", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString("Access Point Network", Tx, Ty, 1);

  Ty = Ty + 35;
  tft.drawCentreString("on nearest Device.", Tx, Ty, 1);

  Ty = Ty + 25;
  tft.drawCentreString("http://192.168.4.1", Tx, Ty, 1);
}

void WiFiSetup()  //  *******************************************************
{
  //Serial.println("void WiFiSetup() for WiFiManager");

  //  Show No Internet Screen and instructions to goto WiFiManager

  GoWiFiManager();

  //Serial.println("Starting WIFI_STA and then WiFiManager");

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // put your setup code here, to run once:
  //Serial.begin(115200);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  //Serial.println(">>Checking Connection<<");

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap

  /*  wm.autoConnect loops in the Header file until a conncetion is made
      so I can't tell the user via the TFT screen that the WiFi has failed
      if an incorrect WiFi name or password has been entered  */
  res = wm.autoConnect("WiFiManager", "password");  // password protected ap

  /*
  if (!res)  // For testing only
    {  //it doesn't do the next line - WiFiManager only returns here after Network found
      //Serial.println(">>Failed to connect<<");
      // ESP.restart();
    }
  else
    {
      //it doesn't do the next line - WiFiManager only returns here after Network found
      //if you get here you have connected to the WiFi
      //Serial.println(">>>>>>>>  connected...Yay!! :)  <<<<<<<<<");
    }
    */

  String Strssid = wm.getWiFiSSID();
  String Strpassword = wm.getWiFiPass();

  //Serial.print("WiFi wm.getWiFiSSID == ");
  //Serial.println(wm.getWiFiSSID());

  //Serial.print("WiFi wm.getWiFiPass == ");
  //Serial.println(wm.getWiFiPass());

  ssid = wm.getWiFiSSID();
  password = wm.getWiFiPass();

  Silent = false;

  SaveEEPROM = true;  //  Only Save to EEPROM if connection is validated

  SetUpWiFi();

  TryWiFi();
}

void GoWriteEEPROM()  //  ***********************************************************
{
  /*  28 characters max for ssid and password each - hopefully
       Initialasing EEPROM  (0) 'Y' or NULL,
       Length of ssid > (1), ssid > (2~30)
       Length of Password > (31), password > (32~60)
       EEPROM.begin(64);
  */

  Serial.println("void GoWriteEEPROM()");

  UpDateGraphics("Save WiFi > EEPROM");

  //Serial.println("Clearing EEPROM");
  for (i = 0; i < 60; i++)
    {
      EEPROM.write(i, 0);
    }

  //Serial.println("writing Y in EEPROM(0) ");
  EEPROM.write(0, 'Y');

  //Serial.println("writing ssid to EEPROM(2) ~ (30)");

  int ssidLength = ssid.length();

  if (ssidLength > 28)
    {
      UpDateGraphics("ERROR - Wifi Name to Long");
      delay(10000);

      ShowTime();

      ssidLength = 28;
    }

  //Serial.print("writing ssid.Length to EPPROM(1) : ");
  //Serial.println(ssidLength);
  EEPROM.write(1, ssidLength);

  //  Writting ssid to EEPROM(2) ~ Length of ssid
  for (i = 0; i < ssidLength; i++)
    {
      EEPROM.write(i + 2, ssid[i]);
      //Serial.print("Wrote: ");
      //Serial.println(ssid[i]);
    }

  //Serial.println("writing password to EEPROM(32) ~ (60)");

  int pswdLength = password.length();

  if (pswdLength > 28)
    {
      UpDateGraphics("ERROR - Password to Long");
      delay(10000);
      ShowTime();
      pswdLength = 28;
    }

  //Serial.print("writing password.Length to EPPROM(31) : ");
  //Serial.println(pswdLength);
  EEPROM.write(31, pswdLength);

  //  Writting password to EEPROM(32) ~ Length of password
  for (i = 0; i < pswdLength; i++)
    {
      EEPROM.write(i + 32, password[i]);
      //Serial.print("Wrote: ");
      //Serial.println(password[i]);
    }
  EEPROM.commit();
  //Serial.println("EEPROM.commit() Done..");

  delay(200);  //  Time for UpDateGraphics to be displayed.
  CleanTimeSlot();
}

void SetUpWiFi()  //  *************************************************************
{

  //Serial.println("void SetUpWiFi() >>> Restart WiFi");

  //Serial.println("Attempt to Connect to LAN");

  WiFi.mode(WIFI_STA);  //   create wifi station

  //Serial.println();
  //Serial.print("(1) ssid ==");
  //Serial.print(ssid);
  //Serial.print("   password ==");
  //Serial.println(password);

  WiFi.begin(ssid.c_str(), password.c_str());

  //Serial.println();
  //Serial.print("(2) ssid ==");
  //Serial.print(ssid);
  //Serial.print("   password ==");
  //Serial.println(password);*/

  delay(1000);
}

void TryWiFi()  //  *****************************************************************
{

  //Serial.print("void TryWiFi() - 1st check - InternetFail (true = 1) == ");
  //Serial.println(InternetFail);

  /*  if(InternetFail)
    {
      SetUpWiFi();
      FailAttempt++;
      Serial.print("FailAttempt == ");
      Serial.println(FailAttempt);
    }*/

  TimeOut = 0;

  InternetFail = true;

  while (InternetFail)  //  LOOP while not connected
    {
      TimeOut = 0;

      //Serial.println("while (InternetFail) ");

      while (WiFi.status() != WL_CONNECTED && TimeOut < 25)
        {
          //Serial.print(".");

          TimeOut++;

          if (!Silent)
            {
              tft.setCursor(10, 220);
              tft.print(DOT);
              DOT = DOT + ".";
            }
          delay(500);
        }

      if (TimeOut < 25)
        {
          //Serial.println("WiFi connection OK!");
          InternetFail = false;
          if (SaveEEPROM)
            {
              GoWriteEEPROM();
              SaveEEPROM = false;
            }
          delay(500);
        }
      else if (FirstBoot)  //  set to true at Boot Time -- Set to False at Line 1129 after successful WiFi
        {
          StartWiFiMenu();  //  Menu  >>  Yes Or NO

          if (SelectionYesNo)  //  true ==> YES
            {
              WiFiSetup();  //  WiFiManager
            }
          else  //  NO
            {
              //  try again
              DOT = ".";
              WiFiGraphics();
              SetUpWiFi();  //  Restart WiFi
            }
        }
      else
        {
          InternetFail = true;
          //Serial.println("No Internet - Try Later--  Will BREAK from while (InternetFail) ");
          UpDateGraphics("Check Internet");

          break;
        }
    }
}

void CheckWiFi()  //  *******************************************************************
{
  //Serial.println("void CheckWiFi()");

  while (TimeOut > 25)
    {
      UpDateGraphics("No Internet");
      //Serial.println("No Internet");
      delay(2000);

      //  Blank Screen
      tft.fillRect(0, 0, 320, 240, TFT_BLACK);
      WiFiSetup();  //  WiFiManager

      TryWiFi();
    }

  //Serial.print("FailAttempt == ");
  //Serial.println(FailAttempt);

  if (FailAttempt > 1)
    {
      FailAttempt = 0;
      InternetFail = false;
      //StartWiFiMenu();
    }
}

void GoInternet()  //  ******************************************************************
{
  //Serial.println("void GoInternet()");
  delay(100);
  //  First time connect to WiFi
  if (ssid == NULL)
    {
      //Serial.println("1.. ssid == NULL");
      //  Read first EEPROM position for "Y"
      Read1stEEPROM();  //  and the rest of ssid & password if available
    }
  else if (FirstBoot)
    {
      //  ssid != NULL therefore Hard Coded WiFi Data
      GoWriteEEPROM();  //  force saving of Hard Coded WiFi Data
    }


  //  If ssid still NULL - do WiFiManager
  if (ssid == NULL)
    {
      //Serial.println("2.. ssid == NULL");
      StartWiFiMenu();  // -- Gives a choice to goto WifiManager or not

      if (NoSelection)  //  ==  true or YES
        {
          WiFiSetup();  // WiFiManager
        }
    }

  if (!Silent)
    {
      WiFiGraphics();
    }

  //need to connect to WiFi to get internet GMT time
  //Serial.println("Connecting to :");
  //Serial.println(ssid);

  SetUpWiFi();  //  Restart WiFi
  TryWiFi();    //  See if WiFi is connection or open

  //  CheckWiFi();  // may not be needed  -  rechecks timeout value - LOOP

  //  everthing should be good...
  FirstBoot = false;

  Silent = true;  //  do not show WiFiGraphics() screen

  //Serial.println("");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
}

void GetWWWTime()  //  *****************************************************
{
  //Serial.println("void GetWWWTime()");

  if (CheckTime)
    {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  //  This one line get GMT time from ntpServer.. :)

      struct tm timeinfo;

      int FailCnt = 0;

      while (!getLocalTime(&timeinfo))
        {
          FailCnt++;
          //Serial.print("Failed to obtain time :");
          //Serial.println(FailCnt);

          tft.fillScreen(TFT_RED);
          tft.setTextColor(TFT_WHITE, TFT_RED);
          tft.setTextSize(2);
          tft.drawCentreString("Failed to obtain time", 160, 100, 1);
          tft.drawCentreString("No Internet??", 160, 130, 1);
          tft.drawNumber(FailCnt, 150, 170, 1);

          delay(2000);

          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  //  This one line get GMT time.. :)

          tm timeinfo;
        }

      Year = timeinfo.tm_year + 1900;  //  Add 1900
      //Serial.println(Year);

      Month = timeinfo.tm_mon + 1;  //  Where Jan = timeinfo.tm_mon + 1 for RTC
      //Serial.println(Month);

      nDay = timeinfo.tm_mday;  //  Day Date Number 1 -> 28, 30 or 31
      //Serial.print("First time get nDay == ");
      //Serial.println(nDay);

      wDay = timeinfo.tm_wday;  //  day of week [0,6] (Sunday = 0)

      LstDay = nDay;  //  used for Time update at Midnight.

      Hour = timeinfo.tm_hour;

      Minute = timeinfo.tm_min;


      //  GAZ TIME CHECK - Start Time anytime HERE
      /*if(DoOnce)  //  **************Testing***************
      {
       Hour = 8;     //  ***************Testing******************
       Minute = 54;  //  *************Testing******************
       DoOnce = false;

       Serial.print("Reset Time to before ");
       Serial.print(Hour + 1);
       Serial.println(" <<<<<<<<<<<<<<<");
       Serial.println();
      }*/

      //Serial.println(Minute);

      Second = timeinfo.tm_sec;

      /* if(Second + 1 < 60)
      {
      Second = Second + 1;
      }*/

      //Serial.println(Second);

      //  To Set Time in RTC from Internet Time - Put updated Time into RTC
      //  rtc.adjust(DateTime(Year, Month, nDay, Hour, Minute, Second));
      //Serial.println("******************************************************");
      //Serial.print("NTP Day & Time : ");
      //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      //Serial.println("******************************************************");

      TimeSync = true;  //  This should only be here

      FirstMillis = millis();  //  Using this method eliminates possible millis() overflow after 49+ days

      /*  //  This method will be affected by millis() overflow at 49+ days
      CountSec = 1;        //  need to reset at MidNight
      targetTime = FirstMillis + CountSec * 1000;  //  this ensure that the time is locked to internal ESP32 clock
      */


      //  Found this -->  to put Above Formatted &timeinfo Output into a single string
      /*
      char timeStringBuff[50]; //50 chars should be enough
      strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      //print like "const char*"
      Serial.println(timeStringBuff);

      //Optional: Construct String object
      String asString(timeStringBuff);*/
    }
}

void GoWeatherMap()  //  ****************************************************************
{
  //Serial.println("void GoWeatherMap()");

  //Serial.print("Length(jsonBuffer) Before Getting New Data == ");
  //Serial.println(jsonBuffer.length());

  //String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEY;

  //  https://openweathermap.org/forecast   5 day / 3 hourly forecast data  ==> LocalCity
  String serverPath = "http://api.openweathermap.org/data/2.5/forecast?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEY;

  //  https://pro.openweathermap.org/data/2.5/forecast/hourly?lat=35&lon=139&appid={API key}
  //String APIKEYhourly = "c834c5045583e40972f8ae334fef0e66";
  //String serverPath = "https://pro.openweathermap.org/data/2.5/forecast/hourly?q=" + LocalCity + "," + CountryCode + "&units=metric&APPID=" + APIKEYhourly;

  jsonBuffer = httpGETRequest(serverPath.c_str());

  //Serial.print("NEW Length(jsonBuffer) == ");
  //Serial.println(jsonBuffer.length());

  //  JSONVar Declaration moved to Global by Gaz
  myObject = JSON.parse(jsonBuffer);

  //Serial.println("Created NEW myObject = JSON.parse(jsonBuffer)");

  //  Printout Data in one continual string
  //Serial.print("JSON object = ");  // Will be jumbled but just copy it and paste to notepad
  //Serial.println(myObject);
  //Serial.println();

  int JsonCnt = 0;

  while ((JSON.typeof(myObject) == "undefined") && JsonCnt < 3)
    {
      //Serial.println("Parsing input failed! - try again");

      JsonCnt++;

      //Serial.print("JsonCnt = ");
      //Serial.println(JsonCnt);

      //NonClockDelay(500);
      //delay(500);

      jsonBuffer = httpGETRequest(serverPath.c_str());
      myObject = JSON.parse(jsonBuffer);

      //Serial.print("NEW Length(jsonBuffer) == ");
      //Serial.println(jsonBuffer.length());
      //  Printout Data in one continual string
      //Serial.println();
      //Serial.println("JSON object = Gaz Set - No Printout");
      //Serial.println(myObject);
      //Serial.println();
    }

  if (JsonCnt > 3)
    {
      //Serial.print("No Json File : Number of Attempts = ");
      //Serial.println(JsonCnt);
      InternetFail = true;
    }
}

void CheckInternet()  //  *************************************************
{
  //Serial.println("void CheckInternet()");

  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED)
    {
      //Serial.print("WiFi.status : ");
      //Serial.println(WiFi.status());   //  3 == Connected??
      InternetFail = false;
    }
  else
    {
      //Serial.println("WiFi Disconnected - Will try Reconnect");
      Silent = true;
      InternetFail = true;

      GoInternet();  //  hopefully reconnect
    }
}

void Case10am()  //  *************************************************
{
  for (i = 0; i < 10; i++)
    {
      //Serial.println(myObject["list"][i]["dt_txt"]);

      TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new library must have changed the process

      //Serial.print("Test TimeSlot :");
      //Serial.println(TestTime);
      //Serial.println(TestTime.substring(11, 13));

      //  Time == 10am Brisbane == 00:00 UTC same day..
      if (TestTime.substring(11, 13) == "00")  //  00 == 10am
        {
          TimeSlot1st = i;

          //  look for Thunderstorm or Drizzle
          ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["main"];

          if (String(ForeCast1stSlot) != "Thunderstorm" || String(ForeCast1stSlot) != "Drizzle")
            {
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
            }

          ForeCast1stSlotStr = String(ForeCast1stSlot);
          //Serial.print("Found 10am time slot: ");
          //Serial.println(i);

          //  Time == 4pm Brisbane == 06:00 UTC same day..
          //  Therefore TimeSlot2nd == TimeSlot1st + 2
          TimeSlot2nd = TimeSlot1st + 2;

          //  look for Thunderstorm or Drizzle
          ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["main"];

          if (String(ForeCast2ndSlot) != "Thunderstorm" || String(ForeCast2ndSlot) != "Drizzle")
            {
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
            }
          ForeCast2ndSlotStr = String(ForeCast2ndSlot);
          //Serial.print("Therefore 4pm time slot == ");
          //Serial.println(TimeSlot2nd);

          break;  //  to break out of for() loop
        }
    }

  //Serial.print("Weather @ 10am: ");
  //Serial.println(myObject["list"][TimeSlot1st]["dt_txt"]);
  //Serial.println(ForeCast1stSlotStr);    //  myObject["list"][TimeSlot1st]["weather"][0]["description"]);


  //Serial.print("Weather @ 4pm: ");
  //Serial.println(myObject["list"][TimeSlot2nd]["dt_txt"]);
  //Serial.println(ForeCast2ndSlotStr);   //  myObject["list"][TimeSlot2nd]["weather"][0]["description"]);
  //Serial.println("***************************");

  //  go get variables for Forecast
  ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Go get Forecast Icons for each Time Slot

  //  Forecasts Icons Text Names
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  String CityForecast = "9am " + ForeCast1stSlotStr;  //   String(City) +
  tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

  CityForecast = "3pm " + ForeCast2ndSlotStr;  //  String(City) +
  tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);
}

void Case4pm()  //  *************************************************
{
  for (i = 0; i < 10; i++)
    {
      //Serial.println(myObject["list"][i]["dt_txt"]);

      TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new liburary must have changed the process

      //  Time == 4pm Brisbane == 06:00 UTC same day..
      if (TestTime.substring(11, 13) == "06")  //  06 UTC == 4pm Brisbane
        {
          //Serial.print("Test TimeSlot : ");
          //Serial.println(TestTime);

          TimeSlot1st = i;

          //  look for Thunderstorm or Drizzle in Json "main"
          ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["main"];

          if (String(ForeCast1stSlot) != "Thunderstorm" || String(ForeCast1stSlot) != "Drizzle")
            {
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
            }

          ForeCast1stSlotStr = String(ForeCast1stSlot);
          //Serial.print("Found 4pm time slot: ");
          //Serial.println(i);

          //  Found (06 UTC) 4pm next Timeslot is 7pm so add 1
          TimeSlot2nd = TimeSlot1st + 1;

          //  look for Thunderstorm or Drizzle in Json "main"
          ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["main"];

          if (String(ForeCast2ndSlot) != "Thunderstorm" || String(ForeCast2ndSlot) != "Drizzle")
            {
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
            }

          ForeCast2ndSlotStr = String(ForeCast2ndSlot);
          //Serial.print("Therefore 7pm time slot: ");
          //Serial.println(TimeSlot2nd);

          break;  //  to break out of for() loop
        }
    }

  //Serial.print("Weather @ 4pm: ");
  //Serial.println(myObject["list"][TimeSlot1st]["dt_txt"]);
  //Serial.println(ForeCast1stSlotStr);    //  myObject["list"][TimeSlot1st]["weather"][0]["description"]);


  //Serial.print("Weather @ 7pm: ");
  //Serial.println(myObject["list"][TimeSlot2nd]["dt_txt"]);
  //Serial.println(ForeCast2ndSlotStr);   //  myObject["list"][TimeSlot2nd]["weather"][0]["description"]);
  //Serial.println("***************************");

  //  go get variables for Forecast
  ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Go get Forecast Icons for each Time Slot

  //  Forecasts Icons Text Names
  tft.setTextSize(1);
  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  String CityForecast = "3pm " + ForeCast1stSlotStr;  //   String(City) +
  tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

  CityForecast = "6pm " + ForeCast2ndSlotStr;  //  String(City) +
  //Serial.println(ForeCast2ndSlotStr);  //  just a check for correct string
  tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);
}

void ShowForecast(String Slot1, String Slot2)  //  ************************************************************
{
  //Serial.println("void ShowForecast(String Slot1, String Slot2)");

  //  Draw Weather Forecasts in lower 2 Boxes..
  //  Also Draw SunRise and SunSet Icons and Times with Outside Temperature when requested

  XX = 110;  //  for blacking out Icons
  YY = 157;
  Xw = 107;  //  217 - 108 == 109
  Yh = 83;   //  240 - 157 == 83

  tft.fillRect(XX, YY, Xw, Yh, TFT_BLACK);  //  black out Icons and Descriptions
  tft.fillRect(XX + Xw + 2, YY, Xw, Yh, TFT_BLACK);

  int S1x = 135;

  int BitMapColour = TFT_LIGHTGREY;

  if (Slot1 == "clear sky") drawBitmap(S1x, 180, clearsky, 60, 60, BitMapColour);

  if (Slot1 == "few clouds") drawBitmap(S1x, 180, fewclouds, 60, 60, BitMapColour);

  if (Slot1 == "scattered clouds")
    {
      drawBitmap(S1x, 180, scatteredclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "scattered";
    }

  if (Slot1 == "broken clouds")
    {
      drawBitmap(S1x, 180, brokenclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "partly";
    }

  if (Slot1 == "overcast clouds")
    {
      drawBitmap(S1x, 180, overcastclouds, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "overcast";
    }

  if (Slot1 == "light rain") drawBitmap(S1x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot1 == "moderate rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot1 == "heavy intensity rain")
    {
      drawBitmap(S1x, 180, heavyrain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "heavy";  //  shorten or change string wording
    }

  if (Slot1 == "very heavy rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "very heavy";  //  shorten or change string wording
    }

  if (Slot1 == "extreme rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "freezing rain")
    {
      drawBitmap(S1x, 180, hail, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "hail";  //  shorten or change string wording
    }

  if (Slot1 == "light intensity shower rain")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot1 == "shower rain") drawBitmap(S1x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot1 == "heavy intensity shower rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "ragged shower rain")
    {
      drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot1 == "Thunderstorm") drawBitmap(S1x, 180, thunderstorm, 60, 60, BitMapColour);

  if (Slot1 == "Drizzle")
    {
      drawBitmap(S1x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }

  //  Using Forecast Subroutine for Sunrise and Suset
  if (Slot1 == "SunRise")
    {
      drawBitmap(S1x, 180, sunrise, 60, 60, BitMapColour);
      //  ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }


  //  ************ SLOT 2 *********************

  int S2x = 245;

  if (Slot2 == "clear sky") drawBitmap(S2x, 180, clearsky, 60, 60, BitMapColour);

  if (Slot2 == "few clouds") drawBitmap(S2x, 180, fewclouds, 60, 60, BitMapColour);

  if (Slot2 == "scattered clouds")
    {
      drawBitmap(S2x, 180, scatteredclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "scattered";
    }

  if (Slot2 == "broken clouds")
    {
      drawBitmap(S2x, 180, brokenclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "partly";
    }

  if (Slot2 == "overcast clouds")
    {
      drawBitmap(S2x, 180, overcastclouds, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "overcast";
    }

  if (Slot2 == "light rain") drawBitmap(S2x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot2 == "moderate rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot2 == "heavy intensity rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "heavy";  //  shorten or change string wording
    }

  if (Slot2 == "very heavy rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "very heavy";  //  shorten or change string wording
    }

  if (Slot2 == "extreme rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "freezing rain")
    {
      drawBitmap(S2x, 180, hail, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "hail";  //  shorten or change string wording
    }

  if (Slot2 == "light intensity shower rain")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "moderate";  //  shorten or change string wording
    }

  if (Slot2 == "shower rain") drawBitmap(S2x, 180, lightrain, 60, 60, BitMapColour);

  if (Slot2 == "heavy intensity shower rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "ragged shower rain")
    {
      drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "stormy";  //  shorten or change string wording
    }

  if (Slot2 == "Thunderstorm") drawBitmap(S2x, 180, thunderstorm, 60, 60, BitMapColour);

  if (Slot2 == "Drizzle")
    {
      drawBitmap(S2x, 180, moderaterain, 60, 60, BitMapColour);
      ForeCast2ndSlotStr = "drizzle";  //  shorten or change string wording
    }

  //  Using Forecast Subroutine for Sunrise and Suset
  if (Slot2 == "SunSet")
    {
      drawBitmap(S2x, 180, sunset, 60, 60, BitMapColour);
      //  ForeCast1stSlotStr = "drizzle";  //  shorten or change string wording
    }
}

void DisplayMinTemp(int ListNumb)  //  ********************************************
{
  //  we have to test 12 UTC and 15 UTC for the lowest Mim Temperature..
  ListNumb = ListNumb - 2;                                             //  ListNumb == 00  (-2) == 18 UTC
  double TempC = myObject["list"][ListNumb]["main"]["temp_min"];       //  18 UTC or 4am Day before
  double Temp2C = myObject["list"][ListNumb + 1]["main"]["temp_min"];  //  21 UTC or 7am

  if (Temp2C < TempC)
    {
      TempC = Temp2C;
    }

  int DecTemp = (TempC - (int)TempC) * 10.00;  //  Use with Deg plus Decimal Deg

  //Serial.print("Minium Temperature: ");
  //Serial.println(TempC);
  //Serial.println("*********************");

  //  clear Deg area
  tft.fillRect(0, 170, 107, 200, TFT_BLACK);

  int xT = 15;

  //  problem for single digital number like 9.4
  int AddX = 0;
  if (TempC < 10) AddX = 27;  //  need to move single digit >>

  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextFont(8);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Minimum Temp", xT, yT, 2);

  yT = yT + 20;
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
  tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Readout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void DisplayMaxTemp(int ListNumb)  //  ********************************************
{
  //  we have to test 00 UTC and 03 UTC for the highest Max Temperature..
  double TempC = myObject["list"][ListNumb]["main"]["temp_max"];       //  00 UTC or 10am Brisbane
  double Temp2C = myObject["list"][ListNumb + 1]["main"]["temp_max"];  //  03 UTC or 1pm

  if (Temp2C > TempC)
    {
      TempC = Temp2C;
    }

  int DecTemp = (TempC - (int)TempC) * 10.00;  //  Use with Deg plus Decimal Deg

  //Serial.print("Maximum Temperature: ");
  //Serial.println(TempC);
  //Serial.println("*********************");

  //  clear Deg area
  tft.fillRect(0, 170, 107, 200, TFT_BLACK);

  int xT = 15;

  //  problem for single digital number like 9.4
  int AddX = 0;
  if (TempC < 10) AddX = 27;  //  need to move single digit >>

  int yT = yTime1Slot;  //  160;  to align all text in bottom area

  tft.setTextFont(8);
  tft.setTextSize(1);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString("Maximum Temp", xT, yT, 2);

  yT = yT + 20;
  tft.setTextFont(4);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
  tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Readout
  tft.print(String("`c"));

  // Displays Decimal degrees
  tft.setCursor(xT + 58, yT + 19);
  tft.print(String("."));
  tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

  tft.setTextFont(8);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  xT = xT + 10;
  yT = yT + 40;
  tft.drawString(LocalCity, xT, yT, 2);
}

void EachDayWeather()  //  Tap on Day Name in Top Left Corner i.e. Tuesday  *************************
{
  //Serial.println("void EachDayWeather()");

  GetForcast = true;
  //Serial.println(" >>> > GetForcast = true  -- Line 2198");
  UpDateReason = " Forecasts ";

  UpDateGraphics(UpDateReason);

  if (TimeOut < 25)  //  Got Internet
    {
      //  Show Outside Temperature, 10am and 4pm weather forecast for next 4 days
      // String TestTime;  //  global var of Day & Time i.e. 2022-09-07 12:00:00

      int NextwDay = wDay;  // Start with Today --  Day of Week i.e. Monday ==
      // int DayCheck = 0;

      if (Hour > 12)  //  i.e. 13 or after 1pm - Start : Reset to Tomorrow
        {
          NextwDay = wDay + 1;

          if (NextwDay > 6) NextwDay = 0;  //  where 0 == Sunday, 1 == Monday etc

          //DayCheck = 4;  //  need to add at least 4 to get next day's forecast
        }

      //Serial.print("Today's Day == ");
      //Serial.println(daysOfTheWeek[wDay]);    //  Example  Saturday
      //Serial.println("***********************");

      for (i = 0; i < 39; i++)  //0 + DayCheck  go thru list[]  - Start at i = 1 not 0 to prevert 12:00 being at i = 0
        {
          TestTime = (const char*)myObject["list"][i]["dt_txt"];  //  Added (const char*) as a new liburary must have changed the process

          //Serial.print("Test TimeSlot :");
          //Serial.println(TestTime.substring(11, 13));  // 12 == 12pm or noon

          //  Time == 10am Brisbane == 00:00 UTC same day..
          if (TestTime.substring(11, 13) == "00")  // 00 UTC == 12am or Midnight UTC or 10am Brisbane
            {
              //Serial.print("TestTime == ");  //  was 12
              //Serial.println(TestTime);              //  Example  2022-09-11 12:00:00

              //Serial.print("Next Day == ");
              //Serial.println(daysOfTheWeek[NextwDay]);  //  Can start today if before 12 noon

              //  Show Each Day in Top Section*********
              //  New Day and Date for Forecasting
              tft.setTextSize(1);
              tft.setTextFont(4);
              tft.setTextColor(TFT_ORANGE, TFT_BLACK);  //  was GREEN YELLOW
              tft.setCursor(12, DayTxtY);
              tft.fillRect(0, 0, 150, 39, TFT_BLACK);
              tft.print(daysOfTheWeek[NextwDay]);

              //  Forecasts Icons Text Names
              //  Get Forecast Icon 10am Slot 1
              TimeSlot1st = i;  //  i == 00 UTC or 10am
              ForeCast1stSlot = myObject["list"][TimeSlot1st]["weather"][0]["description"];
              ForeCast1stSlotStr = String(ForeCast1stSlot);

              //  Get Forecast Icon 4pm or 1600 Slot 2  10am +2 > 4pm
              TimeSlot2nd = i + 2;
              ForeCast2ndSlot = myObject["list"][TimeSlot2nd]["weather"][0]["description"];
              ForeCast2ndSlotStr = String(ForeCast2ndSlot);

              ShowForecast(ForeCast1stSlotStr, ForeCast2ndSlotStr);  //  Shows Weather Icon

              tft.setTextSize(1);
              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, TFT_BLACK);

              String CityForecast = "9am " + ForeCast1stSlotStr;  //   String(City) +
              tft.drawCentreString(CityForecast, xTime1Slot, yTime1Slot, 2);

              CityForecast = "3pm " + ForeCast2ndSlotStr;  //  String(City) +
              tft.drawCentreString(CityForecast, xTime2Slot, yTime2Slot, 2);

              NextwDay++;
              if (NextwDay > 6) NextwDay = 0;

              //  Show Forcast Min Temp at 00 UTC - 00 -2 (7am) and -3 (4am)
              if (i - 2 > -1)  // looking for morning but previous day UTC
                {
                  DisplayMinTemp(i);

                  NonClockDelay(2500);
                  //delay(2500);
                }
              //  Show Max Outside Temperature for each day - Recorded at 00 or 03 UTC,
              DisplayMaxTemp(i);  //  00 UTC == 10am Brisbane

              NonClockDelay(2500);
              //delay(2500);
            }
        }
      //Serial.println("*Should have 4 more Days Worth*");
      //Serial.println("*****************************************");
      // tft.fillRect(0, 0, 150, 39, TFT_BLACK);  //  clear Days Section
      // tft.fillRect(0, 157, 107, 240, TFT_BLACK);  //  clear Outside Temp Section

      SensorDelay = 0;  //  avoid delay to Refresh Temperature Display
      LastSensorDelay = millis();

      //  clear Top Day, Date Area
      tft.fillRect(0, 0, 320, 40, TFT_BLACK);

      //Set ChangeDisplay = true; to update time instantly
      ChangeDisplay = true;

      ShowTime();
      DailyUpdate();
      TemperatureSensor();

      //  Refresh Today's 10am or 4pm Forecasts after doing the 4 day forecast
      //  DailyUpdate();
      /*
      GetForcast = true;
      UpDateReason = "Doing Weather Forcast Update from Display EachDayWeather";
      UpDateGraphics("Forecast Today");
      GoWeather();*/
    }
}

void ConvertUnix2RealTime(long timestamp)
{
  struct tm timeinfo;
  time_t tstamp = (time_t)timestamp;  // Cast timestamp to time_t
  gmtime_r(&tstamp, &timeinfo);
  timeinfo.tm_isdst = 0;          // Set daylight saving time to zero
  timestamp = mktime(&timeinfo);  // Convert back to timestamp
  char OutputTimeStr[20];
  strftime(OutputTimeStr, sizeof(OutputTimeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

  // Print local Output time
  //Serial.print("Output Local Time: ");
  //Serial.println(OutputTimeStr);

  String RealOutputTimeStr = OutputTimeStr;

  // Print local Output time converted to a String
  //Serial.print("Real Output Local Time: ");
  //Serial.println(RealOutputTimeStr);

  OutputHour = RealOutputTimeStr.substring(11, 13);  // == "05"  (If was 05:30
  OutputMin = RealOutputTimeStr.substring(14, 16);   // == "30"

  // Print local Output time : Hour and Minute
  //Serial.print("Output Local Time Hour: ");
  //Serial.println(OutputHour);
  //Serial.print("Output Local Time Minute: ");
  //Serial.println(OutputMin);
}

void DailyUpdate()  //  **********************************************************
{
  //Serial.println("void DailyUpdate()");

  if (!DailyUpdateCall)  //  Prevent clearing Sunrise and Sunset icons and times
    {

      if (!BigDayDisplay)
        {
          //  Get SunRise  -  Added March 2025
          // Extract timezone offset from JSON
          TimezoneOffset = myObject["city"]["timezone"];

          // Extract SunRise time from JSON
          GetSunRiseUnix = myObject["city"]["sunrise"];

          // Convert SunRise time to local time
          SunRiseLocal = GetSunRiseUnix + TimezoneOffset;

          //Serial.print("SunRise Local Time Unix: ");
          //Serial.println(SunRiseLocal);

          ConvertUnix2RealTime(SunRiseLocal);  //  Convert Unix Time to Real Time

          SunRiseHour = OutputHour;
          SunRiseMin = OutputMin;

          //Serial.print("SunRise Local Time Hour: ");
          //Serial.println(SunRiseHour);
          //Serial.print("SunRise Local Time Minute: ");
          //Serial.println(SunRiseMin);

          // Extract SunSet time from JSON
          GetSunSetUnix = myObject["city"]["sunset"];

          // Convert SunSet time to local time
          SunSetLocal = GetSunSetUnix + TimezoneOffset;

          //Serial.print("SunSet Local Time Unix: ");
          //Serial.println(SunSetLocal);

          ConvertUnix2RealTime(SunSetLocal);  //  Convert Unix Time to Real Time

          SunSetHour = OutputHour;
          SunSetMin = OutputMin;

          //Serial.print("SunSet Local Time Hour: ");
          //Serial.println(SunSetHour);
          //Serial.print("SunSet Local Time Minute: ");
          //Serial.println(SunSetMin);


          //  Do Forecast for 10am and 4pm
          if (Hour > 17 || Hour < 10)  //  From (18) 6pm to before 10am
            {
              //Serial.println("Doing Weather Update!! 5pm to before 10am");

              //Serial.print("Hour == ");
              //Serial.print(Hour);
              //Serial.println(" >>>>  Doing Case10am  <<<<");

              Case10am();  //  for Next Day
            }

          //  Do Forecast for 4pm & 7pm
          if (Hour > 9 && Hour < 18)  //  From 10am to before 6pm
            {
              //Serial.println("Doing Weather Update!! 10am to 7pm");

              //Serial.print("Hour == ");
              //Serial.print(Hour);
              //Serial.println(" >>>>  Doing Case4pm   <<<<");

              Case4pm();  //  for Next Day
            }
        }
    }
}

void GoWeather()  //  ******************************************************
{
  //Serial.println("void GoWeather()");

  //  Check for Weather Update evey hour at XHr 01min
  if ((!BigDayDisplay && Minute == 01 && !DoneHourlyCheck) || GetForcast)  //  Gaz Added !BigDayDisplay
    {
      //Serial.println("Hourly Update");

      DoneHourlyCheck = true;
      GetForcast = false;

      //  Check Internet Connection first.. Get TimeOut Value, > 25 == No Internet
      TryWiFi();

      if (TimeOut > 25)
        {
          // UpDateGraphics("No Internet - Try Later");
          // delay(2000);

          //Serial.print("Internet Failed at :");
          //Serial.println(TimeCalc);

          //CleanTimeSlot();
          InternetFail = true;
          UpdateMillis = millis();
        }
      else
        {
          GoWeatherMap();

          ShowTime();

          DailyUpdate();

          GetWWWTime();
          CheckTime = true;
        }

      DrawLines();
    }
  else if (Minute != 01)
    {
      DoneHourlyCheck = false;  //  Gets it ready for the next Hour + 1 minute
    }
}

void TemperatureSensor()  //  ******************************************************************
{
  if (millis() - LastSensorDelay > SensorDelay)  //  Avoid millis() overflow
    {
      //Serial.println("void TemperatureSensor()");

      SensorDelay = 60000;  // == 1 minute  Reset Default Delay for Temperature Sensor
      LastSensorDelay = millis();

      sensors_event_t event;
      dht.temperature().getEvent(&event);

      if (Testing)
        {
          event.temperature = 15;
          Serial.println("Temporary Temperature input here..  line 2800");
        }

      if (isnan(event.temperature))
        {
          //Serial.println(">>>>>>>>>>Error reading temperature!");
          SensorDelay = 2500;  //  Reset Default Delay for Temperature Sensor
          LastSensorDelay = millis();
        }
      else
        {
          float TempC = event.temperature;

          //Serial.print("Time: ");
          //Serial.println(TimeCalc);
          //Serial.print("   event.temperature == ");
          //Serial.println(TempC);

          if (abs(LastTemp - TempC) < 10)  //  Sometimes there is an unsual Temp reading
            {
              LastTemp = TempC;

              TempC = TempC + Calibrate;

              //Serial.print("Calibration == ");
              //Serial.println(Calibrate);

              //Serial.print("TempC == ");
              //Serial.println(TempC);

              int DecTemp = abs((TempC - (int)(TempC)) * 10.00);  //  Use with Deg plus Decimal Deg

              //Serial.print("   Calibrate: ");
              //Serial.print(Calibrate);
              //Serial.print("   Decimal part: ");
              //Serial.println(DecTemp);

              tft.fillRect(0, 157, 107, 83, TFT_BLACK);  //  black out Outside Temp when neccessary

              tft.setTextSize(1);
              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, TFT_BLACK);

              int xT = 15;
              int yT = yTime1Slot;  //  160;  to align all text in bottom area
              tft.drawString("Inside Temp", xT, yT, 2);

              /*  int xH = 248;
          int yH = 165;
          tft.drawString("Humidity", xH, yH, 2);  */

              tft.setTextFont(4);
              tft.setTextColor(TFT_ORANGE, TFT_BLACK);

              //  problem for single digital number like 9.4
              int AddX = 0;
              if (TempC < 10) AddX = 27;  //  need to move single digit >>

              //xT = xT;
              yT = yT + 20;
              tft.drawNumber(TempC, xT + AddX, yT, 6);  // drawNumber == int  ::  drawFloat == float number
              tft.setCursor(xT + 55, yT - 4);           //  -4 for Decimal Rereadout
              tft.print(String("`c"));

              // Displays Decimal degrees
              tft.setCursor(xT + 58, yT + 19);
              tft.print(String("."));
              tft.drawNumber(DecTemp, xT + 65, yT + 17, 4);

              tft.setTextFont(8);
              tft.setTextColor(TFT_GOLD, TFT_BLACK);
              xT = xT + 10;
              yT = yT + 40;
              tft.drawString(LocalCity, xT, yT, 2);
            }
          else
            {
              SensorDelay = 2500;  //  Set here for any errors check again in 2.5 sec
              LastSensorDelay = millis();
            }
        }

      /*
        // Get humidity event and print its value.
        dht.humidity().getEvent(&event);
        if(isnan(event.relative_humidity))
        {
          Serial.println(F("Error reading humidity!"));
          SensorDelay =  2500;  //  check again in 2.5 sec
        }
        else
        {
          float Humid = event.relative_humidity;

          tft.setTextColor(TFT_CYAN, TFT_BLACK);

          xH = xH - 10;
          yH = yH + 25;
          tft.drawNumber(Humid, xH, yH, 6);             // Draw humidity
          tft.setCursor(xH + 55, yH);
          tft.print(String(" % "));

          SensorDelay = 60000;  //  check again in 60 sec
        }
    */
    
     //  Clear Sunrise and Sunset Icons and Times
  if (DailyUpdateCall)
    {
      DailyUpdateCall = false;

      DailyUpdate();
    }
    }
}

void DrawLines()  //  ***************************************************
{
  //Serial.println("void DrawLines()");

  //  Top Horiz Line
  tft.drawLine(0, 40, 320, 40, TFT_LIGHTGREY);  //  2 lines thick
  tft.drawLine(0, 41, 320, 41, TFT_LIGHTGREY);

  // Lower Horiz Line
  tft.drawLine(0, 155, 320, 155, TFT_LIGHTGREY);
  tft.drawLine(0, 156, 320, 156, TFT_LIGHTGREY);

  if (!BigDayDisplay)
    {
      //  Left Vert Line
      tft.drawLine(108, 156, 108, 240, TFT_LIGHTGREY);
      tft.drawLine(109, 156, 109, 240, TFT_LIGHTGREY);

      //  Right Vert Line
      tft.drawLine(217, 156, 217, 240, TFT_LIGHTGREY);
      tft.drawLine(218, 156, 218, 240, TFT_LIGHTGREY);
    }
}

void MidNightUpdate()  //  ********************************************
{
  //Serial.println("void MidNightUpdate()");

  TimeSync = true;

  //Serial.print("Time before Update - ");
  //Serial.println(TimeCalc);  //  TimeCalc == __0:00:00am   __9:04:56pm
  MidNightTime = TimeCalc;

  AfterMidnight = true;

  CheckInternet();

  GetWWWTime();
}

void NonClockDelay(int DelayTime)  //  *****************************************
{
  unsigned long StartDelay = millis();

  ShowDayDate = false;

  while (millis() < StartDelay + DelayTime)
    {
      ShowTime();
    }

  ShowDayDate = true;
}

void ShowTime()  //  *************************************************
{
  int Interval = 0;

  //if(targetTime < millis())  //  This method will be affected by millis() overflow
  //        3020           999
  if ((millis() - FirstMillis) > 999)  //  Using this method, eliminates possible millis() overflow after 49+ days
    {
      Interval = millis() - FirstMillis;  // 3020 - 1000 == 2020 or 2.020 seconds or 00000202 - x9999999 == 203

      FirstMillis = millis();  //  using 999 because getting to this line takes about 1 ms..  apparently?

      //Serial.print("FirstMillis == ");
      //Serial.println(FirstMillis);

      //  Rather than adding 1 Second >>> Add the Inverval, keeping the decimal - Second is a Double type Varible
      Second = Second + (float(Interval) / 1000);

      if (Second > 59.9)  //  60 is not greater than 60 & 60 >> 0
        {                 // Check for roll-over
          // Second = 0;                   // Reset seconds to zero

          //  Rather than Resetting Second to 0 >> Substract 60 to keep decimal seconds
          Second = Second - 60;

          Minute++;           //   mm++;   // Advance minute
          if (Minute > 59)    //  was mm
            {                 // Check for roll-over
              Minute = 0;     //  mm = 0;
              Hour++;         //  hh++;     // Advance hour
              if (Hour > 23)  //  was hh
                {             // Check for 24hr roll-over (could roll-over on 13)
                  Hour = 0;   //   hh = 0;  // 0 for 24 hour clock, set to 1 for 12 hour clock
                }
            }
        }
      //Serial.print("Second == ");
      //Serial.println(Second);

      ChangeDisplay = true;
    }

  if (ChangeDisplay)  //  The Time display is only updated every second
    {
      ChangeDisplay = false;
      //  Check for MidNight to sync Internet Time
      if (Hour == 0 && !TimeSync)  // at MidNight update Time with ntpServer
        {
          MidNightUpdate();
          if (BigDayDisplay) DoBigDay = true;
        }

      if (Hour != 0)
        {
          TimeSync = false;  //  set to false for next MidNight
        }

      AmPm = "am";
      TwHour = Hour;  //  24 hour time
      TwMin = Minute;
      TwSec = Second;
      DispHour = TwHour;

      if (TwHour == 0)
        DispHour = 12;

      if (DispHour > 12)
        {
          DispHour = DispHour - 12;
        }

      if (TwHour > 11)
        {
          AmPm = "pm";
        }

      StrDispHour = String(DispHour);
      if (StrDispHour.length() < 2)
        {
          StrDispHour = "__" + StrDispHour;  //  was "0"   "__"
        }

      StrMin = String(Minute);
      if (StrMin.length() < 2)
        {
          StrMin = "0" + StrMin;
        }

      StrSec = String(int(Second));
      if (StrSec.length() < 2)
        {
          StrSec = "0" + StrSec;
        }

      TimeCalc = StrDispHour + ":" + StrMin + ":" + StrSec + AmPm;
      ShortTime = StrDispHour + ":" + StrMin;  // + AmPm;

      if (AfterMidnight)
        {
          //Serial.print("Time After Midnight Update == ");
          //Serial.println(TimeCalc);
          AfterMidNightTime = TimeCalc;
          AfterMidnight = false;
        }

      if (LastHour != Hour)
        {
          tft.fillRect(0, 42, 320, 110, TFT_BLACK);  // Large Time area
          LastHour = Hour;
        }

      DrawLines();

      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

      if (SetLdrDimmer == MinBright)
        {
          tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
          //Serial.print("Changing SetLdrDimmer = ");
          //Serial.println(SetLdrDimmer);
        }

      tft.setTextFont(7);  //  7 is an Old style Digital Font
      tft.setTextSize(2);
      tft.setCursor(2, 50);
      tft.print(ShortTime);  //  TimeCalc

      if (DelayTime)  //  Brightness has been changed - display NumBars for 0.75sec
        {
          if (millis() - BrightnessBarsStart > BrightCntDwn)
            {
              tft.fillRect(280, 43, 40, 111, TFT_BLACK);  //  154 - 43 = 111
              DelayTime = false;
            }
        }

      if (!DelayTime)
        {
          tft.setTextFont(2);
          tft.setTextColor(TFT_CYAN, TFT_BLACK);
          tft.setCursor(285, 50);
          tft.print(AmPm);

          //  Second Display
          tft.setTextFont(2);
          tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
          tft.setCursor(280, 105);
          tft.print(":" + StrSec);
        }

      if (ShowDayDate)
        {
          if (BigDayDisplay && DoBigDay)  // BigDayDisplay true
            {
              String BigDay = daysOfTheWeek[wDay];

              //Serial.print(BigDay);

              tft.fillRect(0, 156, 320, 84, TFT_BLACK);  //  Lower Temp and Forecast area

              if (BigDay == "Monday") drawBitmap(0, 156, Graphic_Monday, 320, 84, TFT_GREEN);
              if (BigDay == "Tuesday") drawBitmap(0, 156, Graphic_Tuesday, 320, 84, TFT_GREEN);
              if (BigDay == "Wednesday") drawBitmap(0, 156, Graphic_Wednesday, 320, 84, TFT_GREEN);
              if (BigDay == "Thursday") drawBitmap(0, 156, Graphic_Thursday, 320, 84, TFT_GREEN);
              if (BigDay == "Friday") drawBitmap(0, 156, Graphic_Friday, 320, 84, TFT_GREEN);
              if (BigDay == "Saturday") drawBitmap(0, 156, Graphic_Saturday, 320, 84, TFT_GREEN);
              if (BigDay == "Sunday") drawBitmap(0, 156, Graphic_Sunday, 320, 84, TFT_GREEN);

              DoBigDay = false;
            }

          if (wDay != LastDay)
            {
              //  clear Top Day, Date Area
              tft.fillRect(0, 0, 320, 38, TFT_BLACK);

              LastDay = wDay;  //  allows refresh of Day/Date field once a day - avoids flicker
            }

          tft.setTextSize(1);
          tft.setTextFont(4);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.setCursor(12, DayTxtY);
          tft.print(daysOfTheWeek[wDay]);

          StrYear = String(Year);

          StrMonth = String(Month);
          StrMonth = monthsOfYear[Month - 1];
          //  if(StrMonth.length() < 2)
          //  {
          //    StrMonth = "0" + StrMonth;
          //  }

          StrDay = String(nDay);

          if (StrDay.length() < 2)
            {
              StrDay = "0" + StrDay;
            }

          tft.setCursor(165, DayTxtY);

          tft.print(StrDay + "  " + StrMonth + "  " + StrYear);

          //tft.drawString(String(LdrDimmer), 140, 89, 2);  //  only used for testing
        }
    }
}

void loop()  //  ******************************************************************
{
  TouchSpot();

  ShowTime();

  //  Show Inside Temp and Daily Forecast for 5 secs when BigDayDisplay is toggled ON
  if (TempWeatherDisp)
    {
      if (FirstTime)
        {
          FirstTime = false;
          TempTimer = millis();
          BigDayDisplay = false;
          DoBigDay = false;
          ShowDayDate = false;
          // Blank Lower Area
          tft.fillRect(0, 156, 320, 84, TFT_BLACK);
          DrawLines();
          DailyUpdate();
          SensorDelay = 0;
          TemperatureSensor();
        }

      if ((millis() - TempTimer > 5000))  //  Using this method, eliminates possible millis() overflow after 49+ days
        {
          TempWeatherDisp = false;
          BigDayDisplay = true;
          DoBigDay = true;
          ShowDayDate = true;
        }
    }

  if (MinBrightSw)  //  MinBright Switch ON (<99) or OFF (=99)
    {
      GoLDR();
    }

  if (InternetFail)  //  Check internet every 1 minute 60 * 1000 = 60,000 until internet connected
    {
      if ((millis() - UpdateMillis) > 60000)  //  Using this method, eliminates possible millis() overflow after 49+ days
        {
          //Serial.print("Checking for Internet on ");
          //Serial.print(ssid);
          //Serial.print(" every 1 Minute at :");
          //Serial.println(TimeCalc);

          /*  tft.setTextSize(1);
        tft.setTextFont(8);
        tft.setTextColor(TFT_PINK, TFT_BLACK);
        tft.drawString("     WiFi??    ", xTime1Slot, yTime1Slot, 2);
      */

          TryWiFi();

          if (!InternetFail)
            {
              //Serial.println("Internet now UP");
              GetForcast = true;

              GoWeather();  //  Update Weather Icons ????
            }
          else
            {
              //Serial.println("Internet still DOWN");
              UpdateMillis = millis();
            }
        }
    }
  else
    {
      GoWeather();
    }

  if (!BigDayDisplay) TemperatureSensor();
}
