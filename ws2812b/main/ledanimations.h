#ifndef LEDANIMATIONS
#define LEDANIMATIONS

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// https://stackoverflow.com/questions/1675351/typedef-struct-vs-struct-definitions
typedef struct LedMode {
   char mode[16];
   uint8_t brightness;
   uint16_t speed;
   uint8_t red;
   uint8_t green;
   uint8_t blue;
} LedMode;

QueueHandle_t queue;

void ledInit();

void ledTask(void* data);

#endif