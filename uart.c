//got all the uncludes from practice practical 
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "support.h"

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



//////////////////////////////////////////////////////////////////////////////
#define UART_ID      uart0
#define UART_BAUD    2400      //change baud rate to what we need, chat says use 2400 change it later if needed
#define UART_TX_PIN  0
#define UART_RX_PIN  1

#define BUTTON_PIN   3 //active high push button 
#define LED_HIT_PIN 10   // external LED on GPIO 10

//for sending bytes and stuff
#define TEAM_RED    0
#define TEAM_BLUE   1
#define TEAM_GREEN  2
#define TEAM_YELLOW 3

// Set the local identity of the current gun
static uint8_t MY_PLAYER_ID = 3;        // 0 - 15 (4 bits)
static uint8_t MY_TEAM      = TEAM_BLUE;// 0 - 3 (2 bits)
static uint8_t MY_WEAPON    = 1;        // 0 - 3 (2 bits)

//////////////////////////////////////////////////////////////////////////////
void init_uart() {
    // fill in
    uart_init(UART_ID, UART_BAUD);  

    uart_set_hw_flow(UART_ID, false, false);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));  //TX
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN)); //RX
       
    uart_set_format(UART_ID, 8, 1, 0);
    uart_set_fifo_enabled(UART_ID, false);
    uart_set_irqs_enabled(UART_ID, true, false); 
}


void init_button(void) {
    gpio_init(BUTTON_PIN); //use pin 3
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // since button is active HIGH
}
//////////////////////////////////////////////////////////////////////////////

// packet helpers
uint8_t make_hit_byte(uint8_t player_id_0_15, uint8_t team_0_3, uint8_t weapon_0_3)  //makes the byte for the player id set and the weapon id set
{
    return (uint8_t)(((player_id_0_15 & 0x0F) << 4) | ((team_0_3 & 0x03) << 2) | ( weapon_0_3 & 0x03));
}

void send_hit(uint8_t player_id, uint8_t team, uint8_t weapon) {
    uint8_t b = make_hit_byte(player_id, team, weapon);
    for (int i = 0; i < 3; i++) //the for loop gives the reciever a few chances to be hit by one press 3 bytes sent every 75 ms bassically 
    {
        uart_write_blocking(UART_ID, &b, 1);
        sleep_ms(15);
    }
}

// unpack fields from one byte (for RX)
uint8_t get_player(uint8_t b) { return (b >> 4) & 0x0F; } //gets the player id
uint8_t get_team  (uint8_t b) { return (b >> 2) & 0x03; } //gets the team id
uint8_t get_weapon(uint8_t b) { return  b       & 0x03; } //gets the weapon id

//////////////////////////////////////////////////////////////////////////////
int main() {
    init_uart();
    init_button();

    bool last = false;

    while (1) {

        //TX stuff
        bool now = gpio_get(BUTTON_PIN);
        if (now && !last) { //this will prevent someone from holding the button (edge detecting bassically)
            // button pressed -> send packet
            send_hit(MY_PLAYER_ID, MY_TEAM, MY_WEAPON);
            sleep_ms(20); // tiny debounce so one press = one shot
        }
        last = now; 

        //RX stuff
        if (uart_is_readable(UART_ID)) {
            uint8_t b = uart_getc(UART_ID);

            uint8_t tx_team   = get_team(b);
            uint8_t tx_player = get_player(b);
            uint8_t tx_weapon = get_weapon(b);

            // reject friendly fire
            if (tx_team == MY_TEAM) {
                // same team -> invalid packet for scoring; ignore
            } else {
                //we need to flash the led and pause the thing (THIS NEEDS TO BE DONE)
                flash_hit_led(); //needs to be PWM flash 
                sleep_ms(40); //prevents flashing the led from the same hit multiple times
                
            }
        }
        
        sleep_ms(1);
    }
}

//////////////////////////////////////////////////////////////////////////////