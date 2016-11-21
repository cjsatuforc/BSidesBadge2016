/* JSON Parser */
#include <ArduinoJson.h>

/* ESP8266 WiFi */
#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

/* EEPROM */
#include <EEPROM.h>
#include "EEPROMAnything.h"

/* LCD */
#include "SSD1306.h" // Screen Library
#include "OLEDDisplayUi.h" // Include the UI lib
#include "images.h" // Include custom images
SSD1306  display(0x3c, 5, 2);
OLEDDisplayUi ui     ( &display );

/* Timer */
#include "Timer.h"
Timer t;                               //instantiate the timer object

/*Infrared */
#include <IRremoteESP8266.h>
const int TX_PIN = 10;
IRsend irsend(TX_PIN); //an IR led is connected to GPIO pin 0

int RECV_PIN = 4; 
IRrecv irrecv(RECV_PIN);
decode_results results;



/* Button Constants */
const int P1_Left = 0;
const int P1_Right = 2;
const int P1_Top = 1;
const int P1_Bottom = 3;

const int P2_Left = 6;
const int P2_Right = 4;
const int P2_Top = 7;
const int P2_Bottom = 5;



/* Shift Out ( 74HC595 ) */
const int latchPin = 12; //Pin connected to latch pin (ST_CP) of 74HC595
const int clockPin = 14; //Pin connected to clock pin (SH_CP) of 74HC595
const int dataPin = 13; //Pin connected to Data in (DS) of 74HC595

/* Shift In  */
const int pinShcp = 15; //Clock
const int pinStcp = 0; //Latch
const int pinDataIn = 16; // Data

/* List of last 5 seen badges and the amount we have seen since last update */
unsigned int badgeList[5] = {1234567890,1234567891,1234567892,1234567893,1234567894};
int numBadges = 0;

/* Our HTTP client */
HTTPClient http;

/* Default WiFi SSID details */
char defaultSSID[32]     = "EmpireRecords_Devices";
char defaultPassword[32] = "loldevices";
//char defaultSSID[32]     = "AndrewMohawkGlobal";
//char defaultPassword[32] = "0126672737";

/* WiFi struct for EEPROM */
struct WiFiSettings {
  char ssid[32];
  char password[32];
} currentWiFi;


/* Endpoints for badges */
String hashEndPoint = "http://badges2016.andrewmohawk.com:8000/badge/gethash/";
String checkInEndPoint = "http://badges2016.andrewmohawk.com:8000/badge/checkin/";
//String hashEndPoint = "http://10.85.0.241:8000/badge/gethash/";
//String checkInEndPoint = "http://10.85.0.241:8000/badge/checkin/";

/* Badge Name and Number */
String badgeName = "";
unsigned int badgeNumber;

/* UI functions for debouncing, last keypressed for 'sleep', lowPowerMode and fake PWM */

unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 200;  

unsigned long lastAction = 0;
unsigned long lastActionTimeout = 30000;

bool lowPowerMode = false;
byte currentShiftOut = 0;

/* Not current used */
/*
unsigned long lastPWMTime = 0;  
unsigned long PWMDelay = 10;   
*/

unsigned long currTime = 0;


/* UI Functional variables */
String team = "None!";
String alias = "No Alias";
String badgeVerifyCode = "NoCode";
int level = 1;





//Speaker information


#include <pgmspace.h>

PROGMEM const char string_intro[]  = "          BSIDES Schedule      Use right pad to navigate.";  
PROGMEM const char string_0[]      = "          08h00-09h15:         Coffee and Registration";   
PROGMEM const char string_1[]      = "          09h15-09h30:       Grant Ongers - Welcome to BSides";
PROGMEM const char string_2[]      = "          09h30-09h45:       Andrew MacPherson & Mike Davis - The BSides Badge";
PROGMEM const char string_3[]      = "          10h00-10h30:        Neil Roebert Mi->NFC->TM: How to proxy NFC comms using Android";
PROGMEM const char string_4[]      = "          10h45-11h00:       Coffee Break";
PROGMEM const char string_5[]      = "          11h15-11h45:       Chris Le Roy: What the DLL?";
PROGMEM const char string_6[]      = "          12h00-12h45:       Lunch";
PROGMEM const char string_7[]      = "          13h00-13h30:       Ion Todd: Password Securit for humans";
PROGMEM const char string_8[]      = "          13h45-15h15:       Robert Len: (In)Outsider Trading - Hacking stocks using public information";
PROGMEM const char string_9[]      = "          14h30-14h45:       Coffee Break";
PROGMEM const char string_10[]     = "          15h00-15h30:       Charl van der Walt: Love triangles in cyberspace. A tale about trust in 5 chapters";
PROGMEM const char string_11[]     = "          15h45-16h00:       Thomas Underhay & Darryn Cull: SensePost XRDP Tool";
PROGMEM const char string_12[]     = "          16h15-16h30:       BSides CPT 2016 Challenge";
PROGMEM const char string_13[]     = "          16h45-17h00:       Closing";
const char * const BSidesSchedule [] PROGMEM = {string_intro,string_0,string_1,string_2,string_3,string_4,string_5,string_6,string_7,string_8,string_9,string_10,string_11,string_12,string_13};

//number of speakers
int numScheduleItems = 13;
int currentScheduleItem = 0;
char currentSpeaker[120] = {0};

/* Helpers */
#include "ShiftRegisters.h" // input/output registers
#include "WiFi.h" // WiFi connections
#include "communication.h" // Communications To/From server
#include "screen.h" // Screen drawing functions


// This array keeps function pointers to all frames
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4 };

// how many frames are there?
int frameCount = 4;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;





/*
 * Main Setup for badge.
 */
void setup() {
  Serial.begin(74880);
  
  strcpy_P(currentSpeaker, (char*) pgm_read_dword(&(BSidesSchedule[currentScheduleItem])));
  
/* shift out */
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  darkness();

/* shift in */
pinMode(pinStcp, OUTPUT);
 pinMode(pinShcp, OUTPUT);
 pinMode(pinDataIn, INPUT);
  
  
   
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);


initWiFi(true); // Initialise WiFi
  drawProgressBar(0, 10, "Display Init..",50);
  //Setup Serial Connection at ESP debug speed ( so we get debug information as well )
 
  
  //Setup EEPROM - 512 bytes!
  EEPROM.begin(512);

  drawProgressBar(10, 20, "EEPROM ..",50);

  
  drawProgressBar(20, 30, "Serial ..",50);
  Serial.setDebugOutput(true);

  
  drawProgressBar(30, 50, "Connecting to Wifi..",100);
  

  darkness();
  twirl();
  
  drawProgressBar(50, 60, "Joining network..",100);
  fetchStatus(); // fetch network status
  int fetchStatusEvent = t.every(15000, fetchStatus); // Set it to run every 15 seconds after this
  

  drawProgressBar(60, 70, "Configuring AI/IR",75);
  irrecv.enableIRIn(); // Start the receiver
  irsend.begin();
  int tickEvent2 = t.every(5550, transmitBadge);
  
  drawProgressBar(70, 80, "Hacking Planet..",75);
  
  
  drawProgressBar(80, 90, "How about a nice game",75);
  drawProgressBar(90, 100, "...of chess...",50);




   // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
  //ui.setActiveSymbol(activeSymbol);
  //ui.setInactiveSymbol(inactiveSymbol);
 ui.disableAllIndicators();
  
  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  //We dont want autoplay
  ui.disableAutoTransition();

  for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
    display.drawLine(0, 0, i, DISPLAY_HEIGHT-1);
    display.display();
    delay(10);
  }
  for (int16_t i=0; i<DISPLAY_HEIGHT; i+=4) {
    display.drawLine(0, 0, DISPLAY_WIDTH-1, i);
    display.display();
    delay(10);
  }
  // Initialising the UI will init the display too.
  ui.init();
  display.flipScreenVertically();

  lastAction = millis();

}











void loop() {
  int remainingTimeBudget = 0;
  if(lowPowerMode == false)
  {
    remainingTimeBudget = ui.update();
  }
  else
  {
    t.update();
  }


  if (remainingTimeBudget > 0 || lowPowerMode == true) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.

    currTime = millis();
    
    if ((currTime - lastDebounceTime) > debounceDelay) {
      readShift();
      lastDebounceTime = currTime;
    }
    
    currTime = millis(); // Dont ask why we need to do this again............ I mean wtf!
    
    if(currTime - lastAction > lastActionTimeout)
    {
      if(lowPowerMode == false)
      {
      display.clear();
      display.drawXbm(0, 16, sleepingpanda_width, sleepingpanda_height, sleepingpanda_bits);
      display.display();
      lowPowerMode = true;
      setOutShift(0);
      
      }
      

    }
    /*
      
     if ((millis() - lastPWMTime) > PWMDelay) 
    {
      if(lowPowerMode == false)
      {
        setOutShift(currentShiftOut);
      }
      lastPWMTime = millis();
    }
    else
    {
      darkness();
    }
    */
    
    // put your main code here, to run repeatedly:
    t.update();

    if (irrecv.decode(&results)) 
    {
      dump(&results);
      irrecv.resume(); // Receive the next value
    }
   
  }
  
 
  
}
