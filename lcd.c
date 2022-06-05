#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <task.h>

#include <stdio.h>
#include <math.h>

#include "lcd.h"

/* 
    VSS -> gnd
    VDD -> 5V
    V0  -> Pot
    Rs  -> PC5
    R/W -> gnd
    E   -> PC6
    DB0 -> not connected
    DB1 -> not connected
    DB2 -> not connected
    DB3 -> not connected
    DB4 -> PB4
    DB5 -> PB5
    DB6 -> PB6
    DB7 -> PB7
    A   -> 5V
    K   -> gnd
*/

// We set RS to 1 to write data to DDRAM
// SysTick must be initialized first


void LCD_init(void)
{
    // Initalize Port B as digital output
    SYSCTL_RCGCGPIO_R |= 0x02;					// unlock clock to port B
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {}
	GPIO_PORTB_LOCK_R  = GPIO_LOCK_KEY;			// unlock port B
    GPIO_PORTB_PCTL_R  = 0;						// define port B as gpio only
	GPIO_PORTB_CR_R   |= 0xFF;					// enable changes to port B
    GPIO_PORTB_DIR_R  |= 0xFF;					// enable port B as output
    GPIO_PORTB_DEN_R  |= 0xFF;					// enable port B as digital
    GPIO_PORTB_AMSEL_R = 0x00;					// disable analog function
    GPIO_PORTB_PUR_R   = 0;						// disable any PUR
	// Initialize port C
	SYSCTL_RCGCGPIO_R |= (1 << 2);				// unlock clock to port A
	while ((SYSCTL_PRGPIO_R & (1 << 4)) == 0) {}	
	GPIO_PORTC_CR_R   |= 0x60;					// enable changes to pins PA5 and PA6
    GPIO_PORTC_DIR_R  |= 0x60;					// set PA5 and PA6 as output
    GPIO_PORTC_DEN_R  |= 0x60;					// set PA5 and PA6 as digital
    GPIO_PORTC_AMSEL_R = 0x00;					// disable analog function of port A
    GPIO_PORTC_PUR_R   = 0;						// disable any PUR

    // LCD initialization from datasheet
    DELAY(15);
    LCD_cmd(0x30);
    DELAY(5);
    LCD_cmd(0x30);
    DELAY(1);
    LCD_cmd(0x30);
    LCD_cmd(0x20);
    LCD_cmd(setTo4Bits);
    LCD_cmd(entryMode);
	LCD_cmd(cursorBlink);
    DELAY(2);
			
	LCD_cmd(clearDisplay);
	LCD_cmd(returnHome);	
}


void LCD_Write4bits(char data, char control)
{
	data 	&= 0xF0;					// check that data is only in the four MSB
	control &= 0xF0;					// check that only top bits are sent in control (RS)
	GPIO_PORTB_DATA_R  = data;			// set data before enabling the register
	GPIO_PORTC_DATA_R |= control;		// set control to make stable data before enabling
	GPIO_PORTC_DATA_R |= control | EN;	// enabling data
	DELAY(0);					        // small delay
	GPIO_PORTC_DATA_R &= ~0x60;			// falling edge of enable pin to ensure writing data
	GPIO_PORTB_DATA_R = 0;				// removing data
}


void LCD_cmd(char command)
{
    LCD_Write4bits(command & 0xF0, 0); 						    // write MSB 4 bits of command
    LCD_Write4bits(command << 4, 0);   						    // write LSB 4 bits of control
    if ((command == clearDisplay) || (command == returnHome))   // they take longer
    {
        DELAY(2);
    }
    else
    {
        DELAY(1);
    }
}


// for sending characters
void LCD_data(char data)
{
    LCD_Write4bits(data & 0xF0, RS);    // send MSB 4 bits, and set RS
    LCD_Write4bits(data << 4, RS);      // send LSB 4 bits, and set RS
    DELAY(1);
}


void LCD_WriteString(char* str)
{
    uint32_t i;
    for (i = 0; (*(str+i) != '\0'); i++)
    {
        LCD_data(*(str+i));				// send string character by character
    }
}


void gotoxy(uint32_t x, uint32_t y)
{
	if (y==0)
    {
		LCD_cmd(LCD_SETDDRAMADDR|(x+0x00));
    }
	else if (y==1)
    {
		LCD_cmd(LCD_SETDDRAMADDR|(x+0x40));
    }
}


void DELAY(const int ms) {
    for (int j = 0; j < ms; j++)
        for (int i = 0; i < 3180; i++);
}
