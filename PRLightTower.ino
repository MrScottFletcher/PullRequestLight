#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <Arduino_JSON.h>
#include <TimeLib.h>
#include <Time.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include "config.h"

//This project needs the FastLED library - link in the description.
#include "FastLED.h"
#include "PRLightTower.h"

//The total number of LEDs being used is 77
#define NUM_LEDS 77

// The data pin for the NeoPixel strip is connected to digital Pin 6 on the Feather
#define LED_PIN 15

#define COLOR_ORDER GRB

#define CHIPSET     WS2811

#define BRIGHTNESS  10 //MAX 254
#define FRAMES_PER_SECOND 100
#define SECONDS_BETWEEN_PATTERN 20

static char *iot_connectionString;
static char* ado_connectionString;
static char* ado_apiaccesstokenString;
static char *ssid;
static char *pass;

#pragma region LED variables setup
//###############################################
bool gReverseDirection = false;

//Initialise the LED array, the LED Hue (ledh) array, and the LED Brightness (ledb) array.
CRGB leds[NUM_LEDS];

typedef void (*SimplePatternList[])();
//FOR gPatterns:
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle};


uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

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

void setup() {
  delay(3000); // sanity delay
  //Do something simple here to delay start
  pinMode(0, OUTPUT);
  BlinkRed(5);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  
  setupLedRingArrays();
  BlinkRed(2);

  readCredentials();
 
  setupWifi();
}

void setupWifi() {

    //glow first ring
    //test the wifi 
    //If OK, show Green, otherwise, show red
    
    //test the connection to ADO
    //If OK, show Green, otherwise, show red

    //If all OK, flassh all Green twice, and return to enter the status loop as usual;
    //If all NOT ok, stay here and slow pulse red on top only
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


void loop() {

    //enter the demo loop...
    demoLoop1();

}

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
        FastLED.delay(1000 / FRAMES_PER_SECOND);

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
