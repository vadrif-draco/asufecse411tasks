#include <stdint.h> 
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/interrupt.h>

// To switch context... Lab 1 Version
// 1. Put a breakpoint here (in the ++),
// 2. See where SP points at once it hits,
// 3. Change 7th register in stack (PC) with the other function's address
//
// To switch context... Lab 2 Version
// 0. Put both stack pointers on watch,
// 1. Put a breakpoint here (in the ++),
// 2. Put in the CPU's SP the SP of either task,
// That's it. Now, when switching to the other one...
// 1. Put a breakpoint here (in the ++),
// 2. Take the CPU's SP and put it in the SP of the task that was running
// 3. Put in the CPU's SP the other task's SP
//
// Note that in Keil the disassembly window gets auto-scrolled to match the code
// This way you could scroll down to the function you want to go to in both windows
static volatile uint32_t global_100ms_tick_count;
void SysTick_Timeout_ISR(void) { global_100ms_tick_count++; }
void SysTick_Handler(void) { SysTick_Timeout_ISR(); }

void SysTick_Init(void) {
    SysTickDisable();
    SysTickIntDisable();
    SysTickIntRegister(SysTick_Timeout_ISR);
    SysTickPeriodSet(100 * 16000 - 1);
    SysTickIntEnable();
    SysTickEnable();
}

void PORTF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

void SysTick_Delay(uint32_t req_100ms_tick_count) {
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

uint32_t blink_red_task_sf[40] = {0}; // Stack Frame for red LED blinking
uint32_t* blink_red_task_sp = &blink_red_task_sf[40]; // Its Stack Pointer
// So now we have a pointer pointing at the end of an array of 40 registers
void blink_red_task(void) {
    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF);
        SysTick_Delay(5); // delay 500ms busy-wait
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);
        SysTick_Delay(5); // delay 500ms busy-wait
    }
}

uint32_t blink_blue_task_sf[40] = {0}; // Stack Frame for blue LED blinking
uint32_t* blink_blue_task_sp = &blink_blue_task_sf[40]; // Its Stack Pointer
void blink_blue_task(void) {
    while (true) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0xFF);
        SysTick_Delay(10); // delay 1000ms busy-wait
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
        SysTick_Delay(10); // delay 1000ms busy-wait
    }
}

void Fabricate_Frame(void (*func)(void), uint32_t** pointer) {
    *(--*pointer) = 1<<24;  // xPSR (bit #24 must be set); Datasheet says...
    /* EPSR Thumb State
    This bit indicates the Thumb state and should always be set.
    Attempting to execute instructions when this bit is clear results in a fault or lockup.
    The value of this bit is only meaningful when accessing PSR or EPSR. */
    *(--*pointer) = (uint32_t)func; // PC
    *(--*pointer) = 0x13; // LR
    *(--*pointer) = 0x12; // R12
    *(--*pointer) = 0x03; // R3
    *(--*pointer) = 0x02; // R2
    *(--*pointer) = 0x01; // R1
    *(--*pointer) = 0x00; // R0
}

int main() {
    IntMasterEnable();
    SysTick_Init();
    PORTF_Init();
    
    Fabricate_Frame(blink_red_task, &blink_red_task_sp);
    Fabricate_Frame(blink_blue_task, &blink_blue_task_sp);
    
    // uint8_t volatile anti_optimization = 42;
    // if (anti_optimization) blink_red_task();
    // else blink_blue_task();
    
    while(true);
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
