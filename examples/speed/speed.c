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

const int fan_gpio = 12;
bool fan_on = false;

void fan_write(bool on) {
    gpio_write(fan_gpio, on ? 0 : 1);
}

void fan_init() {
    gpio_enable(fan_gpio, GPIO_OUTPUT);
    fan_write(fan_on);
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
    fan_write(led_on);
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fan"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "AA2303A1999A"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Fan"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
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
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "230-31-999"
    .setupId="ALR9",
    
};

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_init();
    fan_init();
    homekit_server_init(&config);
}
