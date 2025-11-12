#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

//////////////////////////////////////////////////////////////////////////////

// No autotest, but you might want to set this anyway.
const char* username = "herrom";

// When testing basic TX/RX
// #define STEP2
// When connecting UART to printf(), fgets()
// #define STEP3
// When testing UART IRQ for buffering
// #define STEP4
// When testing PCS
 #define STEP5

//////////////////////////////////////////////////////////////////////////////

void init_uart() {
    // fill in
    uart_init(uart0, 115200);

    uart_set_hw_flow(uart0, false, false);
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));  
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
       
    uart_set_format(uart0, 8, 1, 0); 
}

#ifdef STEP2
int main() {
    init_uart();
    for (;;) {
        char buf[2];
        uart_read_blocking(uart0, (uint8_t*)buf, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0'; // Ensure null-termination
        uart_puts(uart0, "You said: ");
        uart_puts(uart0, buf);
        uart_puts(uart0, "\n");
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP3

// 3.3
int _read(__unused int handle, char *buffer, int length) {
    // Your code here to read from the UART and fill the buffer.
    // DO NOT USE THE STDIO_* FUNCTIONS FROM ABOVE.  Only UART ones.

    // The argument "handle" is unused.  This is meant for use with 
    // files, which are not very different from text streams.  However, 
    // we read from the UART, not the file specified by the handle.

    // handle is irrelevant since these functions will only ever be called 
    // by the correct functions.  No need for an if statement.

    // Instructions: Given the buffer and a specific length to read, read 1 
    // character at a time from the UART until the buffer is 
    // filled or the length is reached. 

    //uart_read_blocking(uart0, (uint8_t *) buffer, (size_t) length);

    char letter = '\0'; 
    for(int i = 0; i < length; i++)
    {
        letter = uart_getc(uart0);

        if((letter == 8) && (i > 0))
        {
           uart_putc_raw(uart0, 8);
           uart_putc_raw(uart0, 32);
           
           if(i > 0)
           {
            uart_putc_raw(uart0, 8);
            i--;
            continue;
           }
        }
        else if(letter == 10)
        {
            uart_putc(uart0, 13);
            uart_putc(uart0, 10);
        }
        else
        {
            buffer[i] = letter;
            uart_putc_raw(uart0, buffer[i]);
        }
    }

    return(length);
}

int _write(__unused int handle, char *buffer, int length) {
    // Your code here to write to the UART from the buffer.
    // DO NOT USE THE STDIO_* FUNCTIONS FROM ABOVE.  Only UART ones.

    // The argument "handle" is unused.  This is meant for use with 
    // files, which are not very different from text streams.  However, 
    // we write to the UART, not the file specified by the handle.

    // handle is irrelevant since these functions will only ever be called 
    // by the correct functions.  No need for an if statement.

    // Instructions: Given the buffer and a specific length to write, write 1
    // character at a time to the UART until the length is reached. 
    for(int i = 0; i < length; i++)
    {
        uart_putc(uart0, buffer[i]);
    }

    return length;
}

int main()
{
    init_uart();

    // insert any setbuf lines below...


    /*setbuf(stdout, NULL);  // Disable buffering for stdout
    setbuf(stdin, NULL);   // Disable buffering for stdin
    for(;;) {
        putchar(getchar()); 
    }*/



    //step 3.4
    setbuf(stdout, NULL);  // Disable buffering for stdout
    setbuf(stdin, NULL);   // Disable buffering for stdin
    char name[8];
    int age = 0;
    for(;;) {
    //printf("Enter your name and age: ");
    //scanf("%s %d", name, &age);
    //printf("Hello, %s! You are %d years old.\n", name, age);
    _read(0, name, 8);
    sleep_ms(100);  // in case the output loops and is too fast
}
}
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP4

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
void uart_rx_handler();

void init_uart_irq() {
    // fill in.
    uart_set_fifo_enabled(uart0, false);
    uart_set_irqs_enabled(uart0, true, false);
    irq_set_exclusive_handler(33, uart_rx_handler); //UART0_IRQ = 33
    irq_set_enabled(33, true);

}

void uart_rx_handler() {
    // fill in.
    hw_clear_bits(&uart0_hw->icr, 1u<<0);
    char c;
    if(seridx == BUFSIZE)
    {
        return;
    }
    else
    {
        c = (uart0_hw->dr & 0xFF);
        if(c == 0x0A)
        {
            newline_seen = 1;
        }
        if((c == 8) && (seridx > 0))
        {
            uart_putc_raw(uart0, 8);
            uart_putc_raw(uart0, 32);
            uart_putc_raw(uart0, 8); 
            seridx--;
            serbuf[seridx] = '\0';
            return;
        }
        else
        {
            uart_putc_raw(uart0, c);
            serbuf[seridx++] = c; //ASK IF THIS SHOULD INCREMENT HERE OR IN THE NEXT LINE
            //seridx++;
        }
    }

}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
    while(newline_seen == 0)
    {
        sleep_ms(5);
    }

    newline_seen = 0;

    if(length >= seridx)
    {
        for(int i = 0; i < seridx; i++)
        {
            buffer[i] = serbuf[i];
        }
    }
    else
    {
        for(int i = 0; i < length; i++)
        {
            buffer[i] = serbuf[i];
        }
    }
    seridx = 0;
    return length; 
}

int _write(__unused int handle, char *buffer, int length) {
    // fill in.
    uart_write_blocking(uart0,(uint8_t *)buffer,(size_t) length);

    return length;
}

int main() {
    // fill in.
    init_uart();
    init_uart_irq();

    setbuf(stdout, NULL); // Disable buffering for stdout

    char name[8];
    int age = 0;
    for(;;) {
        printf("Enter your name and age: ");
        scanf("%s %d", name, &age);
        // THIS IS IMPORTANT.
        fflush(stdin);
        printf("Hello, %s! You are %d years old.\r\n", name, age);
        sleep_ms(100);  // in case the output loops and is too fast
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP5

// Copy global variables, init_uart_irq, uart_rx_handler, _read, and _write from STEP4.

void cmd_gpio(int argc, char **argv) {
    // This is the main command handler for the "gpio" command.
    // It will call either cmd_gpio_out or cmd_gpio_set based on the arguments.
    
    // Ensure that argc is at least 2, otherwise print an example use case and return.
    if(argc < 2)
    {
        printf("Not enough arguments.\n"); //was thinking of adding more to this 
        return;
    }
    int pin_num;
    // If the second argument is "out":
    //      Ensure that argc is exactly 3, otherwise print an example use case and return.
    //      Convert the third argument to an integer pin number using atoi.
    //      Check if the pin number is valid (0-47), otherwise print an error and return.
    //      Set the pin to output using gpio_init and gpio_set_dir.
    //      Print a success message.
    if(strcmp(argv[1], "out") == 0)
    {
        if(argc == 3)
        {
            pin_num = atoi(argv[2]);
            if((pin_num <= 47) && (pin_num >= 0))
            {
                gpio_init(pin_num);
                gpio_set_dir(pin_num, true);
                printf("Initialized pin %d as output.\n", pin_num);
            }
            else
            {
                printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin_num);
                return;
            }
        }
        else
        {
            printf("Not enough arguments. Missing pin number.\n");
            return;
        }
    }
    
    // If the second argument is "set":
    //      Ensure that argc is exactly 4, otherwise print an example use case and return.
    //      Convert the third argument to an integer pin number using atoi.
    //      Check if the pin number is valid (0-47), otherwise print an error and return.
    //      Check if the pin has been initialized as a GPIO output, if not, return.
    //      Convert the fourth argument to an integer value (0 or 1) using atoi.
    //      Check if the value is valid (0 or 1), otherwise print an error and return.
    //      Set the pin to the specified value using gpio_put.
    //      Print a success message.
    else if(strcmp(argv[1], "set") == 0)
    {
        if(argc == 4)
        {
            pin_num = atoi(argv[2]);
            if((pin_num <= 47) && (pin_num >= 0))
            {
                if(gpio_get_dir(pin_num) == GPIO_OUT) //idk how to check if it has been initialized as an output
                {
                    int value = atoi(argv[3]);
                    if(value == 1 || value == 0)
                    {
                        gpio_put(pin_num, value); 
                        printf("Set pin %d as %d.\n", pin_num, value);
                    }
                    else
                    {
                        printf("Invalid value number: %d. Must be 0 or 1.\n", value);
                    }
                }
                else
                {
                    printf("Pin %d is not initialized as an output.\n", pin_num);
                    return;
                }
            }
            else
            {
                printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin_num);
                return;
            }
        }
        else
        {
            printf("Not enough arguments. Missing pin number and/or value.\n");
            return;
        }
        
    }
    
    // Else, print an unknown command error.

    else
    {
        printf("Unknown command: %s", argv[0]);
    }

}

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
void uart_rx_handler();

void init_uart_irq() {
    // fill in.
    uart_set_fifo_enabled(uart0, false);
    uart_set_irqs_enabled(uart0, true, false);
    irq_set_exclusive_handler(33, uart_rx_handler); //UART0_IRQ = 33
    irq_set_enabled(33, true);

}

void uart_rx_handler() {
    // fill in.
    hw_clear_bits(&uart0_hw->icr, 1u<<0);
    char c;
    if(seridx == BUFSIZE)
    {
        return;
    }
    else
    {
        c = (uart0_hw->dr & 0xFF);
        if(c == 0x0A)
        {
            newline_seen = 1;
        }
        if((c == 8) && (seridx > 0))
        {
            uart_putc_raw(uart0, 8);
            uart_putc_raw(uart0, 32);
            uart_putc_raw(uart0, 8); 
            seridx--;
            serbuf[seridx] = '\0';
            return;
        }
        else
        {
            uart_putc_raw(uart0, c);
            serbuf[seridx++] = c; //ASK IF THIS SHOULD INCREMENT HERE OR IN THE NEXT LINE
            //seridx++;
        }
    }
    //hw_clear_bits(&uart0_hw->icr, 1u<<0);

}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
    while(newline_seen == 0)
    {
        sleep_ms(5);
    }

    newline_seen = 0;

    if(length >= seridx)
    {
        for(int i = 0; i < seridx; i++)
        {
            buffer[i] = serbuf[i];
        }
    }
    else
    {
        for(int i = 0; i < length; i++)
        {
            buffer[i] = serbuf[i];
        }
    }
    seridx = 0;
    return length; 
}

int _write(__unused int handle, char *buffer, int length) {
    // fill in.
    uart_write_blocking(uart0,(uint8_t *)buffer,(size_t) length);

    return length;
}

int main() {
    // See lab for instructions.
    init_uart();
    init_uart_irq();
    setbuf(stdout, NULL);
    printf("%s's Peripheral Command Shell (PCS)\n", username);
    printf("Enter a command below.\n\n");
    int argc;
    char *argv[10];
    char input[100];
   
    for(;;)
    {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
     
        

        fflush(stdin);

        input[strcspn(input, "\n")] = '\0';

        //dont know how to do this part.
        char *token = strtok(input, " ");
        argc = 0;
        while(token != NULL && argc < 10)
        {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        cmd_gpio(argc, argv);
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////
