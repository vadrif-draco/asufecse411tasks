#include "../tm4c123gh6pm.h"
#include <FreeRTOS.h>
#include <task.h>

#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)

void blink_red_task(void*);

int main() {
    SYSCTL_RCGCGPIO_R = 0x20U;
    GPIO_PORTF_DIR_R = 0x0EU;
    GPIO_PORTF_DEN_R = 0x0EU;
    
    xTaskCreate(blink_red_task, "Blink red task", 200, NULL,1,NULL);
    vTaskStartScheduler();
    while (1) {
    }
}   

void blink_red_task(void* parameters) {
    while (1) {
        GPIO_PORTF_DATA_R ^= LED_RED;
        vTaskDelay(1000);
    }
}
