// демо матрицы из стандартного примера FastLED

#include "defines.h"
#include "leds.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void rainbow() {
	// FastLED's built-in rainbow generator
	fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) {
	if( random8() < chanceOfGlitter) {
		leds[ random16(NUM_LEDS) ] += CRGB::White;
	}
}

void rainbowWithGlitter() {
	// built-in FastLED rainbow, plus some random sparkly glitter
	rainbow();
	addGlitter(80);
}

void confetti() {
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy( leds, NUM_LEDS, 10);
	int pos = random16(NUM_LEDS);
	leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon() {
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy( leds, NUM_LEDS, 20);
	int pos = beatsin16( 13, 0, NUM_LEDS-1 );
	leds[pos] += CHSV( gHue, 255, 192);
}

void bpm() {
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
	uint8_t dothue = 0;
	for( int i = 0; i < 8; i++) {
		leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
		dothue += 32;
	}
}

void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
  uint8_t lineStartHue = startHue8;
  for( uint8_t y = 0; y < HEIGHT; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;      
    for( uint8_t x = 0; x < WIDTH; x++) {
      pixelHue += xHueDelta8;
	  drawPixelXY(x, y, CHSV( pixelHue, 255, 255));
    }
  }
}
void xyMatrix() {
    uint32_t ms = millis();
    int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
    int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
    DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
// SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
SimplePatternList gPatterns = { xyMatrix, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern() {
	// add one to the current pattern number, and wrap around at the end
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void demo_tick() {
	// Call the current pattern function once, updating the 'leds' array
	gPatterns[gCurrentPatternNumber]();

	// send the 'leds' array out to the actual LED strip
	if(fl_allowLEDS) FastLED.show();  

	// do some periodic updates
	// EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
	gHue++;
	EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}
