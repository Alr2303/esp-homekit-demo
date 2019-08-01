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

const int led_gpio = 12;
const int fan_gpio = 5;
const int fan_speed_low = 4;
const int fan_speed_mid = 0;
const int fan_speed_high = 2;
bool led_on = true;
bool fan_on = true;
bool fan_low = true;
bool fan_mid = true;
bool fan_high = true;

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

void fan_write(bool on) {
    gpio_write(fan_gpio, on ? 0 : 1);
}


void fan_speed_low_write(bool on) {
    gpio_write(fan_speed_low, on ? 0 : 1);
}


void fan_speed_mid_write(bool on) {
    gpio_write(fan_speed_mid, on ? 0 : 1);
}

void fan_speed_high_write(bool on) {
    gpio_write(fan_speed_high, on ? 0 : 1);
}


void led_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(led_on);
}

void fan_init() {
    gpio_enable(fan_gpio, GPIO_OUTPUT);
    fan_write(fan_on);
}

void fan_speed() {
    gpio_enable(fan_speed_low, GPIO_OUTPUT);
    fan_speed_low_write(fan_low);
    gpio_enable(fan_speed_mid, GPIO_OUTPUT);
    fan_speed_mid_write(fan_mid);
    gpio_enable(fan_speed_high, GPIO_OUTPUT);
    fan_speed_high_write(fan_high);
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


void fan_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            fan_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            fan_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    fan_write(fan_on);

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


void fan_identify(homekit_value_t _value) {
    printf("FAN identify\n");
    xTaskCreate(fan_identify_task, "FAN identify", 128, NULL, 2, NULL);
}

homekit_value_t fan_on_get() {
    return HOMEKIT_BOOL(fan_on);
}

void fan_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    fan_on = value.bool_value;
    fan_write(fan_on);
}


homekit_value_t fan_low_speed_get() {
    return HOMEKIT_BOOL(fan_speed_low);
}

void fan_low_speed_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    fan_low = value.bool_value;
    fan_speed_low_write(fan_low);
}

homekit_value_t fan_mid_speed_get() {
    return HOMEKIT_BOOL(fan_speed_mid);
}

void fan_mid_speed_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    fan_mid = value.bool_value;
    fan_speed_mid_write(fan_mid);
}

homekit_value_t fan_high_speed_get() {
    return HOMEKIT_BOOL(fan_speed_high);
}

void fan_high_speed_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    fan_high = value.bool_value;
    fan_speed_high_write(fan_high);
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
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fan"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "23031999AAAA"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Fan"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, fan_identify),
            NULL
        }),
        HOMEKIT_SERVICE(FAN, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fan"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=fan_on_get,
                .setter=fan_on_set
            ),
            HOMEKIT_CHARACTERISTIC(
                ROTATION_SPEED, false,
                .getter=fan_low_speed_get,
                .setter=fan_low_speed_set,
                .getter=fan_mid_speed_get,
                .setter=fan_mid_speed_set,
                .getter=fan_high_speed_get,
                .setter=fan_high_speed_set
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
    fan_init();
    fan_speed();
    homekit_server_init(&config);
}
