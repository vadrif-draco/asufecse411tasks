#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <FreeRTOS.h>
#include <task.h>

// Don't forget to set this in config to be able to use delay
// #define configUSE_TIMERS                      1

// Also don't forget to enable the function itself
// #define INCLUDE_vTaskDelay                    1

// Regarding #define configMINIMAL_STACK_SIZE
// From the FreeRTOS docs, its purpose is:
// - The size of the stack used by the idle task.
// - Generally this should not be reduced from the value set in the FreeRTOSConfig.h file provided with the demo application for the port you are using.
// However, in our case, we actually do need to reduce it to 128... I'm not sure why

static void toinfinity(void* pvParams) {
    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0xFF);
        vTaskDelay(1000);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
        vTaskDelay(1000);
    }
}

void PORTF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

int main(void) {
    PORTF_Init();
    xTaskCreate(toinfinity, "andbeyond", 420, NULL, 1, NULL);
    vTaskStartScheduler();
    while (true);
}
