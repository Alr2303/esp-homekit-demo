#pragma once

#include <homekit/characteristics.h>
#include <homekit/homekit.h>

typedef struct fan_state_struct {
  bool active;
  float rotationSpeed;
} fan_state_t;

extern fan_state_t FAN;

void on_homekit_event(homekit_event_t event);
extern bool homekit_initialized;


extern homekit_characteristic_t fan_active;
extern homekit_characteristic_t fan_rotation_speed;

extern homekit_server_config_t homekit_config;
