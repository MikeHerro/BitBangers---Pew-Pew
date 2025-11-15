//got all the uncludes from practice practical 
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

// Base library headers ncluded for your convenience.
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/rand.h"

//#include "pwm.h" //our own pwm file 

//////////////////////////////////////////////////////////////////////////////
#define UART_ID      uart1
#define UART_BAUD    2400      //change baud rate to what we need, chat says use 2400 change it later if needed
#define UART_TX_PIN  40
#define UART_RX_PIN  41

#define TRIGGER_PIN   21 //active high push button
#define RELOAD_PIN   26 //active high push button
#define LED_SHOOT_PIN 22   // external LED on GPIO 10
#define LED_HIT_PIN 23   // external LED on GPIO 10

//for sending bytes and stuff
#define TEAM_RED    0
#define TEAM_BLUE   1
#define TEAM_GREEN  2
#define TEAM_YELLOW 3 //still dont know if we want thing (maybe make it white for all three go on?)

#define MAX_ROUNDS 8
#define SHOT_TIME 50000 // in micro seconds
#define RELOAD_TIME 3000000 // in micro seconds

uint8_t make_hit_byte();

// Set the local identity of the current gun
// Could be something configured pregame
static uint8_t MY_PLAYER_ID = 3;        // 0 - 15 (4 bits)
static uint8_t MY_TEAM      = TEAM_BLUE;// 0 - 3 (2 bits)
static uint8_t MY_WEAPON    = 1;        // 0 - 3 (2 bits)

int rounds = MAX_ROUNDS;
bool ready = false;

//////////////////////////////////////////////////////////////////////////////

// packet helpers
uint8_t make_hit_byte()  //makes the byte for the player id set and the weapon id set
{
    return (uint8_t) ((MY_PLAYER_ID << 4) | (MY_TEAM << 2) | (MY_WEAPON << 0));
}

// unpack fields from one byte (for RX) 
//old but masking, ask alex if this makes sense
//will be needed for display
uint8_t get_player(uint8_t b) { return (b >> 4) & 0x0F; } //gets the player id
uint8_t get_team  (uint8_t b) { return (b >> 2) & 0x03; } //gets the team id
uint8_t get_weapon(uint8_t b) { return  b       & 0x03; } //gets the weapon id

//////////////////////////////////////////////////////////////////////////////

//Handles getting glocked
void uart_handler() {
    uart_get_hw(UART_ID)->icr = 1u << 4;
    uint8_t rx_data;
    uart_read_blocking(UART_ID, &rx_data, 1);

    printf("Recieved: %d\n", rx_data);
}

//Initializes a single UART for TX and RX (interrupt is setup for UART1)
void init_uart() {
    // fill in
    uart_init(UART_ID, UART_BAUD);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN)); //TX
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN)); //RX
       
    uart_set_fifo_enabled(UART_ID, false);
    uart_get_hw(UART_ID)->imsc = 1u << 4; //Enables recieve interrupt
    irq_set_exclusive_handler(UART1_IRQ, uart_handler); //added handler
    irq_set_enabled(UART1_IRQ, true); //sets irq enabled on nvic
}

//////////////////////////////////////////////////////////////////////////////

void gpio_handler() {
    //Check if trigger
    if (gpio_get_irq_event_mask(TRIGGER_PIN) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(TRIGGER_PIN, GPIO_IRQ_EDGE_RISE);

        //Check if able to shoot
        if (ready && rounds > 0) {
            timer0_hw->alarm[0] = timer0_hw->timelr + SHOT_TIME;
            uint8_t byte = make_hit_byte();
            printf("Sent: %d\n", byte);
            uart_write_blocking(UART_ID, &byte, 1);
            rounds--;
            ready = false;

            //Implement PWM LEDS
        }
        else {
            printf("Wrong button nigga\n");
        }
    }
    //Check if reload
    else if (gpio_get_irq_event_mask(RELOAD_PIN) & GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(RELOAD_PIN, GPIO_IRQ_EDGE_RISE);

        //Check if already full
        if (rounds != MAX_ROUNDS) {
            ready = false;
            timer0_hw->alarm[0] = timer0_hw->timelr + RELOAD_TIME;
            printf("Reloading...\n");
            rounds = MAX_ROUNDS;
        }
        else {
            printf("Wrong button nigga\n");
        }
    }
}

//Sets up trigger and reloads buttons as inputs and enables irq's
void init_gpio(void) {
    gpio_init(TRIGGER_PIN);
    gpio_init(RELOAD_PIN);

    gpio_set_dir(TRIGGER_PIN, false);
    gpio_set_dir(RELOAD_PIN, false);

    gpio_add_raw_irq_handler_masked(((1u << TRIGGER_PIN) | (1u << RELOAD_PIN)), gpio_handler);
    gpio_set_irq_enabled(TRIGGER_PIN, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(RELOAD_PIN, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(21, true);
}

//////////////////////////////////////////////////////////////////////////////

//Allows for firing
void shot_timer_handler() {
    timer0_hw->intr = 1u << 0;
    ready = true;
}

//Sets up delay between shots and after reloading
void init_shot_timer() {
    timer0_hw->inte = 1u << 0;
    irq_set_exclusive_handler(0, shot_timer_handler);
    irq_set_enabled(0, true);
    timer0_hw->alarm[0] = timer0_hw->timelr + SHOT_TIME;
}

//////////////////////////////////////////////////////////////////////////////

int main() {
    stdio_init_all();

    init_uart();
    init_gpio();
    init_shot_timer();
    
    for (;;) {
        printf("Rounds: %d\n", rounds);
        sleep_ms(1000);
    }
}