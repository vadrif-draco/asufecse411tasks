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


#ifndef UART_H_
#define UART_H_

void UART0_INIT(uint32_t baudrate);
void SEND_CHAR_TO_UART0(char c);
void SEND_STR_TO_UART0(char *str);
char READ_CHAR_FROM_UART0(void);
void clearUARTOutput(void);

#endif