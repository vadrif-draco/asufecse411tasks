#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <task.h>

#include <stdio.h>
#include <math.h>

#include "uart.h"
#include "sensor.h"
#include "lcd.h"


// Declaration of the tasks
void task1(void *pvParameters);
void LCDTask(void *pvParameters);
void UARTTask(void *pvParameters);
void buzzerTask(void *pvParameters);


// Struct for the message to display on the LCD
// Contains a character array for current temperature
// and another character array for the set point
typedef struct Message
{
	char cur_temp[8];
	char set_point[8];
} Message;


// Global queues declaration
xQueueHandle xUARTQueue;
xQueueHandle xLCDQueue;
xQueueHandle xBuzzerQueue;


int main(void)
{
    // Initializations for sensor, LCD, and port F
    initSensor();
    LCD_init();
    SYSCTL_RCGCGPIO_R	|= (1 << 4);                    // turn on bus clock for GPIOF
    GPIO_PORTE_DIR_R	|= (1 << 4);                    //set GREEN pin as a digital output pin
    GPIO_PORTE_DEN_R 	|= (1 << 4);                    // Enable PF3 pin as a digital pin

	
	// Queue definitions
    xLCDQueue = xQueueCreate(1, sizeof(Message));
	xUARTQueue = xQueueCreate(1, sizeof(uint32_t));
	xBuzzerQueue = xQueueCreate(1, sizeof(char));
    // xUARTQueue = xQueueCreate(1, 1);
    // xBuzzerQueue = xQueueCreate(1, 1);
    

    //  Create all the TASKS here

    //  Create Task 1
    xTaskCreate(task1,"MAIN Controller", 128, NULL, 2, NULL);
    //  Create LCD Task
	xTaskCreate(LCDTask,"LCD Controller", 128, NULL, 2, NULL);
    //  Create UART Task
	xTaskCreate(UARTTask,"UART Controller", 64, NULL, 2, NULL);
    //  Create Buzzer Task
	xTaskCreate(buzzerTask,"Buzzer Controller", 64, NULL, 2, NULL);
	
    // Start scheduler
    vTaskStartScheduler();

	while(1);
}


// Implementation of Task 1
void task1(void *pvParameters)
{
    // Variables declarations and definitions
    Message msg;

    unsigned char setpoint = 30;
    uint32_t ADC_value;
    unsigned char temperature;
    float mv;

    unsigned const char alarm_value = 50;
	
	char alarm = 0;

    // Main task loop
    while (1)
    {
        // Receive data from UART queue
        xQueueReceive(xUARTQueue, &setpoint, 0);
		
        ADC_value = readSensor();                   // Read ADC Channel 2
        mv = ADC_value * 3300.0 / 4096.0;           // in mv
        mv = (mv / 3300) * 80.0;                 // Temp in C
        temperature = (int)mv;                      // Temp as integer

        // If cold, turn LED on
        if (temperature < setpoint)
        {
			GPIO_PORTE_DATA_R  = (1 << 4);
        }
        // If hot, turn LED off
        else
        {
			GPIO_PORTE_DATA_R  = 0x00;
        }
		
		sprintf(msg.cur_temp, "%d", temperature);
		sprintf(msg.set_point, "%d", setpoint);
		
		xQueueSend(xLCDQueue,&msg, 0);
		
        // If temparature higher than alarm value, set buzzer ON
        if (temperature > alarm_value)
        {
			alarm = 1;
            xQueueSend(xBuzzerQueue, &alarm, 0);
		}
        // Else, set buzzer OFF
        else
        {
            alarm = 0;
            xQueueSend(xBuzzerQueue, &alarm, 0);
        }
		
		vTaskDelay(500);
    }
}


// Implementation of UART Task
void UARTTask(void *pvParameters)
{
    // Variables declarations and definitions
    unsigned char N;
    uint32_t total;
    
    // Main task loop
    while (1)
    {
        SEND_STR_TO_UART0("\n\r\n\rEnter Temperature Setpoint (Degrees): ");
        N = 0;
        total = 0;
        
        while (1)
        {
            // Read a number
            N = READ_CHAR_FROM_UART0();
            // Echo the number
            SEND_CHAR_TO_UART0(N);

            // If Enter
            if (N == '\r')
                break;

            N = N - '0';
            // Total number
            total = 10 * total + N;
        }

        // Send via Queue
        xQueueSend(xUARTQueue, &total, 0);
        SEND_STR_TO_UART0("\n\rTemperature setpoint changed...");
    }
}


// Implementation of LCD Task
void LCDTask(void *pvParameters)
{
    // Variables declarations and definitions
    Message msg;

    // Initialize LCD
    UART0_INIT(9600);
    LCD_init();
    LCD_cmd(clearDisplay);
    LCD_cmd(returnHome);
    LCD_cmd(cursorOff);

    gotoxy(0,0);
    LCD_WriteString("Measured: ");
    gotoxy(0,1);
    LCD_WriteString("Setpoint: ");

    // Main task loop
    while (1)
    {
        // Receive data from LCD queue
        xQueueReceive(xLCDQueue, &msg, 0);

        // Clear numbers on LCD
        gotoxy(9,0);
        LCD_WriteString("    ");
        gotoxy(9,1);
        LCD_WriteString("    ");

        // Rewrite to LCD
        gotoxy(10,0);
        LCD_WriteString(msg.cur_temp);
        gotoxy(10,1);
        LCD_WriteString(msg.set_point);
        
        // Wait 1 second
        vTaskDelay(1000);
    }
}


// Implementation of Buzzer Task
void buzzerTask(void *pvParameters)
{
    // Variables declarations and definitions
    unsigned char BuzzerState;
    BuzzerState = 0;                                // Default state
    while (1)
    {
        // Get data
        xQueueReceive(xBuzzerQueue, &BuzzerState, 0);
        
        // Alarm?
        if (BuzzerState == 1) 
        {
            // TOGGLE BUZZER
            GPIO_PORTB_DATA_R ^= (1 << 0);
        }
        else
        {
            // Buzzer OFF
            GPIO_PORTB_DATA_R &= ~(1 << 0);
        }
        
        // Delay by 100ms
        DELAY(100);
    }
}