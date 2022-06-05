#include "uart.h"


void UART0_INIT(uint32_t baudrate)
{
    // GPIO part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // UART part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTDisable(UART0_BASE);
    UARTConfigSetExpClk(UART0_BASE, 
                        SysCtlClockGet(), 
                        baudrate, 
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    UARTEnable(UART0_BASE);
}


void SEND_CHAR_TO_UART0(char c)
{
    // Wait if there's no more room for transmit
    while (!UARTSpaceAvail(UART0_BASE));
    while (UARTBusy(UART0_BASE));
    UARTCharPutNonBlocking(UART0_BASE, c);
}


char READ_CHAR_FROM_UART0(void)
{
    // Wait if there's no more room for transmit
	while(!UARTCharsAvail(UART0_BASE));
	return (char) UARTCharGetNonBlocking(UART0_BASE);
}


void SEND_STR_TO_UART0(char *str)
{
    while (*str)
    {
        SEND_CHAR_TO_UART0(*(str++));
    }
}

void clearUARTOutput(void){
	SEND_CHAR_TO_UART0(27);     // this is the escape
	SEND_CHAR_TO_UART0('[');
	SEND_CHAR_TO_UART0('2');
	SEND_CHAR_TO_UART0('J');
	SEND_CHAR_TO_UART0(27);     // this is the escape
	SEND_CHAR_TO_UART0('[');
	SEND_CHAR_TO_UART0('H');
}
