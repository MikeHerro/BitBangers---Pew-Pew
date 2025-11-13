#include "pwm.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"


#define LED_R_PIN 37
#define LED_G_PIN 38
#define LED_B_PIN 39

#define PERIOD 1001 


// global variables
static volatile int duty_cycle = 0;   
static volatile int dir = 0; 
static volatile uint8_t breath_team = 0; //global var for team that shot you
static volatile int breathe_active = 0; //use this for led changing



//from lab 5 (modified)
void init_pwm_static() 
{
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

    pwm_set_wrap(slice_r, PERIOD);
    pwm_set_wrap(slice_g, PERIOD);
    pwm_set_wrap(slice_b, PERIOD);

    // Start with LEDs off
    pwm_set_chan_level(slice_r, chan_r, duty_cycle);
    pwm_set_chan_level(slice_g, chan_g, duty_cycle);
    pwm_set_chan_level(slice_b, chan_b, duty_cycle);

    pwm_set_enabled(slice_r, true);
    pwm_set_enabled(slice_g, true);
    pwm_set_enabled(slice_b, true);
}

void init_pwm_irq() 
{
    uint slice_num_37 = pwm_gpio_to_slice_num(37);

    pwm_set_irq0_enabled(slice_num_37, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_breathing);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    uint16_t current_period = pwm_hw->slice[slice_num_37].top;
    duty_cycle = 100;
    dir = 1;
}

void flash_led(uint8_t team_color) //starts everything in here
{
    breath_team = team_color;  
    duty_cycle = 0;                  // start dim
    dir = 0;                   // start increasing
    breathe_active = 1;        // enable breathing
}

void pwm_breathing() 
{
    uint slice_r = pwm_gpio_to_slice_num(LED_R_PIN);
    uint slice_g = pwm_gpio_to_slice_num(LED_G_PIN);
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);

    uint chan_r = pwm_gpio_to_channel(LED_R_PIN);
    uint chan_g = pwm_gpio_to_channel(LED_G_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);

    pwm_hw->intr = 1u << slice_r;

    if (!breathe_active) //if its not active, turns everything off
    {
        pwm_set_chan_level(slice_r, chan_r, 0);
        pwm_set_chan_level(slice_g, chan_g, 0);
        pwm_set_chan_level(slice_b, chan_b, 0);
        return;
    }

    //ripped from lab 5
    if((dir == 0) && (duty_cycle >= 100)) 
    {
        dir = 1;
    }
    else if((dir == 1) && (duty_cycle <= 0))
    {
        dir = 0;
    }
    if(dir == 0)
    {
        duty_cycle += 1;
    }
    else
    {
        duty_cycle -= 1;
    }

    uint16_t current_period = pwm_hw->slice[slice_r].top;
    uint16_t level = (current_period * duty_cycle) / 100;

    //set all to 0 and change the color we want
    pwm_set_chan_level(slice_r, chan_r, 0);
    pwm_set_chan_level(slice_g, chan_g, 0);
    pwm_set_chan_level(slice_b, chan_b, 0);

    //selects which color to breath
    if (breath_team == 0) {
        pwm_set_chan_level(slice_r, chan_r, level);
    } else if (breath_team == 1) {
        pwm_set_chan_level(slice_b, chan_b, level);
    } else if (breath_team == 2) {
        pwm_set_chan_level(slice_g, chan_g, level);
    }
}