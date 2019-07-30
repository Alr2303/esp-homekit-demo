/*
 * Example of using random passwords and setup IDs.
 *
 * Each time device starts, it generates random
 * password and setup ID. It uses SSD1306 OLED display
 * to show password and pairing QR code.
 *
 * SSD1306 is connected via I2C interface:
 *   SDA -> GPIO4
 *   SCL -> GPIO5
 */

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <esp/hwrand.h>

#include <qrcode.h>
#include <i2c/i2c.h>
#include <ssd1306/ssd1306.h>
#include <fonts/fonts.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "toggle.h"
#include "wifi.h"


#define QRCODE_VERSION 2

#define I2C_BUS 0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DEFAULT_FONT FONT_FACE_TERMINUS_6X12_ISO8859_1

static const ssd1306_t display = {
    .protocol = SSD1306_PROTO_I2C,
    .screen = SSD1306_SCREEN,
    .i2c_dev.bus = I2C_BUS,
    .i2c_dev.addr = SSD1306_I2C_ADDR_0,
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
};

static uint8_t display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];

void display_init() {
    i2c_init(I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ_400K);
    if (ssd1306_init(&display)) {
        printf("Failed to initialize OLED display\n");
        return;
    }
    ssd1306_set_whole_display_lighting(&display, false);
    ssd1306_set_scan_direction_fwd(&display, false);
    ssd1306_set_segment_remapping_enabled(&display, true);
}

void display_draw_pixel(uint8_t x, uint8_t y, bool white) {
    ssd1306_color_t color = white ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;
    ssd1306_draw_pixel(&display, display_buffer, x, y, color);
}

void display_draw_pixel_2x2(uint8_t x, uint8_t y, bool white) {
    ssd1306_color_t color = white ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;

    ssd1306_draw_pixel(&display, display_buffer, x, y, color);
    ssd1306_draw_pixel(&display, display_buffer, x+1, y, color);
    ssd1306_draw_pixel(&display, display_buffer, x, y+1, color);
    ssd1306_draw_pixel(&display, display_buffer, x+1, y+1, color);
}

void display_draw_qrcode(QRCode *qrcode, uint8_t x, uint8_t y, uint8_t size) {
    void (*draw_pixel)(uint8_t x, uint8_t y, bool white) = display_draw_pixel;
    if (size >= 2) {
        draw_pixel = display_draw_pixel_2x2;
    }

    uint8_t cx;
    uint8_t cy = y;

    cx = x + size;
    draw_pixel(x, cy, 1);
    for (uint8_t i = 0; i < qrcode->size; i++, cx+=size)
        draw_pixel(cx, cy, 1);
    draw_pixel(cx, cy, 1);

    cy += size;

    for (uint8_t j = 0; j < qrcode->size; j++, cy+=size) {
      cx = x + size;
      draw_pixel(x, cy, 1);
      for (uint8_t i = 0; i < qrcode->size; i++, cx+=size) {
          draw_pixel(cx, cy, qrcode_getModule(qrcode, i, j)==0);
      }
      draw_pixel(cx, cy, 1);
    }

    cx = x + size;
    draw_pixel(x, cy, 1);
    for (uint8_t i = 0; i < qrcode->size; i++, cx+=size)
        draw_pixel(cx, cy, 1);
    draw_pixel(cx, cy, 1);
}

bool qrcode_shown = false;
void qrcode_show(homekit_server_config_t *config) {
    char setupURI[20];
    homekit_get_setup_uri(config, setupURI, sizeof(setupURI));

    QRCode qrcode;

    uint8_t *qrcodeBytes = malloc(qrcode_getBufferSize(QRCODE_VERSION));
    qrcode_initText(&qrcode, qrcodeBytes, QRCODE_VERSION, ECC_MEDIUM, setupURI);

    qrcode_print(&qrcode);  // print on console

    ssd1306_display_on(&display, true);

    ssd1306_fill_rectangle(&display, display_buffer, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, OLED_COLOR_BLACK);
    ssd1306_draw_string(&display, display_buffer, font_builtin_fonts[DEFAULT_FONT], 0, 26, config->password, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
    display_draw_qrcode(&qrcode, 64, 5, 2);

    ssd1306_load_frame_buffer(&display, display_buffer);

    free(qrcodeBytes);
    qrcode_shown = true;
}

void qrcode_hide() {
    if (!qrcode_shown)
        return;

    ssd1306_clear_screen(&display);
    ssd1306_display_on(&display, false);

    qrcode_shown = false;
}

void on_homekit_event(homekit_event_t event) {
    if (event == HOMEKIT_EVENT_PAIRING_ADDED) {
        qrcode_hide();
    } else if (event == HOMEKIT_EVENT_PAIRING_REMOVED) {
        if (!homekit_is_paired())
            sdk_system_restart();
    }
}



static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

// The GPIO pin that is connected to the relay on the Sonoff Dual R2
const int relay0_gpio = 12;
const int relay1_gpio = 5;
// The GPIO pin that is connected to the LED on the Sonoff Dual R2
const int led_gpio = 13;
// The GPIO pin that is oconnected to the button on the Sonoff Dual R2
const int button_gpio = 9;


void relay_write(int relay, bool on) {
    gpio_write(relay, on ? 1 : 0);
}

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

void reset_configuration_task() {
    //Flash the LED first before we start the reset
    for (int i=0; i<3; i++) {
        led_write(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_write(false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // printf("Resetting Wifi Config\n");

    // wifi_config_reset();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Resetting HomeKit Config\n");

    homekit_server_reset();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Restarting\n");

    sdk_system_restart();

    vTaskDelete(NULL);
}

void reset_configuration() {
    printf("Resetting Sonoff configuration\n");
    xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}




int lamp_state = 3;

void top_light_on_set(homekit_value_t value);
void bottom_light_on_set(homekit_value_t value);

homekit_characteristic_t top_light_on = HOMEKIT_CHARACTERISTIC_(
    ON, true,
    .setter=top_light_on_set,
);
homekit_characteristic_t bottom_light_on = HOMEKIT_CHARACTERISTIC_(
    ON, true,
    .setter=bottom_light_on_set,
);

void lamp_state_set(int state) {
    lamp_state = state % 4;
    bool top_on = (state & 1) != 0;
    bool bottom_on = (state & 2) != 0;

    printf("Setting state %d, top = %s, bottom = %s\n",
           lamp_state, (top_on ? "on" : "off"), (bottom_on ? "on" : "off"));

    relay_write(relay0_gpio, top_on);
    relay_write(relay1_gpio, bottom_on);

    if (top_on != top_light_on.value.bool_value) {
        top_light_on.value = HOMEKIT_BOOL(top_on);
        homekit_characteristic_notify(&top_light_on, top_light_on.value);
    }

    if (bottom_on != bottom_light_on.value.bool_value) {
        bottom_light_on.value = HOMEKIT_BOOL(bottom_on);
        homekit_characteristic_notify(&bottom_light_on, bottom_light_on.value);
    }
}

void top_light_on_set(homekit_value_t value) {
    top_light_on.value = value;

    lamp_state_set(
        (top_light_on.value.bool_value ? 1 : 0) |
        (bottom_light_on.value.bool_value ? 2 : 0)
    );
}

void bottom_light_on_set(homekit_value_t value) {
    bottom_light_on.value = value;

    lamp_state_set(
        (top_light_on.value.bool_value ? 1 : 0) |
        (bottom_light_on.value.bool_value ? 2 : 0)
    );
}

void gpio_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(false);

    gpio_enable(relay0_gpio, GPIO_OUTPUT);
    gpio_enable(relay1_gpio, GPIO_OUTPUT);
    relay_write(relay0_gpio, true);
    relay_write(relay1_gpio, true);
}

void toggle_callback(uint8_t gpio) {
    lamp_state_set(lamp_state+1);
}

void lamp_identify_task(void *_args) {
    // We identify the Sonoff by turning top light on
    // and flashing with bottom light
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            relay_write(relay0_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            relay_write(relay0_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    relay_write(relay0_gpio, true);

    vTaskDelete(NULL);
}

void fan_identify_task(void *_args) {
    // We identify the Sonoff by turning top light on
    // and flashing with bottom light
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            relay_write(relay1_gpio, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            relay_write(relay1_gpio, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    relay_write(relay1_gpio, true);

    vTaskDelete(NULL);
}

void lamp_identify(homekit_value_t _value) {
    printf("Lamp identify\n");
    xTaskCreate(lamp_identify_task, "Lamp identify", 128, NULL, 2, NULL);
}

void fan_identify(homekit_value_t _value) {
    printf("Fan identify\n");
    xTaskCreate(fan_identify_task, "Fan identify", 128, NULL, 2, NULL);
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
	    HOMEKIT_CHARACTERISTIC(NAME, "Light"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0"),
            HOMEKIT_CHARACTERISTIC(MODEL, "light"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, lamp_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Top Light"),
            &top_light_on,
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
	    HOMEKIT_CHARACTERISTIC(NAME, "Fan"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ALR"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Fan"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, fan_identify),
            NULL
        }),
        HOMEKIT_SERVICE(FAN, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Bottom Light"),
            &bottom_light_on,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .on_event = on_homekit_event,
};

void generate_random_password(char *password) {
    for (int i=0; i<10; i++) {
        password[i] = hwrand() % 10 + '0';
    }
    password[3] = password[6] = '-';
    password[10] = 0;
}

void generate_random_setup_id(char *setup_id) {
    static char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i=0; i < 4; i++)
        setup_id[i] = chars[hwrand() % 36];
    setup_id[4] = 0;
}

char password[11];
char setup_id[5];

void user_init(void) {
    uart_set_baud(0, 115200);

    display_init();
    gpio_init();
    wifi_init();
    homekit_server_init(&config);

    if (toggle_create(button_gpio, toggle_callback)) {
        printf("Failed to initialize button\n");
    }

    generate_random_password(password);
    generate_random_setup_id(setup_id);
    config.password = password;
    config.setupId = setup_id;

    if (!homekit_is_paired()) {
        qrcode_show(&config);
    }

    homekit_server_init(&config);
}
