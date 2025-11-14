#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define MAX_ROUNDS 8
#define SHOT_TIME 50000 // in micro seconds
#define RELOAD_TIME 3000000 // in micro seconds

const char* user = "Player 1";
int rounds = MAX_ROUNDS;
bool ready = false;

void init_gpio() {
    gpio_init(10);
    gpio_init(11);
    gpio_set_dir(10, false);
    gpio_set_dir(11, false);
    gpio_set_irq_enabled(10, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(11, GPIO_IRQ_EDGE_RISE, true);
    irq_set_exclusive_handler(22, true);
    irq_set_enabled(22, true);
}

void gpio_handler() {
    if (0u | gpio_get_irq_event_mask(11)) {
        gpio_acknowledge_irq(11, 0xffff);

        if (ready && rounds > 0)
            timer0_hw->alarm[0] = timer0_hw->timelr + SHOT_TIME;
            uart_write_blocking(uart0, user, sizeof(user));
            rounds--;
            ready = false;

            //Implement PWM LEDS
    }
    else if (0u | gpio_get_irq_event_mask(10)) {
        ready = false;
        timer0_hw->alarm[0] = timer0_hw->timelr + RELOAD_TIME;
        rounds = MAX_ROUNDS;
    }
}

void init_uart() {
    // fill in
    gpio_set_function(12, 2);
    gpio_set_function(13, 2);
    uart_init(uart0, 115200);
}

void init_shot_timer() {
    timer0_hw->inte = 1u << 0;
    irq_set_exclusive_handler(0, shot_timer_handler);
    irq_set_enabled(0, true);
    timer0_hw->alarm[0] = timer0_hw->timelr + SHOT_TIME;
}

void shot_timer_handler() {
    ready = true;
}


int main() {
    init_uart();
    init_gpio();
    init_shot_timer();
    for (;;) {
        sleep_ms(1);
    }
}
