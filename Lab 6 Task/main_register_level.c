// Don't forget to unset CLOCK_SETUP in system_TM4C123.c (Startup)
// #define CLOCK_SETUP 0

#include <stdint.h>
#include <stdbool.h>
#include <FreeRTOS.h>
#include <task.h>
#include "queue.h"
#include "../tm4c123gh6pm.h"

#define RED (1 << 1)
#define BLUE (1 << 2)
#define GREEN (1 << 3)

void DELAY(uint8_t ms) {
    for (int j = 0; j < ms; j++)
        for (int i = 0; i < 3180; i++);
}

void FLASH(uint32_t LED, uint8_t ms) {
    GPIO_PORTF_DATA_R |= LED;
    DELAY(ms);
    GPIO_PORTF_DATA_R &=~LED;
}

xQueueHandle xQueue;

uint8_t counter = 0;
void initTask(void);
void BTN1_CHK_TASK(void* pvParams);
void BTN2_CHK_TASK(void* pvParams);
void UART_TASK(void* pvParams);
void send_byte(uint8_t one_byte);


int main(void) {
    initTask();
    xQueue = xQueueCreate( 5, sizeof( uint8_t ) );

    if( xQueue != NULL )
    {
        
        xTaskCreate(BTN1_CHK_TASK, "BTN1_CHK_TASK", 128, NULL, 1, NULL);
        xTaskCreate(BTN2_CHK_TASK, "BTN2_CHK_TASK", 128, NULL, 1, NULL);
    
        /* Create the task that will read from the queue.  The task is created with
        priority 2, so above the priority of the sender tasks. */
        xTaskCreate( UART_TASK, "Receiver", 128, NULL, 2, NULL );

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }

    while (true);
}

void initTask(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20; // Activating the clock on Port F
    while ((SYSCTL_PRGPIO_R & 0x20) == 0); // Ensure activation
    GPIO_PORTF_LOCK_R = 0x4C4F434B; // Unlock port F registers
    GPIO_PORTF_CR_R = 0x1F; // Allow changes in first 5 pins
    GPIO_PORTF_DIR_R = 0xE; // Set direction of LED pins out
    GPIO_PORTF_PUR_R = 0x11; // Setting switches as pull-up
    GPIO_PORTF_DEN_R = 0x1F; // Enabling digital operations


    /* UART0 initialization */
    SYSCTL_RCGCGPIO_R |= 0x1; // Activating the clock on Port A
    while ((SYSCTL_PRGPIO_R & 0x1) == 0); // Ensure activation
    
    SYSCTL_RCGCUART_R = 0x1U;
    while ((SYSCTL_RCGCUART_R & 0x1) == 0);
    
    UART0_CTL_R = 0; /* disable UART0 */
    UART0_IBRD_R = 104; /* 16MHz/16=1MHz, 1MHz/104=9600 baud rate */
    UART0_FBRD_R = 11; /* fraction part, see Example 4-4 */
    UART0_CC_R = 0; /* use system clock */
    UART0_LCRH_R = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
    UART0_CTL_R = 0x301; /* enable UART0, TXE, RXE */
    
    GPIO_PORTA_AFSEL_R|=0x3;           //Enabling alternate function
    GPIO_PORTA_DEN_R|=0x3;             //digital enable
    GPIO_PORTA_PCTL_R=((GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x11);
    GPIO_PORTA_AMSEL_R&=~0x3;
}

void BTN1_CHK_TASK(void* pvParams){
    for(;;)
    {
        if ((GPIO_PORTF_DATA_R & 1) == 0) {
            DELAY(10);
            if ((GPIO_PORTF_DATA_R & 1) == 0) {
                FLASH(RED, 20);
                ++counter;
            }
            while ((GPIO_PORTF_DATA_R & 1) == 0);
            DELAY(10);
            while ((GPIO_PORTF_DATA_R & 1) == 1);
        }
        taskYIELD();
    }
}
void BTN2_CHK_TASK(void* pvParams){
    for(;;)
    {
        if ((GPIO_PORTF_DATA_R & 0x10) == 0) {
            DELAY(10);
            if ((GPIO_PORTF_DATA_R & 0x10) == 0){
                FLASH(BLUE, 20);
                xQueueSendToBack( xQueue, &counter, 0 );
                FLASH(GREEN, 20);
                counter = 0;
            }
            while ((GPIO_PORTF_DATA_R & 0x10) == 0);
            
            if ((GPIO_PORTF_DATA_R & 0x10) == 1) {
                DELAY(10);
                while ((GPIO_PORTF_DATA_R & 0x10) == 0);
            }
        }
        taskYIELD();
    }
}

void UART_TASK(void* pvParams){
    portBASE_TYPE xStatus;
    uint8_t lReceivedValue;
    
    for(;;)
    {
        xStatus = xQueueReceive( xQueue, &lReceivedValue, portMAX_DELAY );

        if( xStatus == pdPASS ) {
            uint32_t divisor = 10;
            while(lReceivedValue / divisor) divisor*=10;
            while(divisor/=10) {
                uint8_t div = lReceivedValue/divisor;
                send_byte('0'+div);
                lReceivedValue -= (div*divisor);
            }
            send_byte('\n');
            send_byte('\r');
        }
        
        taskYIELD();
    }
}
void send_byte(uint8_t one_byte){
    while((UART0_FR_R&0x20)!=0);  //monitoring bit field 5 to check that 
    DELAY(10);
    UART0_DR_R=one_byte;          //any previous transmission has completed or not
}
