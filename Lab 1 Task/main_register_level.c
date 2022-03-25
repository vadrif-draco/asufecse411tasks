#include "../tm4c123gh6pm.h"
#include <stdint.h> 

#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)

void blink_red(void);
void blink_blue(void);
void delay(uint32_t);
void SysTick_Handler(void);

int main() {
    SYSCTL_RCGCGPIO_R = 0x20U;
    GPIO_PORTF_DIR_R = 0x0EU;
    GPIO_PORTF_DEN_R = 0x0EU;
    NVIC_ST_RELOAD_R = 100 * 16000 - 1;
    NVIC_ST_CTRL_R = 7;         
    // Enable interrupts to the processor.
    __asm("CPSIE  I");
    
    uint8_t volatile anti_optimization = 42;
    if (anti_optimization) blink_red();
    else blink_blue();
    while (1);
}
static uint32_t volatile l_tickCtr;


void delay(uint32_t ticks) {
	__asm("CPSID  I");
	
    uint32_t cur_value = l_tickCtr;
	uint32_t start = cur_value;
	
	__asm("CPSIE  I");



    while ((cur_value - start) < ticks) {
		__asm("CPSID  I");
		cur_value = l_tickCtr;
		__asm("CPSIE  I");
    }
}
void SysTick_Handler(void) {
    ++l_tickCtr;
}

void blink_red(void) {
    while (1) {
        GPIO_PORTF_DATA_R = LED_RED;
        delay(5);
        GPIO_PORTF_DATA_R &= ~LED_RED;
        delay(5);
    }
}

void blink_blue(void) {
    while (1) {
        GPIO_PORTF_DATA_R = LED_BLUE;
        delay(10);
        GPIO_PORTF_DATA_R &= ~LED_BLUE;
        delay(10);
    }
}
