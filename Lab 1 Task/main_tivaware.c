#define TEXAS

#ifdef TEXAS
#include "../TExaS Simulation (open with Keil uVision4)/TExaS.h"
#include "../TExaS Simulation (open with Keil uVision4)/tm4c123gh6pm.h"
#endif

#include <stdint.h> 
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/interrupt.h>

// To switch context...
// 1. Put a breakpoint here (in the ++),
// 2. See where SP points at once it hits,
// 3. Change 7th register in stack (PC) with the other function's address
// Note that in Keil the disassembly window gets auto-scrolled to match the code
// This way you could scroll down to the function you want to go to in both windows
static volatile uint32_t global_100ms_tick_count;
void systick_timeout_ISR(void) {
    global_100ms_tick_count++;
}
void SysTick_Handler(void) { systick_timeout_ISR(); }

void SYSTICK_init(void) {
    SysTickDisable();
    SysTickIntDisable();
    SysTickIntRegister(systick_timeout_ISR);
    SysTickPeriodSet(100 * 16000 - 1);
    SysTickIntEnable();
    SysTickEnable();
}

void PORTF_init(void) {
    #ifndef TEXAS
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    #endif
    #ifdef TEXAS
    volatile unsigned long delay;
    SYSCTL_RCGC2_R |= 0x00000020;
    delay = SYSCTL_RCGC2_R;
    #endif
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

void SYSTICK_delay(uint32_t req_100ms_tick_count) {
    // Set the current global no. of ticks as the starting value
    // This must be done in a critical section
    IntMasterDisable();
    uint32_t starting_value = global_100ms_tick_count;
    IntMasterEnable();
    
    // Then we keep fetching it to compare, again in a critical section
    uint32_t current_value;
    do {
        IntMasterDisable();
        current_value = global_100ms_tick_count;
        IntMasterEnable();
    }
    while (current_value - starting_value < req_100ms_tick_count);
}

void blink_red_task(void) {
    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF);
        SYSTICK_delay(5); // delay 500ms busy-wait
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);
        SYSTICK_delay(5); // delay 500ms busy-wait
    }
}

void blink_blue_task(void) {
    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0xFF);
        SYSTICK_delay(10); // delay 1000ms busy-wait
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
        SYSTICK_delay(10); // delay 1000ms busy-wait
    }
}

int main() {
    IntMasterEnable();
    
    #ifdef TEXAS
    TExaS_Init(SW_PIN_PF40, LED_PIN_PF321);
    #endif
    
    SYSTICK_init();
    PORTF_init();
    
    uint8_t volatile anti_optimization = 42;
    if (anti_optimization) blink_red_task();
    else blink_blue_task();
    
    while(true); // "infinite loop that should never be reached"
}    

/*

Step 4: Run the code in debugging mode and manually switches between the tasks:
1. Run the code in debugging mode.
2. Find the memory address of each of the two tasks with the help of the disassembly window.
3. Put a breakpoint in the Systick handler.
4. Find the return stack from the Systick handler with the help of the memory window and the stack pointer in the register window.
5. Change the value of program counter in the return stack to the address of the blue LED task.
6. Witness what happens.

*/
