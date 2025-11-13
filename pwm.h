#pragma once
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void init_pwm_static();
void init_pwm_irq();
void flash_led(uint8_t team_color);
void pwm_breathing();
