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

const int light_gpio1 = 2;
//Light assigned @ gpio1

const int fan_gpio2 = 3;
//Fan assigned @ gpio2

bool light_on = false;
bool fan_on = false;
//Function light write
void light_write(bool on) {
    gpio_write(light_gpio1, on ? 0 : 1);
}

//Function fan write
void fan_write(bool on) {
    gpio_write(fan_gpio2, on ? 0 : 1);
}
//Initialize light
void light_init() {
    gpio_enable(light_gpio1, GPIO_OUTPUT);
    light_write(light_on);
}

//Initialize fan
void fan_init() {
    gpio_enable(fan_gpio2, GPIO_OUTPUT);
    fan_write(fan_on);
}

//IDENTIFY TASK
void identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            light_write(true);
            fan_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            light_write(false);
            fan_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    light_write(light_on);
    fan_write(fan_on);

    vTaskDelete(NULL);
}

//IDENTITY LIGHT
void light_identify(homekit_value_t _value) {
    printf("Light identify\n");
    xTaskCreate(identify_task, "Light identify", 128, NULL, 2, NULL);
}
//IDENTITY FAN
void fan_identify(homekit_value_t _value) {
    printf("Fan identify\n");
    xTaskCreate(identify_task, "Fan identify", 128, NULL, 2, NULL);
}


homekit_value_t light_on_get() {
    return HOMEKIT_BOOL(light_on);
}

homekit_value_t fan_on_get() {
    return HOMEKIT_BOOL(fan_on);
}


void light_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    light_on = value.bool_value;
    light_write(light_on);
}


void fan_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    fan_on = value.bool_value;
    fan_write(fan_on);
}


homekit_accessory_t *accessories[] = {
    //FOR LIGHT
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "23031999AAAA"),
            HOMEKIT_CHARACTERISTIC(MODEL, "LIGHT"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
            NULL
    
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=light_on_get,
                .setter=light_on_set
                NULL
                
        
            
        }),
        NULL
    }),
    //FOR FAN
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fan"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "23032000AAAA"),
            HOMEKIT_CHARACTERISTIC(MODEL, "FAN"),
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
    light_init();
    fan_init();
    homekit_server_init(&config);
}
