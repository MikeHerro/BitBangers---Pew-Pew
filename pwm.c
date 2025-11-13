#include "pwm.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"


#define LED_R_PIN 37
#define LED_G_PIN 38
#define LED_B_PIN 39


// Breathing state
static volatile int duty_cycle         = 0;   // 0–100%
static volatile int dir                = 0;   // 0 = up, 1 = down
static volatile uint8_t hit_team_color = 0;   // 0=red,1=blue,2=green
static volatile int hit_ticks_remaining = 0;  // how long effect runs
static volatile int breathe_divider    = 0;   // slows breathing

//from lab 5 (modified)
void init_pwm_static(uint32_t period, uint32_t duty) {
    gpio_set_function(LED_R_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_G_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);

    uint slice_r = pwm_gpio_to_slice_num(LED_R_PIN);
    uint slice_g = pwm_gpio_to_slice_num(LED_G_PIN);
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);

    uint chan_r = pwm_gpio_to_channel(LED_R_PIN);
    uint chan_g = pwm_gpio_to_channel(LED_G_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);

    // Nice slow PWM for smooth breathing
    pwm_set_clkdiv(slice_r, 150);
    pwm_set_clkdiv(slice_g, 150);
    pwm_set_clkdiv(slice_b, 150);

    pwm_set_wrap(slice_r, period - 1);
    pwm_set_wrap(slice_g, period - 1);
    pwm_set_wrap(slice_b, period - 1);

    // Start with LEDs off
    pwm_set_chan_level(slice_r, chan_r, duty);
    pwm_set_chan_level(slice_g, chan_g, duty);
    pwm_set_chan_level(slice_b, chan_b, duty);

    pwm_set_enabled(slice_r, true);
    pwm_set_enabled(slice_g, true);
    pwm_set_enabled(slice_b, true);
}

static void init_pwm_irq(void) {
    uint slice_r = pwm_gpio_to_slice_num(LED_R_PIN);

    pwm_set_irq0_enabled(slice_r, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_breathing);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    duty_cycle          = 0;
    dir                 = 0;
    hit_team_color      = 0;
    hit_ticks_remaining = 0;
    breathe_divider     = 0;
}

void pwm_hit_init(void) {
    // period=1000, start duty=0 (off)
    init_pwm_static(1000, 0);
    init_pwm_irq();
}


void pwm_hit_start(uint8_t team_id) //need to edit this one for later, just a rough idea
{
    // Expect 0=red, 1=blue, 2=green
    if (team_id > 2) {
        team_id = 0;  // default to red if unknown
    }

    hit_team_color      = team_id;
    hit_ticks_remaining = 800;  // ~1s of breathing; tweak to taste

    duty_cycle      = 0;
    dir             = 0;
    breathe_divider = 0;
}


static void pwm_breathing(void) {
    uint slice_r = pwm_gpio_to_slice_num(LED_R_PIN);
    uint slice_g = pwm_gpio_to_slice_num(LED_G_PIN);
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);

    uint chan_r = pwm_gpio_to_channel(LED_R_PIN);
    uint chan_g = pwm_gpio_to_channel(LED_G_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);

    // Clear IRQ for this slice
    pwm_hw->intr = 1u << slice_r;

    // If no hit active, keep LEDs off
    if (hit_ticks_remaining <= 0) {
        pwm_set_chan_level(slice_r, chan_r, 0);
        pwm_set_chan_level(slice_g, chan_g, 0);
        pwm_set_chan_level(slice_b, chan_b, 0);
        return;
    }

    hit_ticks_remaining--;

    uint16_t current_period = pwm_hw->slice[slice_r].top;

    // Slow breathing down: update brightness every N interrupts
    breathe_divider++;
    if (breathe_divider >= 5) {
        breathe_divider = 0;

        if ((dir == 0) && (duty_cycle >= 100)) {
            dir = 1;
        } else if ((dir == 1) && (duty_cycle <= 0)) {
            dir = 0;
        }

        if (dir == 0) {
            duty_cycle += 1;
        } else {
            duty_cycle -= 1;
        }
    }

    uint16_t level = (current_period * duty_cycle) / 100;

    uint16_t r_level = 0, g_level = 0, b_level = 0;

    // Map team → LED color
    // 0 = red team  -> red led
    // 1 = blue team -> blue led
    // 2 = green team-> green led
    if (hit_team_color == 0) {
        r_level = level;
    } else if (hit_team_color == 1) {
        b_level = level;
    } else if (hit_team_color == 2) {
        g_level = level;
    }

    pwm_set_chan_level(slice_r, chan_r, r_level);
    pwm_set_chan_level(slice_g, chan_g, g_level);
    pwm_set_chan_level(slice_b, chan_b, b_level);
}