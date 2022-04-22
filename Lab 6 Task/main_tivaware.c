// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 6 Requirements:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// Side Note: If you wish, read Mazidi reference to remember how UART is initialized

// - Create two tasks of same priority:
//   1. A task reads SW1 (remember debounce check); when pressed increment a global variable
//   2. A task reads SW2 (^^^^^^^^^^^^^^^^^^^^^^^); when pressed send the global variable's value in queue and return it to zero
//   - Use task yield in the two tasks

// - Create a task of higher priority for UART communication where it receives data from queue and sends to UART0

// It is preferrable to use the TivaWare API Library and PuTTY for the serial communication on UART0

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Notes accumulated throughout the different lab tasks:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// Don't forget to set this in config to be able to use delay timers
// #define configUSE_TIMERS                      1

// Don't forget to enable the functions you want to use, for ex:
// #define INCLUDE_vTaskDelay                    1
// #define INCLUDE_vTaskDelete                   1
// #define INCLUDE_xTaskDelayUntil               1
// ...

// Likewie the hooks you want to use, for ex:
// #define configUSE_IDLE_HOOK                   1
// #define configUSE_TICK_HOOK                   0
// For hooks though you need to implement the function in the code if you set its macro to 1

// Regarding #define configMINIMAL_STACK_SIZE
// From the FreeRTOS docs, its purpose is:
// - The size of the stack used by the idle task.
// - Generally this should not be reduced from the value set in the FreeRTOSConfig.h file provided with the demo application for the port you are using.
// However, in our case, we actually do need to reduce it to 128... I'm not sure why

// Regarding Power Saving (#define configUSE_TICKLESS_IDLE)
// It is common to reduce the power consumed by the microcontroller on which FreeRTOS is running by using the Idle task hook
// to place the microcontroller into a low power state. The power saving that can be achieved by this simple method is limited by
// the necessity to periodically exit and then re-enter the low power state to process tick interrupts. Further, if the frequency
// of the tick interrupt is too high, the energy and time consumed entering and then exiting a low power state for every tick
// will outweigh any potential power saving gains for all but the lightest power saving modes.

// The Idle task is the only task able to run because all the application tasks are either in the Blocked state or in the Suspended state.

// The FreeRTOS tickless idle mode stops the periodic tick interrupt during idle periods (periods when there are no application tasks
// that are able to execute; i.e., whenever the Idle task is the only task able to execute), then makes a correcting adjustment to
// the RTOS tick count value when the tick interrupt is restarted. Stopping the tick interrupt allows the microcontroller to remain
// in a deep power saving state until either an interrupt occurs, or it is time for the RTOS kernel to transition a task into the Ready state. 

// Set configUSE_TICKLESS_IDLE to 1 to use the low power tickless mode, or 0 to keep the tick interrupt running at all times.
// Low power tickless implementations are not provided for all FreeRTOS ports.

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 6 code:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#define RED GPIO_PIN_1
#define BLUE GPIO_PIN_2
#define GREEN GPIO_PIN_3

uint32_t global_counter;
QueueHandle_t global_queue;
TaskHandle_t UART0_TASK_handle;

void DELAY(const int ms) {
    for (int j = 0; j < ms; j++)
        for (int i = 0; i < 3180; i++);
}

void LED_FLASH(const uint8_t LED, const uint32_t FLASH_DURATION) {
    GPIOPinWrite(GPIO_PORTF_BASE, LED, 0xFF);
    DELAY(FLASH_DURATION);
    GPIOPinWrite(GPIO_PORTF_BASE, LED, 0x00);
}

void BTN1_CHK_TASK(void* pvParams) {
    while(true) {
        
        // If you sense a key press
        if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4))) {
            
            // Debounce
            do {
                // vTaskDelay(5); // Debounce check: 5-tick delay (== 5ms @1KHz)
                DELAY(5);
            } while((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)));

            // Then flash the red LED and increment the global counter
            LED_FLASH(RED, 100);
            global_counter++;

            // Wait until you remove your finger from the button
            while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)));
            
            // Debounce again
            do {
                // vTaskDelay(5); // Debounce check: 5-tick delay (== 5ms @1KHz)
                DELAY(5);
            } while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)));

        }
        // Yield so that the other task can check for its own button
        taskYIELD();
    }
}

void BTN2_CHK_TASK(void* pvParams) {
    while(true) {

        // If you sense a key press
        if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0))) {
            
            // Debounce
            do {
                // vTaskDelay(5); // Debounce check: 5-tick delay (== 5ms @1KHz)
                DELAY(5);
            } while((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)));
            
            // Flash LED
            LED_FLASH(BLUE, 100);
            
            // Lower priority of UART0 task
            vTaskPrioritySet(UART0_TASK_handle, 1);
            
            // Send the current value of the global counter, and on success flash LED and restart counter
            if (xQueueSendToBack(global_queue, (void*) &global_counter, 10) == pdTRUE) {
                LED_FLASH(GREEN, 100);
                global_counter = 0;
            }
            
            // Return priority of UART0 task
            vTaskPrioritySet(UART0_TASK_handle, 3);
            
            // Wait until you remove your finger from the button
            while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)));
            
            // Debounce again
            do {
                // vTaskDelay(5); // Debounce check: 5-tick delay (== 5ms @1KHz)
                DELAY(5);
            } while(!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)));

        }
        // Yield so that the other task can check for its own button
        taskYIELD();
    }
}

void UART0_TASK(void* pvParams) {
    while(true) {
        uint32_t message[1];
        if (xQueueReceive(global_queue, (void*) message, portMAX_DELAY) == pdTRUE) {
            uint32_t divisor = 10;
            while(*message / divisor) divisor*=10;
            while(divisor/=10) {
                while(UARTBusy(UART0_BASE)); // Wait if UART0 is busy
                uint8_t div = *message/divisor;
                UARTCharPut(UART0_BASE, '0'+div);
                *message -= (div*divisor);
            }
            UARTCharPut(UART0_BASE, '\n');
            UARTCharPut(UART0_BASE, '\r');
        }
    }
}

void UART0_INIT(uint32_t baudrate) {
    // GPIO part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // UART part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTDisable(UART0_BASE);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), baudrate, UART_CONFIG_WLEN_8
                                                                | UART_CONFIG_STOP_ONE
                                                                | UART_CONFIG_PAR_NONE);
    UARTEnable(UART0_BASE);
}

void PORTF_INIT(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

int main(void) {
    UART0_INIT(9600);
    PORTF_INIT();
    global_queue = xQueueCreate(10, sizeof(uint32_t));
    if (global_queue != NULL) {
        xTaskCreate(BTN1_CHK_TASK, "SW1 Listener", 128, NULL, 2, NULL);
        xTaskCreate(BTN2_CHK_TASK, "SW2 Listener", 128, NULL, 2, NULL);
        xTaskCreate(UART0_TASK, "UART0 Listener", 128, NULL, 3, &UART0_TASK_handle);
        vTaskStartScheduler();
    }
    while (true);
}
