#pragma once

#include <homekit/homekit.h>

homekit_value_t fan_active_get();
void fan_active_set(homekit_value_t value);
homekit_value_t fan_speed_get();
void fan_speed_set(homekit_value_t value);

