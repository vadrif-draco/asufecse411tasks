// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Lab 4 Notes
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Required:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// 1. Use RTOS APIs to make three tasks
//    - One blinks red every 1s
//    - One blinks blue every 2s
//    - One blinks green every 3s
// All with same priority (so; they'll roundrobin)
   
// 2. Draw timing diagram of tasks like the one in screenshot and lecture for 9s
// 3. Also draw timing diagram of the LED combination shown for 9s as well
   
// 4. In lab documentation, answer 2 questions:
//     1. Give different priorities; re-draw the two diagrams
//     2. vTaskDelay vs. vTaskDelayUntil, does it make a difference which used?

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// Further Notes:
// ----------------------------------------------------------------------------------------------------------------------------------------------------

// Don't forget to set this in config to be able to use delay
// #define configUSE_TIMERS                      1

// Also don't forget to enable the function itself
// #define INCLUDE_vTaskDelay                    1

// Regarding #define configMINIMAL_STACK_SIZE
// From the FreeRTOS docs, its purpose is:
// - The size of the stack used by the idle task.
// - Generally this should not be reduced from the value set in the FreeRTOSConfig.h file provided with the demo application for the port you are using.
// However, in our case, we actually do need to reduce it to 128... I'm not sure why

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>

#include <FreeRTOS.h>
#include <task.h>

#define USE_DELAY_UNTIL
#define SAME_PRIORITIES

static void toinfinity(void* pvParams) {
    uint8_t* params = pvParams;

    #ifdef USE_DELAY_UNTIL
    portTickType tickCount;
    #endif

    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, params[0], 0xFF);
        
        #ifdef USE_DELAY_UNTIL
        tickCount = xTaskGetTickCount();
        xTaskDelayUntil(&tickCount, params[1]*configTICK_RATE_HZ);
        #else
        vTaskDelay(params[1]*configTICK_RATE_HZ);
        #endif
        
        GPIOPinWrite(GPIO_PORTF_BASE, params[0], 0x00);
        
        #ifdef USE_DELAY_UNTIL
        tickCount = xTaskGetTickCount();
        xTaskDelayUntil(&tickCount, params[1]*configTICK_RATE_HZ);
        #else
        vTaskDelay(params[1]*configTICK_RATE_HZ);
        #endif
    }
}

void PORTF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

uint8_t params_r[2] = {GPIO_PIN_1, 1};
uint8_t params_g[2] = {GPIO_PIN_3, 3};
uint8_t params_b[2] = {GPIO_PIN_2, 2};

int main(void) {
    PORTF_Init();
    
    #ifdef SAME_PRIORITIES
    xTaskCreate(toinfinity, "andbeyond_b", 128, (void*)params_b, 1, NULL);
    xTaskCreate(toinfinity, "andbeyond_g", 128, (void*)params_g, 1, NULL);
    xTaskCreate(toinfinity, "andbeyond_r", 128, (void*)params_r, 1, NULL);
    #else
    xTaskCreate(toinfinity, "andbeyond_b", 128, (void*)params_b, 2, NULL);
    xTaskCreate(toinfinity, "andbeyond_g", 128, (void*)params_g, 1, NULL);
    xTaskCreate(toinfinity, "andbeyond_r", 128, (void*)params_r, 3, NULL);
    #endif

    vTaskStartScheduler();
    while (true);
}
