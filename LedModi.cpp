#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>                        // LIB FOR LEDS
#include "LedModi.h"
#include "dist.h"
#include "StringSplit.h"
#include <bits/stdc++.h>

#define NUM_LEDS 36
#define FASTLED_DATA_PIN 3

void e_static();
void e_strobe();
void e_rainbow_cycle();
void e_off();
void e_random_strobe();
void e_light_walk();
void e_rainbow_fading();
void e_random_colors();
void e_ma_one();

// Werden von UDP_Request angesteuert und hier ausgewertet:
String MODUS = "strobe";                    // INT TO HOLD THE CURRENT MODE
volatile uint16_t H = 96;                  // INT TO HOLD CURRENT HUE ------------ ANYTHING BETWEEN 0 - 255
volatile uint16_t S = 255;                 // INT TO HOLD CURRENT SATURATION ----- ANYTHING BETWEEN 0 - 255
volatile uint16_t V = 150;                 // INT TO HOLD CURRENT VALUE ---------- ANYTHING BETWEEN 0 - 255
volatile uint16_t DELAY_P = 500;           // INT TO HOLD CURRENT SPEED/ DELAY

volatile boolean STATIC_TEST = 1;          // TEST IF STATIC SHOULD CHANGE COLOR
volatile uint8_t WAHRSCHEINLICHKEIT = 5;    // INT FOR STROBE OR RANDOMSTROBE TO HOLD PROPABILITY
volatile int L_W_WIDTH = 5;
String MA_PARAM;
volatile boolean MA_TEST = 1;
//---------------------------------------

volatile boolean STROBE_TEST = 0;          // TEST FOR STROBE
volatile short int POS_RAINBOW = 0;        // INT TO HOLD POSITION OF RAINBOW
volatile int POS_LIGHT = 0;
unsigned long previousMillis = 0;
volatile int MA_HOLD[6] = {};

CRGB leds[NUM_LEDS];                        // INIT COLOR FOR EVERY LED <- DISPLAYED COLOR

//          INITIALIZE FASTLED-LIB

void LED_BEGIN() {
   FastLED.addLeds<WS2812B, FASTLED_DATA_PIN, GRB>(leds, NUM_LEDS);
}

//          SORT BY MODE AND PLAY IT
void LED_MODE_SORT() {                                          // THIS FUNCTION JUST LOOKS FOR THE MODE AND ACTIVATES ITS FUNCTION
   if (MODUS != "static" && STATIC_TEST == 0) {
      STATIC_TEST = 1;
   }
   if (MODUS == "static") {e_static();}
   else if (MODUS == "strobe") {e_strobe();}
   else if (MODUS == "rainbow_cycle") {e_rainbow_cycle();}
   else if (MODUS == "off") {e_off();}
   else if (MODUS == "random_strobe") {e_random_strobe();}
   else if (MODUS == "walking_light") {e_light_walk();}
   else if (MODUS == "rainbow_fading") {e_rainbow_fading();}
   else if (MODUS == "random_colors") {e_random_colors();}
   else if (MODUS == "ma_one") { e_ma_one();}
}

//          STATIC
void e_static() {                                          // SETS A STATIC HSV
   if (STATIC_TEST) {
      for (int i = 0; i < NUM_LEDS; i++) {
         leds[i] = CHSV(H, S, V);
      }
      STATIC_TEST = 0;
      FastLED.show();
   }
}

//          STROBE
void e_strobe() {                                          // SETS A STROBE WITH HSV COLOR
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= DELAY_P) {
      previousMillis = currentMillis;
      if (STROBE_TEST == 1) {
         for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(H, S, V);
         }
         STROBE_TEST = 0;
         FastLED.show();
      }
      else {
         for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(0, 0, 0);
         }
         FastLED.show();
         STROBE_TEST = 1;
      }
   }
}

//          RAINBOW CYCLE
void e_rainbow_cycle() {                                   // DISPLAYS A RAINBOW CYCLE
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= (DELAY_P/10)) {
      previousMillis = currentMillis;
      if (POS_RAINBOW == 256) { POS_RAINBOW = 0; }
      else {
         for (int i = 0; i < NUM_LEDS; i++) {
            int r_h = (255 / NUM_LEDS)*i - POS_RAINBOW;
            if (r_h < 0) {
               r_h = r_h + 255;
               leds[i] = CHSV(r_h, S, V);
            }
            else {
               leds[i] = CHSV(r_h, S, V);
            }
         }
      }
      FastLED.show();
      POS_RAINBOW = (POS_RAINBOW + 1);
   }
}

//          OFF
void e_off() {                                             // TURNS ALL LEDS OFF
   for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(H, S, 0);
   }
   FastLED.show();
}

//          RANDOM STROBE
void e_random_strobe() {                                   // STROBES RANDOM LEDS
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= DELAY_P) {
      previousMillis = currentMillis;
      int often = WAHRSCHEINLICHKEIT * NUM_LEDS / 10;
      int verlauf[NUM_LEDS];
      for (int i = 0; i < NUM_LEDS; i++) {
         verlauf[i] = i;
      }
      std::random_shuffle(std::begin(verlauf), std::end(verlauf));
      if (STROBE_TEST == 1) {
         for (int i = 0; i < often; i++) {
            leds[verlauf[i]] = CHSV(H, S, V);
         }
         FastLED.show();
         STROBE_TEST = 0;
      }
      else {
         for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(0, 0, 0);
         }
         FastLED.show();
         STROBE_TEST = 1;
      }
   }
}

//          LIGHT WALK
int pos_lw(int i, int pos_l);
int val_lw(int i, int pos_l);

void e_light_walk(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= DELAY_P) {
    previousMillis = currentMillis;
    if (POS_LIGHT > (NUM_LEDS*100)){ POS_LIGHT = 0;}
    for (int i=0;i<NUM_LEDS;i++){
      leds[i] = CHSV(0,0,0);
    }
    int pos_l = POS_LIGHT;
    for (int i=0;i<L_W_WIDTH;i++){
      int pos_led = pos_lw(i, pos_l);
      int val_led = val_lw(i, pos_l);
      leds[pos_led] = CHSV(H,S,val_led);
    }
    FastLED.show();
    
    if ( DELAY_P <= 5){ POS_LIGHT += 50;}
    else if ( DELAY_P > 5 && DELAY_P <= 10 ){ POS_LIGHT += 45;}
    else if(DELAY_P > 10 && DELAY_P <= 20){POS_LIGHT += 40;}
    else if(DELAY_P > 20 && DELAY_P <= 50){POS_LIGHT += 35;}
    else if(DELAY_P > 50 && DELAY_P <= 80){POS_LIGHT += 30;}
    else if(DELAY_P > 80 && DELAY_P <= 120){POS_LIGHT += 25;}
    else if(DELAY_P > 120 && DELAY_P <= 150){POS_LIGHT += 20;}
    else{POS_LIGHT += 10;}
  }
}

int pos_lw(int i, int pos_l){
  pos_l = pos_l-1;
  int c = (pos_l/100)-i;
  if (c<0){
    int d = NUM_LEDS+c;
    return d;
  }
  else{
    return c;
  }
}

int val_lw(int i,int pos_l){
  if (i==0){
    int c = (pos_l%100)*V/100;
    if ( c == 0){
      return V;
    }
    else{
      return c+1;
    }
  }
  else if ( i == (L_W_WIDTH-1)){
    pos_l = pos_l-1;
    int c = (100-(pos_l%100))*V/100;
    return c+1;
  }
  else{
    return V;
  }
}

//          RAINBOW FADING
void e_rainbow_fading() {
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= DELAY_P) {
      previousMillis = currentMillis;
      if (POS_RAINBOW == 256) { POS_RAINBOW = 0; }
      for (int i = 0; i < NUM_LEDS; i++) {
         leds[i] = CHSV(POS_RAINBOW, S, V);
      }
      FastLED.show();
      POS_RAINBOW += 2;
   }
}

//          RANDOM COLORS
void e_random_colors() {
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= DELAY_P) {
      previousMillis = currentMillis;
      for (int i = 0; i < NUM_LEDS; i++) {
         leds[i] = CHSV(random(0, 255), S, V);
      }
      FastLED.show();
   }
}


void e_ma_one(){
  
}