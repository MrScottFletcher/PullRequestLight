#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
//#include <Arduino_JSON.h>
#include <TimeLib.h>
#include <Time.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <base64.h>

#include "config.h"

//This project needs the FastLED library - link in the description.
#include "FastLED.h"
#include "PRLightTower.h"

static char *iot_connectionString;
static char* ado_connectionString;
static char* ado_apiaccesstokenString;
static char *ssid;
static char *pass;

static WiFiClientSecure sslClient; // for ESP8266
const char* UserAgent = "PullRequestLight/1.0 (Arduino ESP8266)";

time_t lastTimeCheck;
int lastPrCount;

int pollingCheckIntervalSeconds;

//=================================================================

#pragma region LED variables setup
//###############################################
bool gReverseDirection = false;
//The total number of LEDs being used is 77
#define NUM_LEDS 77

// The data pin for the NeoPixel strip is connected to digital Pin 6 on the Feather
#define LED_PIN 15

#define COLOR_ORDER GRB

#define CHIPSET     WS2811

#define MAIN_LOOP_SPEED_1_to_1000 100 //Higher is faster.  Inserts a ms delay of "1000/MAIN_LOOP_SPEED_1_to_1000"
#define SECONDS_BETWEEN_PATTERN 20

//Initialise the LED array, the LED Hue (ledh) array, and the LED Brightness (ledb) array.
CRGB leds[NUM_LEDS];

typedef void (*SimplePatternList[])();
//FOR gPatterns:
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle};

typedef void (*DisplayPatternFunction)(void);

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t BRIGHTNESS = 10; //MAX 254

#define NUM_LEDS_RINGS 5

#define NUM_LEDS_RING_01 22
#define NUM_LEDS_RING_02 20 //20
#define NUM_LEDS_RING_03 17 //17
#define NUM_LEDS_RING_04 13
#define NUM_LEDS_RING_05 4

int leds_ring_01[NUM_LEDS_RING_01]; //0-21
int leds_ring_02[NUM_LEDS_RING_02]; //22-41 //42
int leds_ring_03[NUM_LEDS_RING_03]; //42-58 //44-60
int leds_ring_04[NUM_LEDS_RING_04]; //59-72 //61-74
int leds_ring_05[NUM_LEDS_RING_05];  //75-77

int *led_rings[NUM_LEDS_RINGS]{ leds_ring_01,leds_ring_02,leds_ring_03,leds_ring_04,leds_ring_05 };
int led_ring_counts[NUM_LEDS_RINGS]{ NUM_LEDS_RING_01, NUM_LEDS_RING_02,NUM_LEDS_RING_03, NUM_LEDS_RING_04, NUM_LEDS_RING_05};


//###############################################

#pragma endregion

//=================================================================

#pragma region Display States and Manager Code

enum DisplayModeEnum {
    Weather_RainStarting,
    Weather_SnowStarting,
    Weather_SevereWeather_ALERT,
    PullRequest_Pending,
};

class DisplayState {
    time_t startTime;
    //zero means continuous until removed
    time_t scheduledEndTime;
    String StateName;
    DisplayModeEnum DisplayMode;
    DisplayPatternFunction Pattern;
};


class DisplayStateManager {
    DisplayState* States[3];

    DisplayState currentDisplayState;
    time_t currentDisplayStateStarted;
    int currentDisplayStateIndex;

public:
    DisplayState getNextDisplayState() {
        return *States[0];
    }

public:
    void removeDisplayState_byMode(DisplayModeEnum mode) {

        //Find the mode in the list

        //if active, move currentDisplayState to the next

        //remove it from our display states list
    }

public:
    void addDisplayState_byMode(DisplayModeEnum mode, int longevityInSeconds) {

        //Find the mode in the list.

        //if found, update its longevity from now - zero is forever

        //If not in the list, add it
    }


};

DisplayStateManager CurrentStates;

#pragma endregion

//=================================================================
#pragma region CONFIRGURE PATTERNS FOR CONDITIONS

//DisplayPatternFunction getDisplayPattern(DisplayModeEnum mode) {
//    switch (mode) {
//    case Weather_RainStarting:
//        return rainbow;
//        break;
//    case Weather_SnowStarting:
//        return rainbowWithGlitter;
//        break;
//    default:
//        return confetti;
//        break;
//    }
//}

#pragma endregion

//=================================================================
//              SETUP AND LOOP
//=================================================================

void setup() {
  delay(3000); // sanity delay
  //Do something simple here to delay start
  pinMode(0, OUTPUT);
  BlinkRed(5);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  
  pollingCheckIntervalSeconds = 60;

  setupLedRingArrays();
  delay(500);
  BlinkRed(2);
  
  fullSetupLightsAndWiFi();

  
  lastTimeCheck = GetTime();
  
  //testParsing();
}

void loop() {

    //each loop will be a couple of minutes.  We don't want to bang on the time server
    //every few millis, so let's only do that every so often.

    static int loopCount = 0;
    
    //enter the demo loop...
    //demoLoop1();
    bool in_activeTime;
    time_t et;

    //?We're going to this every loop, but we might throttle later.
    if (loopCount == 0 || loopCount % 10 == 0) {
        et = GetTime();
        Serial.printf("Fetched NTP epoch time is: %lu.\r\n", et);
    }
    
    tm* t = gmtime(&et);
    
    //This "et" will not be updated until the above if triggers.  This is kind of a hot mess.
    //Byproduct of polk-and-prod experiemntation...
    if (lastTimeCheck + pollingCheckIntervalSeconds < et) {
        lastTimeCheck = et;
        //Illinois is GMT-6, so "0" == 6PM.  "12" is 6AM.
        bool in_activeTime = (t->tm_hour > 12);
        //bool in_activeTime = true;

        if (in_activeTime) {
            in_activeTime = true;
            UpdatePrCounts();
        }
        else
        {
            Serial.printf("Pausing because HOUR is: %d.\r\n", t->tm_hour);
            //Shut all the lights off
            FastLED.setBrightness(0);
            FastLED.show();
            //hold for a bit since we're - like two minutes
            delay(240000);
        }
    }
    else {
        Serial.println("Pausing a bit because Poll time has not triggered");
        delay(2000);
    }

    //****************************************************
    //If we want to handle the FastLED like the samples, we would alter the
    //speed and pallette, and do the Show() call here.  Need to research the 
    //polling intervals differently, though.
    //****************************************************

    loopCount++;
}

//void doLEDLoop() {
//    //This might become the managed loop
//    FastLED.show();
//    FastLED.delay(1000/MAIN_LOOP_SPEED_1_to_1000);
//}

//=================================================================

#pragma region PULL REQUEST POLL AND COUNT CODE

void UpdatePrCounts() {
    int prCount = getPrCount();
    //compare the states, do the things.
    ChangeStates_PrCount(prCount);
}

void ChangeStates_PrCount(int newPrCount) {

    if (newPrCount > 0) {
        for (size_t i = 0; i < 7; i++)
        {
            fadeIn(CRGB::Blue, 100, 75);
            fadeOut(CRGB::Blue, 100, 75);
        }
    }
    //This is the smarter State Change version...
    //static int lastPrCount = 0;
    //if (newPrCount != lastPrCount) {
    //    //changed
    //    if (newPrCount > 0) {
    //        //set to on
    //        CurrentStates.addDisplayState_byMode(PullRequest_Pending, 0);
    //    }
    //    else {
    //        //set to off
    //        CurrentStates.removeDisplayState_byMode(PullRequest_Pending);
    //    }
    //}

}

int getPrCount() {
    const size_t capacity = 9216; //(9K)
    DynamicJsonDocument doc(capacity);
    String Link;
    HTTPClient http;    //Declare object of class HTTPClient
    //"C:\Program Files\Git\usr\bin\openssl.exe" s_client -connect rlicorp.visualstudio.com:443 | "C:\Program Files\Git\usr\bin\openssl.exe" x509 -fingerprint -noout
    //SHA1 Fingerprint=79:DA:31:82:67:4D:25:43:77:18:24:8F:BA:6C:6E:5D:18:55:2E:A3
    const char* fingerprint = "79:DA:31:82:67:4D:25:43:77:18:24:8F:BA:6C:6E:5D:18:55:2E:A3";

    sslClient.setInsecure();
    http.setAuthorization(ado_apiaccesstokenString, ado_apiaccesstokenString);
    http.setUserAgent(UserAgent);
    Link = ado_connectionString;
    http.begin(Link, fingerprint);     //Specify request destination
    int httpCode = http.GET();            //Send the request
    Serial.println(httpCode);   //Print HTTP return code

    if (httpCode != 200) {
        for (size_t i = 0; i < 2; i++)
        {
            fadeIn(CRGB::Crimson, 20, 25);
            fadeOut(CRGB::Crimson, 20, 25);
        }

        return -1;
    }
    else {

#pragma region HTTP client errors
        //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h#L49
        //#define HTTPC_ERROR_CONNECTION_REFUSED  (-1)
        //#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
        //#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
        //#define HTTPC_ERROR_NOT_CONNECTED       (-4)
        //#define HTTPC_ERROR_CONNECTION_LOST     (-5)
        //#define HTTPC_ERROR_NO_STREAM           (-6)
        //#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
        //#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
        //#define HTTPC_ERROR_ENCODING            (-9)
        //#define HTTPC_ERROR_STREAM_WRITE        (-10)
        //#define HTTPC_ERROR_READ_TIMEOUT        (-11)
#pragma endregion

        String payload = http.getString();    //Get the response payload
        http.end();  //Close connection
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            String err = String(error.c_str());
            Serial.println(err);
            for (size_t i = 0; i < 2; i++)
            {
                fadeIn(CRGB::Yellow, 10, 25);
                fadeOut(CRGB::Yellow, 10, 25);
            }

            return -1;
        }

        int count = doc["count"];

        return count;
    }
}

#pragma endregion

//=================================================================

//##########################################################################################################

#pragma region SETUP LIGHTS, RINGS, WIFI, TIME SECTION

void fullSetupLightsAndWiFi() {
    FastLED.setBrightness(150);
    //-------------------------------------
    //SERIAL
    //-------------------------------------
    illuminateRing(0, 0, 0, 100);
    FastLED.show();

    initSerial();

    illuminateRing(0, 0, 100, 0);
    FastLED.show();

    delay(300);
    BlinkRed(1);
    //-------------------------------------
    //CREDENTIALS
    //-------------------------------------
    illuminateRing(1, 0, 0, 100);
    FastLED.show();

    readCredentials();

    illuminateRing(1, 0, 100, 0);
    FastLED.show();
    //-------------------------------------
    //BREATHE a moment before the WiFi setup
    delay(300);
    BlinkRed(1);
    //-------------------------------------
    //WIFI NETWORK   
    //-------------------------------------
    illuminateRing(2, 0, 0, 100);
    FastLED.show();

    bool bConnected = setupWifi();

    //Initialize the Time routine
    illuminateRing(2, 0, 0, 100);
    FastLED.show();

    if (bConnected) {

        //-------------------------------------
        //TIME
        //-------------------------------------
        illuminateRing(3, 0, 0, 100);
        FastLED.show();

        initTime();

        illuminateRing(3, 0, 100, 0);
        FastLED.show();

        delay(1000);
        BlinkRed(1);
        //==============================================
        clearLEDs();
        FastLED.show();

        delay(300);
        BlinkRed(1);
        //-------------------------------------
    }
    else {
        //bail out to some other mode
    }
    //do we really want to continue if not connected?
}

bool setupWifi() {
    //Initialize the WIFI
    illuminateRing(3, 0, 0, 100);
    FastLED.show();

    bool connected = initWifi();
    if (connected)
    {
        illuminateRing(3, 0, 100, 0);
        FastLED.show();
        delay(2000);
    }
    else
    {
        for (size_t i = 0; i < 10; i++)
        {
            if (i % 2 == 0)
                FastLED.showColor(CRGB::DarkRed);
            else
                FastLED.showColor(CRGB::Yellow);

            delay(1000);
        }
    }
    return connected;
}

void setupLedRingArrays() {
    int currentLed = 0;
    for (unsigned int x = 0; x < NUM_LEDS_RINGS; x = x + 1)
    {
        //hit each ring if is modulus of the loop number
        for (unsigned int a = 0; a < led_ring_counts[x]; a = a + 1) {
            if (currentLed < NUM_LEDS) {
                led_rings[x][a] = currentLed;
            }
            currentLed++;
        }
    }
}

bool initWifi()
{
    int maxLoopCheckCount = 3;
    int eachLoopCheckWaitSeconds = 10;


    bool bConnected = false;
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    int loopCounter = 0;
    while (WiFi.status() != WL_CONNECTED && loopCounter < maxLoopCheckCount)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(eachLoopCheckWaitSeconds * 1000);
        loopCounter++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("Connected to wifi %s.\r\n", ssid);
        bConnected = true;
    }
    else {
        Serial.printf("WIFI CONNECTION FAILED!");
    }

    return bConnected;
}


void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}

time_t GetTime() {
    time_t epochTime;
    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            break;
        }
    }
    return epochTime;
}
#pragma endregion

//##########################################################################################################

//i.e. CRGB::White
void fadeIn(CRGB color, uint8_t loopDelayMs, uint8_t maxBrightness) {

    for (uint8_t b = 0; b < 255; b++) {
        FastLED.showColor(color, b * maxBrightness / 255);
        delay(loopDelayMs);
    };
};

//fadeDurationMilliseconds
//1000 / 
void fadeOut(CRGB color, uint8_t loopDelayMs, uint8_t maxBrightness)
{
    for (uint8_t b = 255; b > 0; b--) {
        FastLED.showColor(color, b * maxBrightness / 255);
        delay(loopDelayMs);
    };
}

//##########################################################################################################

void demoLoop1() {

    //60 frames a second
    int framesPerSecond = 20;
    int msDelay = 1000 / framesPerSecond;
    int desiredDurationSeconds = 60;
    int loops = 1000/msDelay * desiredDurationSeconds;

    //FireRings(loops, msDelay);

    for (int i = 401; i > 0; i = i - 50)
    {
        lightRingStackColors(1, i);
    }
    //fast
    lightRingStackColors(50, 100);

    for (int i = 10; i > 0; i=i-1)
    {
        singleDotCrawl(1, i);
    }
    singleDotCrawl(100, 1);

    for (int i = 20; i > 0; i = i - 1)
    {
        lightMultiColumnCrawl(1, i);
    }
    //fast
    lightMultiColumnCrawl(25, 1);

    for (int i = 801; i > 0; i=i-50)
    {
        lightRingCrawl(1, i);
    }
    //fast
    lightRingCrawl(100, 100);


    for (int i = 20; i > 0; i= i-2)
    {
        lightColumnCrawl(1, i);
    }
    lightColumnCrawl(25, 1);
}

//#####################################################################################

#pragma region LED Program Code

//############################################################################################

void BlinkErrorStack(int changes, int delayMs) {
    for (size_t i = 0; i < changes; i++)
    {
        if (i % 2 == 0)
            FastLED.showColor(CRGB::Chartreuse);
        if (i % 3 == 0)
            FastLED.showColor(CRGB::DarkRed);
        else
            FastLED.showColor(CRGB::Honeydew);

        delay(delayMs);
    }
}


void singleDotCrawl(int loops, int delayms) 
{
    if (delayms < 1) delayms = 1;
    FastLED.setBrightness(150);
    for (size_t z = 0; z < loops; z++)
    {
        for (int x = 0; x < NUM_LEDS; x++)
        {
            clearLEDs();

            leds[x].setRGB(100, 100, 100);

            FastLED.show();
            delay(delayms);
        }
    }
}


void lightRingCrawl(int loops, int delayms) {
    if (delayms < 5) delayms = 5;
    for (size_t z = 0; z < loops; z++)
    {
        for (int x = 0; x < NUM_LEDS_RINGS; x = x + 1)
        {
            //all black
            clearLEDs();
            //hit each ring if is modulus of the loop number
            illuminateRing(x, 100, 100, 100);
            FastLED.show();
            delay(delayms);
        }
    }
}

void lightRingStackColors(int loops, int delayms) {
    FastLED.setBrightness(150);
    if (delayms < 5) delayms = 5;
    for (size_t z = 0; z < loops; z++)
    {
        int brightness = 200;
        int r = 0;
        int g = 0; 
        int b = 0;

        //all black
        clearLEDs();
        for (int x = 0; x < NUM_LEDS_RINGS; x = x + 1)
        {
            if (x == 0) {
                r = brightness;
                g = 0;
                b = 0;
            }
            if (x == 1) {
                r = 0;
                g = brightness;
                b = 0;
            }
            else if (x == 2) {
                r = 0;
                g = 0;
                b = brightness;
            }
            else if (x == 3) {
                r = 0;
                g = brightness / 2;
                b = brightness / 2;
            }
            else if (x == 4) {
                r = brightness / 2;
                g = brightness / 2;
                b = 0;
            }
            else if (x == 5) {
                r = brightness / 2;
                g = 0;
                b = brightness / 2;
            }
            illuminateRing(x, r, g, b);
            FastLED.show();
            delay(delayms);
        }
    }
}

void illuminateRing(int ringIndex, int r, int g, int b) {
    if (ringIndex < 0)
        ringIndex = 0;

    for (int a = 0; a < led_ring_counts[ringIndex]; a = a + 1) {
        leds[led_rings[ringIndex][a]].setRGB(r,g,b);
    }
}

void lightColumnCrawl(int loops, int delayms) {
    if (delayms < 1) delayms = 1;
    
    for (size_t z = 0; z < loops; z++)
    {
        for (double d = 0; d < 365; d = d + 3)
        {
            clearLEDs();
            //Don't hit the top ring.  It is not a full ring.
            IlluminateAllRingsAtDegree(d);
            FastLED.show();
            delay(delayms);
        }
    }
}

void lightMultiColumnCrawl(int loops, int delayms) {
    if (delayms < 1) delayms = 1;

    for (size_t z = 0; z < loops; z++)
    {
        for (double d = 0; d < 365; d = d + 3)
        {
            clearLEDs();
            //Don't hit the top ring.  It is not a full ring.
            IlluminateAllRingsAtDegree(d);
            IlluminateAllRingsAtDegree(d + 90);
            IlluminateAllRingsAtDegree(d + 180);
            IlluminateAllRingsAtDegree(d + 270);
            FastLED.show();
            delay(delayms);
        }
    }
}


void IlluminateAllRingsAtDegree(double d)
{
    static double threeSixty = 360;
    for (int x = 0; x < NUM_LEDS_RINGS - 1; x = x + 1)
    {
        double thisRingCount = led_ring_counts[x];
        double degreeConversionfactor = thisRingCount / threeSixty;
        //hit LED in the ring that matches the degress
        int indexOfLedToLight = degreeConversionfactor * ((int)d % 360);
        if (led_ring_counts[x] > indexOfLedToLight)
        {
            int stripIndex = led_rings[x][indexOfLedToLight];
            leds[stripIndex].setRGB(100, 100, 100);
        }
    }
}


void clearLEDs() {
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i].setRGB(0, 0, 0);
    }
}
void demoLoop() {
    while (1) {
        // Add entropy to random number generator; we use a lot of it.
        random16_add_entropy(random(10));

        gPatterns[gCurrentPatternNumber]();
        //Fire2012(); // run simulation frame

        FastLED.show(); // display this frame
        FastLED.delay(1000 / MAIN_LOOP_SPEED_1_to_1000);

        // for gPatterns, do some periodic updates
        EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
        EVERY_N_SECONDS(SECONDS_BETWEEN_PATTERN) { nextPattern(); } // change patterns periodically
    }
}

//=======================================================================================

void BlinkRed(int count){
  for(int i = 0; i < count; i++){
      digitalWrite(0, HIGH);
      delay(500);
      digitalWrite(0, LOW);
      delay(500);
  }
}

//=======================================================================================
// For gPatterns, list of patterns to cycle through.  Each is defined as a separate function below.
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

//#####################################################################################
// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120


void Fire2012()
{
    //NOTE - requires caller to do FastLED.Show() and Delay();
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}


void FireRings(int loops, int delayms)
{
    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS_RINGS];
    if (delayms < 1) delayms = 1;
    for (size_t z = 0; z < loops; z++)
    {
        // Step 1.  Cool down every cell a little
        for (int i = 0; i < NUM_LEDS_RINGS; i++) {
            heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS_RINGS) + 2));
        }

        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for (int k = NUM_LEDS_RINGS - 1; k >= 2; k--) {
            heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
        }

        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        if (random8() < SPARKING) {
            int y = random8(7);
            heat[y] = qadd8(heat[y], random8(160, 255));
        }

        int currentLedStripIndex = 0;
        // Step 4.  Map from heat cells to LED colors
        for (int j = 0; j < NUM_LEDS_RINGS; j++)
        {
            CRGB color1 = HeatColor(heat[j]);
            double thisRingCount = led_ring_counts[j];
            //hit LED in the ring that matches the degress
            for (int i = 0; i < thisRingCount; i++)
            {
                leds[currentLedStripIndex] = color1;
                currentLedStripIndex++;
            }
        }
        FastLED.show();
        delay(delayms);
    }
}

#pragma endregion

//#####################################################################################

#pragma region TEST METHOD FOR TRYING STUFF

void testParsing() {

    FastLED.setBrightness(200);
    //-------------------------------------
    //SERIAL
    //-------------------------------------
    illuminateRing(0, 0, 100, 100);
    FastLED.show();
    delay(300);
    BlinkRed(1);

    //const size_t capacity = 4790; //from a sample
    const size_t capacity = 9216; //(9K)
    //const size_t capacity = 102400; //does not work

    Serial.printf("Allocating doc.\r\n");
    DynamicJsonDocument doc(capacity);
    String Link;
    HTTPClient http;    //Declare object of class HTTPClient

    //"C:\Program Files\Git\usr\bin\openssl.exe" s_client -connect rlicorp.visualstudio.com:443 | "C:\Program Files\Git\usr\bin\openssl.exe" x509 -fingerprint -noout
    //SHA1 Fingerprint=79:DA:31:82:67:4D:25:43:77:18:24:8F:BA:6C:6E:5D:18:55:2E:A3
    const char* fingerprint = "79:DA:31:82:67:4D:25:43:77:18:24:8F:BA:6C:6E:5D:18:55:2E:A3";

    Serial.printf("Encoding API Key...\r\n");
    sslClient.setInsecure();

    Serial.printf("Adding headers...\r\n");
    http.setAuthorization(ado_apiaccesstokenString, ado_apiaccesstokenString);

    http.setUserAgent(UserAgent);


    //GET Data
    //The link to Marine
    //Link = "https://rlicorp.visualstudio.com/DefaultCollection/Marine/_apis/git/pullrequests?api-version=5.0";
    //Tests ALL Pull Requests in org.
    //Link = "https://rlicorp.visualstudio.com/DefaultCollection/_apis/git/pullrequests?api-version=5.0";
    Link = ado_connectionString;

    Serial.printf("HTTP Begin...\r\n");
    http.begin(Link, fingerprint);     //Specify request destination

    Serial.printf("HTTP GET...\r\n");
    int httpCode = http.GET();            //Send the request
    Serial.printf("END GET...\r\n");
    Serial.println(httpCode);   //Print HTTP return code

    //// HTTP client errors
    //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h#L49
    //#define HTTPC_ERROR_CONNECTION_REFUSED  (-1)
    //#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
    //#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
    //#define HTTPC_ERROR_NOT_CONNECTED       (-4)
    //#define HTTPC_ERROR_CONNECTION_LOST     (-5)
    //#define HTTPC_ERROR_NO_STREAM           (-6)
    //#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
    //#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
    //#define HTTPC_ERROR_ENCODING            (-9)
    //#define HTTPC_ERROR_STREAM_WRITE        (-10)
    //#define HTTPC_ERROR_READ_TIMEOUT        (-11)

    illuminateRing(0, 0, 0, 0);
    illuminateRing(1, 0, 0, 0);
    illuminateRing(2, 0, 0, 0);
    illuminateRing(3, 0, 0, 0);

    //if (httpCode > 0) { //Check the returning code
    //    if (httpCode == 200) {
    //        //WAS 200!!   Yeah!! - SLOW GREEN
    //        for (size_t i = 0; i < 5; i++)
    //        {
    //            if (i % 2 == 0) {
    //                //Green
    //                illuminateRing(4, 0, 250, 0);
    //            }
    //            else {
    //                illuminateRing(4, 0, 0, 0);
    //            }
    //            FastLED.show();
    //            delay(600);
    //        }
    //    }
    //    else {
    //        //not 200 - FAST YELLOW
    //        for (size_t i = 0; i < 100; i++)
    //        {
    //            if (i % 2 == 0) {
    //                //Yellow
    //                illuminateRing(4, 250, 250, 0);
    //            }
    //            else {
    //                illuminateRing(4, 0, 0, 0);
    //            }
    //            FastLED.show();
    //            delay(100);
    //        }
    //    }
    //}
    //else {
    //    for (size_t i = 0; i < 100; i++)
    //    {
    //        //MEDIUM RED
    //        if (i % 2 == 0) {
    //            illuminateRing(4, 250, 0, 0);
    //        }
    //        else {
    //            illuminateRing(4, 0, 0, 0);
    //        }
    //    FastLED.show();
    //    delay(300);
    //    }
    //}
    //delay(10000);

    Serial.printf("HTTP getString()...\r\n");
    String payload = http.getString();    //Get the response payload
    Serial.println(payload);    //Print request response payload

    Serial.printf("HTTP end()...\r\n");
    http.end();  //Close connection

    //===================================================================

    Serial.printf("Deserializing payload now...\r\n");
    DeserializationError error = deserializeJson(doc, payload);

    Serial.printf("Deserializing COMPLETE...\r\n");

    Serial.printf("Checking error object...\r\n");
    if (error)
    {
        Serial.printf("ERROR parsing json.\r\n");

        String err = String(error.c_str());
        //could be IncompleteInput - payload too long
        Serial.println(err);
        for (size_t i = 0; i < 10; i++)
        {
            if (i % 2 == 0)
                FastLED.showColor(CRGB::Chartreuse);
            if (i % 3 == 0)
                FastLED.showColor(CRGB::DarkRed);
            else
                FastLED.showColor(CRGB::Honeydew);

            delay(1000);
        }
        return;
    }

    Serial.printf("Past the parsing checks...\r\n");
    FastLED.showColor(CRGB::Green);
    FastLED.show();
    delay(5000);

    clearLEDs();
    FastLED.show();

    illuminateRing(4, 0, 0, 250);
    FastLED.show();
    delay(1000);

    Serial.printf("Getting Count...\r\n");
    int count = doc["count"];
    Serial.printf("trying to show the count...\r\n");
    Serial.printf("Found PRs in json: %d ", count);
    Serial.printf("---------------\r\n");
    Serial.printf("\r\n");

    if (count > 0) {
        Serial.printf("Setting glitter...\r\n");
        rainbowWithGlitter();
        FastLED.show();
        delay(5000);
    }
    else {
        illuminateRing(1, 250, 250, 0);
        illuminateRing(2, 250, 250, 0);
        illuminateRing(3, 250, 250, 0);
        illuminateRing(4, 250, 250, 0);
        FastLED.show();
    }


    illuminateRing(4, 0, 250, 0);
    FastLED.show();
    delay(1000);

    illuminateRing(0, 0, 200, 0);
    FastLED.show();
    delay(300);
    BlinkRed(1);

    Serial.printf("End of test.\r\n");
}
#pragma endregion

//#####################################################################################
