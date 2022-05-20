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

// In lab 7, the microcontroller ran out of HEAP memory (was set as 4096), presumbaly because of the overhead of TivaWare and FreeRTOS includes.
// Changing configTOTAL_HEAP_SIZE in the config to 8192 solved the problem.

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 8 Requirements:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// Create two tasks
//  1. A continuous lower priority prints "this is the counter task" and prints from 0 to 10
//  2. A periodic (i.e., uses vtaskdelay) higher priority handler task for ISR on switch using binary semaphore to trigger it
//      the task itself, when triggered, prints "led toggler task" then toggles red led then delays 500ms
//      upon trying again it tries to get the semaphore and fails so blocks

// Both use a shared resource which is uart0
// Make a function that sends stuff to uart0, both tasks need to call it to use uart
// Therefore we need a mutex for using the function so the two tasks wouldn't cut each other while sending stuff to uart

// in the button ISR you must clear the flag of the interrupt before exiting

// ********************************************************************************************************
// Super Bonus:                                                                                           *
// Configure a Software Interrupt to produce the above behavior without Mutex with all the same givens.   *
// ********************************************************************************************************

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 8 Code:
// ----------------------------------------------------------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <TM4C123GH6PM.h>
#include <core_cm4.h>
#define mainSW_INTERRUPT_ID        ( ( IRQn_Type ) 0 )
#define mainTRIGGER_INTERRUPT()    NVIC_SetPendingIRQ( mainSW_INTERRUPT_ID )
#define mainCLEAR_INTERRUPT()    NVIC_ClearPendingIRQ( mainSW_INTERRUPT_ID )
#define mainSOFTWARE_INTERRUPT_PRIORITY         ( 6 )

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <task.h>

#define RED GPIO_PIN_1
#define BLUE GPIO_PIN_2
#define GREEN GPIO_PIN_3

QueueHandle_t global_queue = NULL;
SemaphoreHandle_t global_semaphore = NULL;

void DELAY(const int ms) {
    for (int j = 0; j < ms; j++)
        for (int i = 0; i < 3180; i++);
}

void SEND_TO_UART0(char* str) {

    while (*str) {

        // Wait if there's no more room for transmit
        while (!UARTSpaceAvail(UART0_BASE));
        while (UARTBusy(UART0_BASE));
        UARTCharPutNonBlocking(UART0_BASE, *(str++));

    }

    UARTCharPutNonBlocking(UART0_BASE, '\n');
    UARTCharPutNonBlocking(UART0_BASE, '\r');

}

void LedTogglerTask(void* pvParams) {

    xSemaphoreTake(global_semaphore, 0);

    while (true) {

        xSemaphoreTake(global_semaphore, portMAX_DELAY);

        // xSemaphoreTake(global_mutex, portMAX_DELAY);
        vTaskSuspendAll();
        SEND_TO_UART0("\n\rThis is the LedToggler task\n\r");
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1));
        // vTaskDelay(500); // The continuous task doesn't resume because this task didn't give the mutex back yet
        DELAY(500);
        // xSemaphoreGive(global_mutex);
        xTaskResumeAll();

    }
}

void CounterTask(void* pvParams) {

    while (true) {
        // xSemaphoreTake(global_mutex, portMAX_DELAY);
        vTaskSuspendAll();
        SEND_TO_UART0("This is the Counter task");
        DELAY(100);
        for (int i = 0; i < 10; i++) {
            char c[2] = { ('0' + i), 0 }; // Null terminator
            SEND_TO_UART0(c);
            DELAY(100);
        }
        SEND_TO_UART0("10");
        DELAY(100);
        // xSemaphoreGive(global_mutex);
        xTaskResumeAll();
    }

}

#include <stdlib.h>
void Triggerer(void* pvParams) {
    while (true) {
        vTaskDelay((rand() & 0x2FFF));
        mainTRIGGER_INTERRUPT();
    }
}

void GPIOA_Handler(void) {
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(global_semaphore, &xHigherPriorityTaskWoken);
    mainCLEAR_INTERRUPT();
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void PORTF_INIT(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

void UART0_INIT(uint32_t baudrate) {
    // GPIO part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // UART part of initialization
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTDisable(UART0_BASE);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), baudrate, UART_CONFIG_WLEN_8
        | UART_CONFIG_STOP_ONE
        | UART_CONFIG_PAR_NONE);
    UARTEnable(UART0_BASE);
}

int main(void) {

    NVIC_SetPriority(mainSW_INTERRUPT_ID, mainSOFTWARE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(mainSW_INTERRUPT_ID);

    PORTF_INIT();
    UART0_INIT(9600);
    global_semaphore = xSemaphoreCreateBinary();
    global_queue = xQueueCreate(10, sizeof(uint32_t));
    if (global_queue != NULL && global_semaphore != NULL) {
        xTaskCreate(LedTogglerTask, "LedTogglerTask", 128, NULL, 4, NULL);
        xTaskCreate(CounterTask, "CounterTask", 128, NULL, 2, NULL);
        xTaskCreate(Triggerer, "Periodic Software Interrupt Trigger", 128, NULL, 3, NULL);
        vTaskStartScheduler();
    }
    while (true);
}
