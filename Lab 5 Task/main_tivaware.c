// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 5 Requirements:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// 1. Create two tasks of same priority
//        - One blinks red every 1s
//        - One blinks green every 2s
// 2. Toggle blue LED inside vApplicationIdleHook
// 3. Draw timing diagram of tasks and LED lights like in the previous task

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
// Lab 5 code:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <FreeRTOS.h>
#include <task.h>

static void toinfinity(void* pvParams) {
    uint8_t* params = pvParams;
    while (true) {
        vTaskDelay(params[1]*configTICK_RATE_HZ);
        GPIOPinWrite(GPIO_PORTF_BASE, params[0], ~GPIOPinRead(GPIO_PORTF_BASE, params[0]));
    }
}

void PORTF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0xFF);
}

// Parameters for function: LED to blink, blink delay time in seconds
uint8_t params_r[2] = {GPIO_PIN_1, 1}; // Red for 1s
uint8_t params_g[2] = {GPIO_PIN_3, 2}; // Green for 2s
uint32_t counter = 0;

int main(void) {
    PORTF_Init();
    xTaskCreate(toinfinity, "andbeyond_r", 128, (void*)params_r, 1, NULL);
    xTaskCreate(toinfinity, "andbeyond_g", 128, (void*)params_g, 1, NULL);
    vTaskStartScheduler();
    while (true);
}

#if configUSE_IDLE_HOOK == 1
void vApplicationIdleHook(void) {
    // __asm("wfi");

#elif configUSE_TICK_HOOK == 1
void vApplicationTickHook(void) {

#endif

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2));
    ++counter;
}
