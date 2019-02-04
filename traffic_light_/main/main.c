#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define delayMs(ms) vTaskDelay((ms) / portTICK_RATE_MS)

#define ESP_INTR_FLAG_DEFAULT 0

SemaphoreHandle_t xSemaphore = NULL;

// interrupt service routine, called when the button is pressed
void IRAM_ATTR button_isr_handler(void* arg) {
	
    // notify the button task
	xSemaphoreGiveFromISR(xSemaphore, NULL);
}

// task that will react to button clicks
void button_task(void* arg) {
	uint8_t mode = 0;
	
	// infinite loop
	for (;;) {
		bool toggle = false;
		printf("Current mode is: %u\n", mode);
		
		switch (mode)
		{
			case 0:
				// Red
				printf("Red\n");

				gpio_set_level(CONFIG_RED_LED_PIN, 1);
				gpio_set_level(CONFIG_YELLOW_LED_PIN, 0);
				gpio_set_level(CONFIG_GREEN_LED_PIN, 0);
				delayMs(6000);

				toggle = false;
				for (uint8_t n = 0; n < 5; n ++) {
					toggle = !toggle;
					gpio_set_level(CONFIG_RED_LED_PIN, toggle);
					delayMs(500);
				}

				gpio_set_level(CONFIG_YELLOW_LED_PIN, 1);
				delayMs(2000);
				break;

			case 1:
				// Green
				printf("Green\n");
				
				gpio_set_level(CONFIG_RED_LED_PIN, 0);
				gpio_set_level(CONFIG_YELLOW_LED_PIN, 0);
				gpio_set_level(CONFIG_GREEN_LED_PIN, 1);

				for (uint8_t n = 0; n < 12; n++) {
					// wait for the notification from the ISR
					if (xSemaphoreTake(xSemaphore, 1) == pdTRUE) {
						printf("Button pressed!\n");
						mode = 0;

						// Yellow
						gpio_set_level(CONFIG_RED_LED_PIN, 0);
						gpio_set_level(CONFIG_YELLOW_LED_PIN, 0);
						gpio_set_level(CONFIG_GREEN_LED_PIN, 0);

						toggle = false;
						for (uint8_t n = 0; n < 5; n ++) {
							toggle = !toggle;
							gpio_set_level(CONFIG_YELLOW_LED_PIN, toggle);
							delayMs(1000);
						}

						break;
					}

					delayMs(1000);
				}

				if (mode == 1) {
					toggle = false;
					for (uint8_t n = 0; n < 5; n ++) {
						toggle = !toggle;
						gpio_set_level(CONFIG_GREEN_LED_PIN, toggle);
						delayMs(500);
					}
				} 
				else {
					continue;
				}	

				break;
		}

		mode ++;
		mode = mode % 2;
	}
}

void app_main()
{
	
	// create the binary semaphore
	xSemaphore = xSemaphoreCreateBinary();
	
	// configure button and led pins as GPIO pins
	gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
	gpio_pad_select_gpio(CONFIG_RED_LED_PIN);
	gpio_pad_select_gpio(CONFIG_YELLOW_LED_PIN);
	gpio_pad_select_gpio(CONFIG_GREEN_LED_PIN);
	
	// set the correct direction
	gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_RED_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(CONFIG_YELLOW_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(CONFIG_GREEN_LED_PIN, GPIO_MODE_OUTPUT);

	// enable pull-up for button pin
	gpio_set_pull_mode(CONFIG_BUTTON_PIN, GPIO_PULLUP_ONLY);
	
	// enable interrupt on low level for button pin
	gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
	
	// start the task that will handle the button
	xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);
	
	// install ISR service with default configuration
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	
	// attach the interrupt service routine
	gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);
}