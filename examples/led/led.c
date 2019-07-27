#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"


static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

const int led_gpio = 2;
const int led_gpio1 = 4;
bool led_on = false;
bool led_on1 = false;

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

void led_write1(bool on) {
    gpio_write(led_gpio1, on ? 0 : 1);
}

void led_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(led_on);
}

void led_init1() {
    gpio_enable(led_gpio1, GPIO_OUTPUT);
    led_write1(led_on1);
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write(led_on);

    vTaskDelete(NULL);
}


void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write1(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write1(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write1(led_on1);

    vTaskDelete(NULL);
}



void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
    led_write(led_on);
}


void led_identify(homekit_value_t _value) {
    printf("LED identify1\n");
    xTaskCreate(led_identify_task, "LED identify1", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get1() {
    return HOMEKIT_BOOL(led_on1);
}

void led_on_set1(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on1 = value.bool_value;
    led_write1(led_on1);
}



homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "23031999AAAA"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Light"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=led_on_get,
                .setter=led_on_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "23031999AAAA"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Light"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify1),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=led_on_get1,
                .setter=led_on_set1
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "230-31-999"
};

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_init();
    led_init();
    homekit_server_init(&config);
}
