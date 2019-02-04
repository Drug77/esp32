#include "ledanimations.h"

#include <string.h>

#include "ws2812.h"

#define WS2812_PIN		18
#define WS2812_PIXELS 	16 // Number of your "pixels"

#define delayMs(ms) vTaskDelay((ms) / portTICK_RATE_MS)
#define min(a, b) (((a) < (b)) ? (a) : (b))

rgbVal *pixels;
rgbVal noColor;

inline rgbVal makeRGBValWithBrightness(uint8_t r, uint8_t g, uint8_t b, uint8_t br)
{
	rgbVal v;

	v.r = min(r * (br / 100.0), 255);
	v.g = min(g * (br / 100.0), 255);
	v.b = min(b * (br / 100.0), 255);

	return v;
}

bool delayWait(uint16_t s) {
	uint16_t d = 510 - s;

	for (uint16_t i = 0; i < (d / 10); i++) {
		if (uxQueueMessagesWaiting(queue) > 0) {
			return true;
		}

		delayMs(10);
	}

	return false;
}

// Input a value 0 to 255 to get a color value
// The colors are a transition r - g - b - back to r
rgbVal wheel(uint8_t wheelPos, uint8_t brightness) {
	wheelPos = 255 - wheelPos;

	if (wheelPos < 85) {
		return makeRGBValWithBrightness(255 - wheelPos * 3, 0, wheelPos * 3, brightness);
	}

	if (wheelPos < 170) {
		wheelPos -= 85;
		return makeRGBValWithBrightness(0, wheelPos * 3, 255 - wheelPos * 3, brightness);
	}

	wheelPos -= 170;

	return makeRGBValWithBrightness(wheelPos * 3, 255 - wheelPos * 3, 0, brightness);
}

// Fill the dots one after the other with a color
void colorWipe(rgbVal c, uint16_t s) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = c;
		ws2812_setColors(WS2812_PIXELS, pixels);

		// Delay 
		if (delayWait(s)) {
			return;
		}
	}

	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = noColor;
		ws2812_setColors(WS2812_PIXELS, pixels);

		// Delay 
		if (delayWait(s)) {
			return;
		}
	}
}

// Theater-style crawling lights.
void theaterChase(rgbVal c, uint16_t s) {
	for (uint8_t j = 0; j < 10; j++) {  // do 10 cycles of chasing
		for (uint8_t q = 0; q < 3; q++) {
			for (uint8_t i = 0; i < WS2812_PIXELS; i = i + 3) {
				if ((i + q) < WS2812_PIXELS) {
					pixels[i + q] = c; // turn every third pixel on
				}
			}
			ws2812_setColors(WS2812_PIXELS, pixels);

			// Delay 
			if (delayWait(s)) {
				return;
			}

			for (uint8_t i = 0; i < WS2812_PIXELS; i = i + 3) {
				if ((i + q) < WS2812_PIXELS) {
					pixels[i + q] = noColor; // turn every third pixel off
				}
			}
			ws2812_setColors(WS2812_PIXELS, pixels);
		}
	}
}

void rainbow(uint8_t b, uint16_t s) {
	for (uint8_t j = 0; j <= 255; j++) {
		for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
			pixels[i] = wheel((i + j) & 255, b);
		}
		ws2812_setColors(WS2812_PIXELS, pixels);
		
		// Delay 
		if (delayWait(s)) {
			return;
		}
	}
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t b, uint16_t s) {
	for(uint16_t j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
		for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
			pixels[i] = wheel(((i * 256 / WS2812_PIXELS) + j) & 255, b);
		}
		ws2812_setColors(WS2812_PIXELS, pixels);
		
		// Delay 
		if (delayWait(s)) {
			return;
		}
	}
}

void candle(uint8_t b) {
	rgbVal c = makeRGBValWithBrightness(255, 147, 41, b);

	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = c;
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void tungsten40W(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 197, 143, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void tungsten100W(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 214, 170, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void halogen(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 241, 224, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void carbonArc(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 250, 244, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void highNoonSun(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 255, 251, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void pureWhite(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(255, 255, 255, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void overcastSky(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(201, 226, 255, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void clearBlueSky(uint8_t b) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = makeRGBValWithBrightness(64, 156, 255, b);
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void color(rgbVal c) {
	for (uint8_t i = 0; i < WS2812_PIXELS; i++) {
		pixels[i] = c;
	}
	ws2812_setColors(WS2812_PIXELS, pixels);
		
	if (uxQueueMessagesWaiting(queue) > 0) {
		return;
	}
	delayMs(100);
}

void ledInit() {
    ws2812_init(WS2812_PIN);

    queue = xQueueCreate(1, sizeof(LedMode));
	noColor = makeRGBVal(0, 0, 0);

	xTaskCreate(ledTask, "ledTask", 2048, NULL, 1, NULL);
}

void ledTask(void* data) {
    struct LedMode ledMode = {"\0", 0, 0, 0, 0, 0};

    rgbVal c = makeRGBVal(ledMode.red, ledMode.green, ledMode.blue);
    pixels = malloc(sizeof(rgbVal) * WS2812_PIXELS);

    while (1) {
        if (uxQueueMessagesWaiting(queue) > 0) {
            xQueueReceive(queue, &ledMode, 0);

            c = makeRGBValWithBrightness(ledMode.red, ledMode.green, ledMode.blue, ledMode.brightness);

			printf("Struct Mode is: %s\n", ledMode.mode);
			printf("Struct Brightness is: %d\n", ledMode.brightness);
			printf("Struct Speed is: %d\n", ledMode.speed);
			printf("Struct Red is: %d\n", ledMode.red);
			printf("Struct Green is: %d\n", ledMode.green);
			printf("Struct Blue is: %d\n", ledMode.blue);
        }

		if (strcmp(ledMode.mode, "color-wipe") == 0) {
			colorWipe(c, ledMode.speed);
		}
		else if (strcmp(ledMode.mode, "theatre-chase") == 0) {
			theaterChase(c, ledMode.speed);
		}
		else if (strcmp(ledMode.mode, "rainbow") == 0) {
			rainbow(ledMode.brightness, ledMode.speed);
		}
        else if (strcmp(ledMode.mode, "rainbow-cycle") == 0) {
			rainbowCycle(ledMode.brightness, ledMode.speed);
		}
		else if (strcmp(ledMode.mode, "candle") == 0) {
			candle(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "tungsten-40w") == 0) {
			tungsten40W(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "tungsten-100w") == 0) {
			tungsten100W(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "halogen") == 0) {
			halogen(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "carbon-arc") == 0) {
			carbonArc(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "high-noon-sun") == 0) {
			highNoonSun(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "pure-white") == 0) {
			pureWhite(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "overcast-sky") == 0) {
			overcastSky(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "clear-blue-sky") == 0) {
			clearBlueSky(ledMode.brightness);
		}
		else if (strcmp(ledMode.mode, "color") == 0) {
			color(c);
		}
		else if (strcmp(ledMode.mode, "none") == 0) {
			color(noColor);
		}

		vTaskDelay(1);
    }
}