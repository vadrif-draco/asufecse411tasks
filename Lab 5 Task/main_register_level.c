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
#include <FreeRTOS.h>
#include <task.h>
#include "../tm4c123gh6pm.h"

#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)
void vApplicationIdleHook();
static void blink_led(void* pvParams) {
    uint16_t* params = pvParams;
	
    while (true) {
        GPIO_PORTF_DATA_R ^= params[0];
        
        vTaskDelay(params[1]);
    }
}

void PORTF_Init(void) {
    SYSCTL_RCGCGPIO_R = 0x20U;
    GPIO_PORTF_DIR_R = 0x0EU;
    GPIO_PORTF_DEN_R = 0x0EU;
}

uint16_t params_r[2] = {LED_RED, 1000};
uint16_t params_g[2] = {LED_GREEN, 2000};

void vApplicationIdleHook()
{	

	GPIO_PORTF_DATA_R ^= LED_BLUE;
	__asm("wfi");
}
int main(void) {
    PORTF_Init();
    xTaskCreate(blink_led, "Blink Red", 128, (void*)params_r, 1, NULL);
    xTaskCreate(blink_led, "Blink Green", 128, (void*)params_g, 1, NULL);
	
	
    vTaskStartScheduler();
    while (true);
}
