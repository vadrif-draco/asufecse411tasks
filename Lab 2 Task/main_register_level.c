#include "../tm4c123gh6pm.h"
#include <stdint.h> 

#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)


void blink_red(void);
void blink_blue(void);
void delay(uint32_t);
void SysTick_Handler(void);

uint32_t blink_red_stack[40];
uint32_t *blink_red_sp = &blink_red_stack[40];

uint32_t blink_blue_stack[40];
uint32_t *blink_blue_sp = &blink_blue_stack[40];

int main() {
    SYSCTL_RCGCGPIO_R = 0x20U;
    GPIO_PORTF_DIR_R = 0x0EU;
    GPIO_PORTF_DEN_R = 0x0EU;
    NVIC_ST_RELOAD_R = 100 * 16000 - 1;
    NVIC_ST_CTRL_R = 7;         
    // Enable interrupts to the processor.
    //__asm("CPSIE  I");
	
    *(--blink_red_sp) = (1U << 24);  // PSR
    *(--blink_red_sp) = (uint32_t)&blink_red; // PC
    *(--blink_red_sp) = 0x0000000EU; // LR
    *(--blink_red_sp) = 0x0000000CU; // R12
    *(--blink_red_sp) = 0x00000003U; // R3
    *(--blink_red_sp) = 0x00000002U; // R2
    *(--blink_red_sp) = 0x00000001U; // R1
    *(--blink_red_sp) = 0x00000000U; // R0
	
    *(--blink_blue_sp) = (1U << 24);  // PSR
    *(--blink_blue_sp) = (uint32_t)&blink_blue; // PC
    *(--blink_blue_sp) = 0x0000000EU; // LR 
    *(--blink_blue_sp) = 0x0000000CU; // R12
    *(--blink_blue_sp) = 0x00000003U; // R3 
    *(--blink_blue_sp) = 0x00000002U; // R2 
    *(--blink_blue_sp) = 0x00000001U; // R1 
    *(--blink_blue_sp) = 0x00000000U; // R0 

    while (1) {
    }
}    
static uint32_t volatile l_tickCtr;


void delay(uint32_t ticks) {
	__asm("CPSID  I");
	
    uint32_t cur_value = l_tickCtr;

	__asm("CPSIE  I");
	
	uint32_t start = cur_value;
	
    while ((cur_value - start) < ticks) {
		__asm("CPSID  I");
		cur_value = l_tickCtr;
		__asm("CPSIE  I");
    }
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


void SysTick_Handler(void) {
    ++l_tickCtr;
}




