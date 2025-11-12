#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

//////////////////////////////////////////////////////////////////////////////
#define UART_ID      uart0
#define UART_BAUD    2400      //change baud rate to what we need, chat says use 2400 change it later if needed
#define UART_TX_PIN  0
#define UART_RX_PIN  1

#define BUTTON_PIN   3 //active high push button 


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


static void init_button(void) {
    gpio_init(BUTTON_PIN); //use pin 3
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // since button is active HIGH
}
//////////////////////////////////////////////////////////////////////////////

// packet helpers
uint8_t make_hit_byte(uint8_t player_id, uint8_t weapon_id) { //makes the byte for the player id set and the weapon id set
    return (uint8_t)(((player_id & 0x0F) << 4) | (weapon_id & 0x0F));
}

void send_hit(uint8_t player_id, uint8_t weapon_id) {
    uint8_t b = make_hit_byte(player_id, weapon_id);
    // send it a few times with small gaps for reliability
    for (int i = 0; i < 3; i++) { //the for loop gives the reciever a few chances to be hit by one press 4 bytes sent every 100 ms bassically
        uart_write_blocking(UART_ID, &b, 1);
        sleep_ms(15); // short pause between repeats
    }
}

//////////////////////////////////////////////////////////////////////////////
int main() {
    //stdio_init_all(); // optional

    init_uart();
    init_button();

    bool last = false;
    uint8_t my_player_id = 3; //we want players to choose right
    uint8_t my_weapon_id = 1; //idk if we want to do this or not

    while (1) {
        bool now = gpio_get(BUTTON_PIN);
        if (now && !last) { //this will prevent someone from holding the button (edge detecting bassically)
            // button pressed -> send packet
            send_hit(my_player_id, my_weapon_id);
        }
        last = now; 

        // optional receive check (does nothing yet)
        if (uart_is_readable(UART_ID)) {
            uint8_t b = uart_getc(UART_ID);
            (void)b;
        }

        sleep_ms(1);
    }
}

//////////////////////////////////////////////////////////////////////////////