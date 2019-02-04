#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
//#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "ledanimations.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

// set AP CONFIG values
#ifdef CONFIG_AP_HIDE_SSID
	#define CONFIG_AP_SSID_HIDDEN 1
#else
	#define CONFIG_AP_SSID_HIDDEN 0
#endif	
#ifdef CONFIG_WIFI_AUTH_OPEN
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_OPEN
#endif
#ifdef CONFIG_WIFI_AUTH_WEP
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WEP
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_ENTERPRISE
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_ENTERPRISE
#endif

// TAG for logging
static const char* TAG = "LED-SERVER";

// HTTP headers and web pages
const static char HTTP_HEADER_OK[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char HTTP_HEADER_NOT_FOUND[] = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
const static char HTTP_HEADER_BAD_REQUEST[] = "HTTP/1.1 400 Bad Request\nContent-type: text/html\n\n";
const static char HTTP_HEADER_INTERNAL_SERVER_ERROR[] = "HTTP/1.1 500 Internal Server Error\nContent-type: text/html\n\n";
const static char HTTP_HEADER_JS[] = "HTTP/1.1 200 OK\nContent-type: application/javascript\n\n";
const static char HTTP_HEADER_CSS[] = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";
const static char HTTP_HEADER_FONT[] = "HTTP/1.1 200 OK\nContent-type: font/woff2\n\n";

const static char HTTP_JSON_HEADER_OK[] = "HTTP/1.1 200 OK\nContent-type: application/json\n\n";
const static char HTTP_JSON_HEADER_BAD_REQEST[] = "HTTP/1.1 400 Bad Request\nContent-type: application/json\n\n";

const static char HTTP_RESPONSE_INVALID_REQUEST_BODY[] = "Invalid request body.";
const static char HTTP_RESPONSE_UNKNOWN_REQUEST[] = "Unkown request.";
const static char HTTP_RESPONSE_MALFORMED_REQUEST[] = "Malformed request.";
const static char HTTP_RESPONSE_BAD_REQUEST[] = "Bad request.";
const static char HTTP_RESPONSE_INVALID_JSON[] = "Invalid JSON in request body.";

const static char HTTP_DEFAULT_HTML[] = "<!DOCTYPE html><html lang=en><head><meta charset=utf-8><meta http-equiv=X-UA-Compatible content=\"IE=edge\"><meta name=viewport content=\"width=device-width,initial-scale=1\"><link rel=icon href=/favicon.ico><title>ws2812b-ui</title><link href=/app.css rel=preload as=style><link href=/app.js rel=preload as=script><link href=/app.css rel=stylesheet></head><body><noscript><strong>We're sorry but ws2812b-ui doesn't work properly without JavaScript enabled. Please enable it to continue.</strong></noscript><div id=app></div><script src=/app.js></script></body></html>";

// embedded binary data
extern const uint8_t appJsStart[] asm("_binary_app_js_start");
extern const uint8_t appJsEnd[]   asm("_binary_app_js_end");
extern const uint8_t appCssStart[] asm("_binary_app_css_start");
extern const uint8_t appCssEnd[]   asm("_binary_app_css_end");
extern const uint8_t fontStart[] asm("_binary_font_woff2_start");
extern const uint8_t fontEnd[]   asm("_binary_font_woff2_end");

typedef struct JsonError {
   char field[16];
   char errorName[32];
   char errorMessage[48];
} JsonError;

/* 	Event group for inter-task communication
	AP - is an Access Point (you will host the network)
	STA - is STA(tion) (you will connect to another network)
*/
static EventGroupHandle_t eventGroup;
const int AP_CONNECTED_BIT = BIT0;
const int STA_CONNECTED_BIT = BIT1;

struct LedMode ledMode = {"\0", 0, 0, 0, 0, 0};

#define CHAR_ARRAY_SIZE(x) (sizeof((x)) / sizeof(char *))

// https://stackoverflow.com/questions/1088622/how-do-i-create-an-array-of-strings-in-c
char *ledModes[] = {
	"color-wipe", "theatre-chase", "rainbow", "rainbow-cycle", 
	"candle", "tungsten-40w", "tungsten-100w", "halogen", 
	"carbon-arc", "high-noon-sun", "pure-white", "overcast-sky", 
	"clear-blue-sky", "color", "none" 
};

// Wifi event handler
static esp_err_t wifiEventHandler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {

	case SYSTEM_EVENT_AP_START:
		break;
		
	case SYSTEM_EVENT_AP_STACONNECTED:
		xEventGroupSetBits(eventGroup, STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		xEventGroupClearBits(eventGroup, STA_CONNECTED_BIT);
		break;
		
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    
	case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(eventGroup, AP_CONNECTED_BIT);
        break;
    
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(eventGroup, AP_CONNECTED_BIT);
        break;
    
	default:
        break;
    }
	  
	return ESP_OK;
}

// setup and start the wifi connection
void wifiSetup() {
	
	eventGroup = xEventGroupCreate();

	#ifdef CONFIG_WIFI_MODE_AP
		// initialize the tcp stack
		tcpip_adapter_init();

		// stop DHCP server
		ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
		
		// assign a static IP to the network interface
		tcpip_adapter_ip_info_t info;
		memset(&info, 0, sizeof(info));
		IP4_ADDR(&info.ip, 192, 168, 10, 1);
		IP4_ADDR(&info.gw, 192, 168, 10, 1);
		IP4_ADDR(&info.netmask, 255, 255, 255, 0);
		ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
		
		// start the DHCP server   
		ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
		
		// initialize the wifi event handler
		ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
		
		// initialize the wifi stack in AccessPoint mode with config in RAM
		wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

		// configure the wifi connection and start the interface
		wifi_config_t ap_config = {
			.ap = {
				.ssid = CONFIG_AP_SSID,
				.password = CONFIG_AP_PASSWORD,
				.ssid_len = 0,
				.channel = CONFIG_AP_CHANNEL,
				.authmode = CONFIG_AP_AUTHMODE,
				.ssid_hidden = CONFIG_AP_SSID_HIDDEN,
				.max_connection = CONFIG_AP_MAX_CONNECTIONS,
				.beacon_interval = CONFIG_AP_BEACON_INTERVAL,			
			},
		};
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
		
		// start the wifi interface
		ESP_ERROR_CHECK(esp_wifi_start());
	#else
		// initialize the tcp stack
		tcpip_adapter_init();

		// initialize the wifi event handler
		ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));

		wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

		wifi_config_t wifi_config = {
			.sta = {
				.ssid = CONFIG_STA_SSID,
				.password = CONFIG_STA_PASSWORD,
			},
		};
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());
	#endif
}

static void httpServerNetconnServe(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**)&buf, &buflen);

		// copy the buffer to extract the first line
		char tBuf[32];
		// note 31, not 32, there's one there for the null terminator
		strncpy(tBuf, buf, 31);
		tBuf[31] = "\0";
		
		// extract the first line, with the request
		char *firstLine = strtok(tBuf, "\n");
		
		if (firstLine) {
			// default page
			if (strstr(firstLine, "GET / ")) {
				ESP_LOGD(TAG, "Sending default page.");
				netconn_write(conn, HTTP_HEADER_OK, sizeof(HTTP_HEADER_OK) - 1, NETCONN_NOCOPY);
				netconn_write(conn, HTTP_DEFAULT_HTML, sizeof(HTTP_DEFAULT_HTML) - 1, NETCONN_NOCOPY);
			}

			else if (strstr(firstLine, "GET /app.js ")) {
				printf("Sending app.js...\n");
				netconn_write(conn, HTTP_HEADER_JS, sizeof(HTTP_HEADER_JS) - 1, NETCONN_NOCOPY);
				netconn_write(conn, appJsStart, appJsEnd - appJsStart, NETCONN_NOCOPY);
			}

			else if (strstr(firstLine, "GET /app.css ")) {
				printf("Sending app.css...\n");
				netconn_write(conn, HTTP_HEADER_CSS, sizeof(HTTP_HEADER_CSS) - 1, NETCONN_NOCOPY);
				netconn_write(conn, appCssStart, appCssEnd - appCssStart, NETCONN_NOCOPY);
			}

			else if (strstr(firstLine, "GET /font.woff2 ")) {
				printf("Sending font.woff2...\n");
				netconn_write(conn, HTTP_HEADER_FONT, sizeof(HTTP_HEADER_FONT) - 1, NETCONN_NOCOPY);
				netconn_write(conn, fontStart, fontEnd - fontStart, NETCONN_NOCOPY);
			}

			// JSON command
			else if (strstr(firstLine, "POST /api/command ")) {
				ESP_LOGD(TAG, "Parsing JSON command.");

				char *body = strstr(buf, "\r\n\r\n");
				if (body != NULL) { 
					body += 4;				

					cJSON *root = cJSON_Parse(body);					
					if (root != NULL) {
						JsonError **jsonErrors = calloc(12, sizeof(JsonError));

						jsonErrors[0] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[0]->field, "mode");
						strcpy(jsonErrors[0]->errorName, "mode.empty");
						strcpy(jsonErrors[0]->errorMessage, "Mode is empty.");
						jsonErrors[1] = NULL;

						jsonErrors[2] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[2]->field, "brightness");
						strcpy(jsonErrors[2]->errorName, "brightness.empty");
						strcpy(jsonErrors[2]->errorMessage, "Brightness is empty.");
						jsonErrors[3] = NULL;

						jsonErrors[4] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[4]->field, "speed");
						strcpy(jsonErrors[4]->errorName, "speed.empty");
						strcpy(jsonErrors[4]->errorMessage, "Speed is empty.");
						jsonErrors[5] = NULL;

						jsonErrors[4] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[4]->field, "red");
						strcpy(jsonErrors[4]->errorName, "red.empty");
						strcpy(jsonErrors[4]->errorMessage, "Red is empty.");
						jsonErrors[7] = NULL;

						jsonErrors[8] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[8]->field, "green");
						strcpy(jsonErrors[8]->errorName, "green.empty");
						strcpy(jsonErrors[8]->errorMessage, "Green is empty.");
						jsonErrors[9] = NULL;

						jsonErrors[10] = malloc(sizeof(JsonError));
						strcpy(jsonErrors[10]->field, "Blue");
						strcpy(jsonErrors[10]->errorName, "blue.empty");
						strcpy(jsonErrors[10]->errorMessage, "Param 3 is empty.");
						jsonErrors[11] = NULL;

						jsonErrors[12] = NULL;

						//uint freeRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
						//ESP_LOGI(TAG, "free RAM is %d.", freeRAM);

						cJSON *item = root->child;
						while (item) {

							// Validate Mode
							if (strcmp(item->string, "mode") == 0) {
								if (cJSON_IsString(item) && (item->valuestring != NULL)) {
									if (strlen(item->valuestring) > 0) {
										free(jsonErrors[0]);
										jsonErrors[0] = NULL;
									}

									bool modeMatch = false;
									for (uint8_t i = 0; i < CHAR_ARRAY_SIZE(ledModes); i++)
									{
										if (strcmp(item->valuestring, ledModes[i]) == 0) {
											modeMatch = true;
										}
									}
									if (!modeMatch) {
										jsonErrors[1] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[1]->field, "mode");
										strcpy(jsonErrors[1]->errorName, "mode.invalid");
										strcpy(jsonErrors[1]->errorMessage, "Mode is invalid.");
									}

									if (jsonErrors[0] == NULL && jsonErrors[1] == NULL) {
										strcpy(ledMode.mode, item->valuestring);
									}
								}
								else {
									strcpy(jsonErrors[0]->field, "mode");
									strcpy(jsonErrors[0]->errorName, "mode.invalid");
									strcpy(jsonErrors[0]->errorMessage, "Mode is not a string.");
								}
							}

							// Validate Brightness
							else if (strcmp(item->string, "brightness") == 0) {
								if (cJSON_IsNumber(item)) {
									free(jsonErrors[2]);
									jsonErrors[2] = NULL;

									if (item->valueint < 1 || item->valueint > 100) {
										jsonErrors[3] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[3]->field, "brightness");
										strcpy(jsonErrors[3]->errorName, "brightness.invalid");
										strcpy(jsonErrors[3]->errorMessage, "Brightness is invalid.");
									}

									if (jsonErrors[3] == NULL) {
										ledMode.brightness = item->valueint;
									}
								}
								else {
									strcpy(jsonErrors[2]->field, "brightness");
									strcpy(jsonErrors[2]->errorName, "brightness.invalid");
									strcpy(jsonErrors[2]->errorMessage, "Brightness is not a digit.");
								}
							}

							// Validate Speed
							else if (strcmp(item->string, "speed") == 0) {
								if (cJSON_IsNumber(item)) {
									free(jsonErrors[4]);
									jsonErrors[4] = NULL;

									if (item->valueint < 10 || item->valueint > 500) {
										jsonErrors[5] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[5]->field, "speed");
										strcpy(jsonErrors[5]->errorName, "speed.invalid");
										strcpy(jsonErrors[5]->errorMessage, "Speed is invalid.");
									}

									if (jsonErrors[5] == NULL) {
										ledMode.speed = item->valueint;
									}
								}
								else {
									strcpy(jsonErrors[4]->field, "speed");
									strcpy(jsonErrors[4]->errorName, "speed.invalid");
									strcpy(jsonErrors[4]->errorMessage, "Speed is not a digit.");
								}
							}

							// Validate Red
							else if (strcmp(item->string, "red") == 0) {
								if (cJSON_IsNumber(item)) {
									free(jsonErrors[6]);
									jsonErrors[6] = NULL;

									if (item->valueint < 0 || item->valueint > 255) {
										jsonErrors[7] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[7]->field, "red");
										strcpy(jsonErrors[7]->errorName, "red.invalid");
										strcpy(jsonErrors[7]->errorMessage, "Red is invalid.");
									}

									if (jsonErrors[7] == NULL) {
										ledMode.red = item->valueint;
									}
								}
								else {
									strcpy(jsonErrors[6]->field, "red");
									strcpy(jsonErrors[6]->errorName, "red.invalid");
									strcpy(jsonErrors[6]->errorMessage, "Red is not a digit.");
								}
							}

							// Validate Green
							else if (strcmp(item->string, "green") == 0) {
								if (cJSON_IsNumber(item)) {
									free(jsonErrors[8]);
									jsonErrors[8] = NULL;

									if (item->valueint < 0 || item->valueint > 255) {
										jsonErrors[9] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[9]->field, "green");
										strcpy(jsonErrors[9]->errorName, "green.invalid");
										strcpy(jsonErrors[9]->errorMessage, "Green is invalid.");
									}

									if (jsonErrors[9] == NULL) {
										ledMode.green = item->valueint;
									}
								}
								else {
									strcpy(jsonErrors[8]->field, "green");
									strcpy(jsonErrors[8]->errorName, "green.invalid");
									strcpy(jsonErrors[8]->errorMessage, "Green is not a digit.");
								}
							}

							// Validate Blue
							else if (strcmp(item->string, "blue") == 0) {
								if (cJSON_IsNumber(item)) {
									free(jsonErrors[10]);
									jsonErrors[10] = NULL;

									if (item->valueint < 0 || item->valueint > 255) {
										jsonErrors[11] = malloc(sizeof(JsonError));
										strcpy(jsonErrors[11]->field, "blue");
										strcpy(jsonErrors[11]->errorName, "blue.invalid");
										strcpy(jsonErrors[11]->errorMessage, "Blue is invalid.");
									}

									if (jsonErrors[11] == NULL) {
										ledMode.blue = item->valueint;
									}
								}
								else {
									strcpy(jsonErrors[10]->field, "blue");
									strcpy(jsonErrors[10]->errorName, "blue.invalid");
									strcpy(jsonErrors[10]->errorMessage, "Blue is not a digit.");
								}
							}

							// Other
							else {
								free(jsonErrors[12]);
								jsonErrors[12] = NULL;

								jsonErrors[12] = malloc(sizeof(JsonError));
								strcpy(jsonErrors[12]->field, "unknown");
								strcpy(jsonErrors[12]->errorName, "unknown");
								strcpy(jsonErrors[12]->errorMessage, "Unknown field present.");
							}

							item = item->next;
						}

						bool isValidJson = true;
						for (uint8_t n = 0; n <= 12; n++) {
							if (jsonErrors[n] != NULL) {
								isValidJson = false;
							}
						}

						if (isValidJson) {
							cJSON *responseJson = cJSON_CreateObject();	
							cJSON_AddStringToObject(responseJson, "mode", ledMode.mode);
							cJSON_AddNumberToObject(responseJson, "brightness", ledMode.brightness);
							cJSON_AddNumberToObject(responseJson, "speed", ledMode.speed);
							cJSON_AddNumberToObject(responseJson, "red", ledMode.red);
							cJSON_AddNumberToObject(responseJson, "green", ledMode.green);
							cJSON_AddNumberToObject(responseJson, "blue", ledMode.blue);
							char *jsonString = cJSON_Print(responseJson);

							ESP_LOGI(TAG, "JSON is VALID.");
							netconn_write(conn, HTTP_JSON_HEADER_OK, sizeof(HTTP_JSON_HEADER_OK) - 1, NETCONN_NOCOPY);
							netconn_write(conn, jsonString, strlen(jsonString), NETCONN_NOCOPY);

							cJSON_Delete(responseJson);

							// Send data to led task
							xQueueSend(queue, &ledMode, 0);
						}
						else {
							cJSON *responseJson = cJSON_CreateObject();	
							cJSON_AddTrueToObject(responseJson, "error");
							cJSON *errors = cJSON_CreateArray();
							cJSON_AddItemToObject(responseJson, "errors", errors);

							for (uint8_t n = 0; n <= 12; n++) {
								if (jsonErrors[n] == NULL) {
									continue;
								}

								cJSON *error = cJSON_CreateObject();
								cJSON_AddItemToArray(errors, error);

								cJSON *field = cJSON_CreateString(jsonErrors[n]->field);
								cJSON_AddItemToObject(error, "field", field);

								cJSON *name = cJSON_CreateString(jsonErrors[n]->errorName);
								cJSON_AddItemToObject(error, "error_name", name);

								cJSON *message = cJSON_CreateString(jsonErrors[n]->errorMessage);
								cJSON_AddItemToObject(error, "error_message", message);
							}

							char *jsonString = cJSON_Print(responseJson);

							ESP_LOGI(TAG, "JSON is INVALID.");
							netconn_write(conn, HTTP_JSON_HEADER_BAD_REQEST, sizeof(HTTP_JSON_HEADER_BAD_REQEST) - 1, NETCONN_NOCOPY);
							netconn_write(conn, jsonString, strlen(jsonString), NETCONN_NOCOPY);

							cJSON_Delete(responseJson);
						}

						// Free allocated memory
						for (uint8_t n = 0; n <= 12; n++) {
							if (jsonErrors[n] != NULL) {
								free(jsonErrors[n]);
							}
						}
						free(jsonErrors);
					}
					else {
						ESP_LOGD(TAG, "Invalid JSON in request body.");
						netconn_write(conn, HTTP_HEADER_BAD_REQUEST, sizeof(HTTP_HEADER_BAD_REQUEST) - 1, NETCONN_NOCOPY);
						netconn_write(conn, HTTP_RESPONSE_INVALID_JSON, sizeof(HTTP_RESPONSE_INVALID_JSON) - 1, NETCONN_NOCOPY);
					}
					
					// Free resources
					cJSON_Delete(root);
				}
				else {
					ESP_LOGD(TAG, "Invalid request body.");
					netconn_write(conn, HTTP_HEADER_BAD_REQUEST, sizeof(HTTP_HEADER_BAD_REQUEST) - 1, NETCONN_NOCOPY);
					netconn_write(conn, HTTP_RESPONSE_INVALID_REQUEST_BODY, sizeof(HTTP_RESPONSE_INVALID_REQUEST_BODY) - 1, NETCONN_NOCOPY);
				}
			}
			
			else { 
				ESP_LOGD(TAG, "Unkown request: %s.", firstLine);
				netconn_write(conn, HTTP_HEADER_NOT_FOUND, sizeof(HTTP_HEADER_NOT_FOUND) - 1, NETCONN_NOCOPY);
				netconn_write(conn, HTTP_RESPONSE_UNKNOWN_REQUEST, sizeof(HTTP_RESPONSE_UNKNOWN_REQUEST) - 1, NETCONN_NOCOPY); 
			};
		}
		else { 
			ESP_LOGD(TAG, "Malformed request.");
			netconn_write(conn, HTTP_HEADER_INTERNAL_SERVER_ERROR, sizeof(HTTP_HEADER_INTERNAL_SERVER_ERROR) - 1, NETCONN_NOCOPY);
			netconn_write(conn, HTTP_RESPONSE_MALFORMED_REQUEST, sizeof(HTTP_RESPONSE_MALFORMED_REQUEST) - 1, NETCONN_NOCOPY); 
		}
	}
	else {
		ESP_LOGD(TAG, "Bad request.");
		netconn_write(conn, HTTP_HEADER_INTERNAL_SERVER_ERROR, sizeof(HTTP_HEADER_INTERNAL_SERVER_ERROR) - 1, NETCONN_NOCOPY);
		netconn_write(conn, HTTP_RESPONSE_BAD_REQUEST, sizeof(HTTP_RESPONSE_BAD_REQUEST) - 1, NETCONN_NOCOPY);
	}
	
	// close the connection and free the buffer
	netconn_close(conn);
	netbuf_delete(inbuf);
}

static void httpServer(void *pvParameters) {
	struct netconn *conn, *newconn;
	err_t err;

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	ESP_LOGI(TAG, "HTTP Server listening...");

	do {
		err = netconn_accept(conn, &newconn);
		ESP_LOGD(TAG, "New client connected.");
		if (err == ERR_OK) {
			httpServerNetconnServe(newconn);
			netconn_delete(newconn);
		}

		vTaskDelay(1); //allows task to be pre-empted
	} while (err == ERR_OK);

	netconn_close(conn);
	netconn_delete(conn);
}

// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	wifiSetup();
	
	#ifdef CONFIG_WIFI_MODE_STA
		// wait for connection
		printf("Connecting to network SSID=%s ...\n", CONFIG_STA_SSID);
		xEventGroupWaitBits(eventGroup, AP_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
		printf("Connected.\n");
		
		// print the local IP address
		tcpip_adapter_ip_info_t ip_info;
		ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
		printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
		printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
		printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	#else
		printf("Starting access point, SSID=%s ...\n", CONFIG_AP_SSID);
		// print the local IP address
		tcpip_adapter_ip_info_t ip_info;
		ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));
		printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
		printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
		printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	#endif	
	
	// run the mDNS daemon
	//mdns_server_t* mDNS = NULL;
	//ESP_ERROR_CHECK(mdns_init(TCPIP_ADAPTER_IF_STA, &mDNS));
	//ESP_ERROR_CHECK(mdns_set_hostname(mDNS, "esp32"));
	//ESP_ERROR_CHECK(mdns_set_instance(mDNS, "Basic HTTP Server"));
	//printf("mDNS started\n");

	ledInit();
	
	// start the HTTP Server task
    xTaskCreate(&httpServer, "http_server", 4096, NULL, 1, NULL);
}
