#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <task.h>

#define RED GPIO_PIN_1
#define BLUE GPIO_PIN_2
#define GREEN GPIO_PIN_3
#define WHITE (RED|GREEN|BLUE)

bool PREEMPT_SENDER_A = false;
bool PREEMPT_SENDER_B = false;
bool REQUIRE_OTHER_MUTEX = false;
TaskHandle_t SENDER_A_HANDLE = NULL;
TaskHandle_t SENDER_B_HANDLE = NULL;
SemaphoreHandle_t GLOBAL_MUTEX_A = NULL;
SemaphoreHandle_t GLOBAL_MUTEX_B = NULL;
SemaphoreHandle_t GLOBAL_MUTEX_U0 = NULL;
SemaphoreHandle_t DEADLOCK_TRIGGER_SEMAPHORE = NULL;
SemaphoreHandle_t DEADLOCK_RECOVER_SEMAPHORE = NULL;

void DELAY(const int ms); // Helper function to invoke a blocking non-FreeRTOS delay in ms
void SLOW_SEND_TO_UART0(char* str, int delay); // Helper function to simulate a slow I/O operation based on delay

void CONTINUOUS_SENDER_A(void* pvParams); // Continuous task that sends a string to UART0, will take part in deadlock
void CONTINUOUS_SENDER_B(void* pvParams); // Continuous task that sends a string to UART0, will take part in deadlock

void DEADLOCK_TRIGGER_H(void* pvParams); // Handler task that triggers a deadlock in the code upon button push
void DEADLOCK_RECOVER_H(void* pvParams); // Handler task that attempts to recover from deadlock
void DEADLOCK_ISR_INIT(void); // Initialization for the ISR that enables these handler tasks
void DEADLOCK_ISR(void); // ISR selects handler to enable by checking ISR source pin

void UART0_INIT(uint32_t baudrate); // UART0 Initialization
void PORTF_INIT(void); // PORTF Initialization


int main(void) {
    
    PORTF_INIT();
    UART0_INIT(9600);
    DEADLOCK_ISR_INIT();
    GLOBAL_MUTEX_A = xSemaphoreCreateMutex();
    GLOBAL_MUTEX_B = xSemaphoreCreateMutex();
    GLOBAL_MUTEX_U0 = xSemaphoreCreateMutex();
    DEADLOCK_TRIGGER_SEMAPHORE = xSemaphoreCreateBinary();
    DEADLOCK_RECOVER_SEMAPHORE = xSemaphoreCreateBinary();
    if (GLOBAL_MUTEX_A && GLOBAL_MUTEX_B && GLOBAL_MUTEX_U0 && DEADLOCK_TRIGGER_SEMAPHORE && DEADLOCK_RECOVER_SEMAPHORE) {
        xTaskCreate(CONTINUOUS_SENDER_A, "1st Continuous Sender Task", 128, NULL, 1, &SENDER_A_HANDLE);
        xTaskCreate(CONTINUOUS_SENDER_B, "2nd Continuous Sender Task", 128, NULL, 1, &SENDER_B_HANDLE);
        xTaskCreate(DEADLOCK_TRIGGER_H, "Deadlock Triggering Handler Task", 128, NULL, 2, NULL);
        xTaskCreate(DEADLOCK_RECOVER_H, "Deadlock Recovery Handler Task", 128, NULL, 2, NULL);
        vTaskStartScheduler();
    }
    // Unreachable code section, only reachable if something goes wrong...
    GPIOPinWrite(GPIO_PORTF_BASE, WHITE, WHITE);
    while (true);
    
}


void vApplicationIdleHook(void) {
    
    __asm("wfi");
    xSemaphoreTake(GLOBAL_MUTEX_U0, portMAX_DELAY);
    SLOW_SEND_TO_UART0("Seems like no tasks can run, we are in deadlock!", 25);
    if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A))
        SLOW_SEND_TO_UART0(" - CONTINUOUS_SENDER_A is holding GLOBAL_MUTEX_A", 10);
    if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B))
        SLOW_SEND_TO_UART0(" - CONTINUOUS_SENDER_A is holding GLOBAL_MUTEX_B", 10);
    if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A))
        SLOW_SEND_TO_UART0(" - CONTINUOUS_SENDER_B is holding GLOBAL_MUTEX_A", 10);
    if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B))
        SLOW_SEND_TO_UART0(" - CONTINUOUS_SENDER_B is holding GLOBAL_MUTEX_B", 10);
    xSemaphoreGive(GLOBAL_MUTEX_U0);
    
}


void CONTINUOUS_SENDER_A(void* pvParams) {
    
    bool use_other_mutex;
    while (true) {
    
        // Need to store the value because it may change after taskYIELD()
        // And I need it to be the same throughout the run
        use_other_mutex = REQUIRE_OTHER_MUTEX;
        
        // If the deadlock supervisor decided it should preempt its resources
        if (PREEMPT_SENDER_A) {
            // Then do release resources that you have then block and restart
            if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) xSemaphoreGive(GLOBAL_MUTEX_A);
            if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) xSemaphoreGive(GLOBAL_MUTEX_B);
            PREEMPT_SENDER_A = false;
            vTaskDelay(100);
            continue;
        }
        
        // Try and take the mutex
        if (xSemaphoreTake(GLOBAL_MUTEX_A, 0) == pdFALSE) {
            // If it fails because it is taken by someone else block and restart
            if (SENDER_A_HANDLE != xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) {
                vTaskDelay(100);
                continue;
            }
        }
        
        // And, if the deadlock trigger requires the task to take the other mutex
        if (use_other_mutex) {
            // It should attempt to do so
            if (xSemaphoreTake(GLOBAL_MUTEX_B, 0) == pdFALSE) {
                // If it fails because someone else has it, block and restart
                if (SENDER_A_HANDLE != xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) {
                    vTaskDelay(100);
                    continue;
                }
            }
        }
        
        xSemaphoreTake(GLOBAL_MUTEX_U0, portMAX_DELAY);
        GPIOPinWrite(GPIO_PORTF_BASE, BLUE, BLUE); // Start
        SLOW_SEND_TO_UART0("<<< Hello from sender A!", 50);
        GPIOPinWrite(GPIO_PORTF_BASE, BLUE, ~BLUE); // End
        if (xSemaphoreGive(GLOBAL_MUTEX_U0) == pdTRUE)
            taskYIELD(); // Now allow other task to use UART if it wants
        
        if (use_other_mutex) xSemaphoreGive(GLOBAL_MUTEX_B);
        xSemaphoreGive(GLOBAL_MUTEX_A);
    
    }
    
}


void CONTINUOUS_SENDER_B(void* pvParams) {
    
    bool use_other_mutex;
    while(true) {
    
        use_other_mutex = REQUIRE_OTHER_MUTEX;
        
        if (PREEMPT_SENDER_B) {
            if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) xSemaphoreGive(GLOBAL_MUTEX_B);
            if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) xSemaphoreGive(GLOBAL_MUTEX_A);
            PREEMPT_SENDER_B = false;
            vTaskDelay(100);
            continue;
        }
        
        if (xSemaphoreTake(GLOBAL_MUTEX_B, 0) == pdFALSE) {
            if (SENDER_B_HANDLE != xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) {
                vTaskDelay(100);
                continue;
            }
        }
        
        if (use_other_mutex) {
            if (xSemaphoreTake(GLOBAL_MUTEX_A, 0) == pdFALSE) {
                if (SENDER_B_HANDLE != xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) {
                    vTaskDelay(100);
                    continue;
                }
            }
        }
        
        xSemaphoreTake(GLOBAL_MUTEX_U0, portMAX_DELAY);
        GPIOPinWrite(GPIO_PORTF_BASE, GREEN, GREEN); // Start
        SLOW_SEND_TO_UART0(">>> Hello from sender B!", 50);
        GPIOPinWrite(GPIO_PORTF_BASE, GREEN, ~GREEN); // End
        if (xSemaphoreGive(GLOBAL_MUTEX_U0) == pdTRUE)
            taskYIELD(); // Now allow other task to use UART if it wants
        
        if (use_other_mutex) xSemaphoreGive(GLOBAL_MUTEX_A);
        xSemaphoreGive(GLOBAL_MUTEX_B);

    }
    
}


void DEADLOCK_ISR_INIT(void) {
    
    IntPrioritySet(INT_GPIOF_TM4C123, 0xA0);
    GPIOIntRegister(GPIO_PORTF_BASE, DEADLOCK_ISR);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4);
    
}

void DEADLOCK_ISR(void) {
    
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    if (GPIO_INT_PIN_0 & GPIOIntStatus(GPIO_PORTF_BASE, true)) {
        
        xSemaphoreGiveFromISR(DEADLOCK_TRIGGER_SEMAPHORE, &xHigherPriorityTaskWoken);
        GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_0);
        
    } else if (GPIO_INT_PIN_4 & GPIOIntStatus(GPIO_PORTF_BASE, true)) {
        
        xSemaphoreGiveFromISR(DEADLOCK_RECOVER_SEMAPHORE, &xHigherPriorityTaskWoken);
        GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
        
    }
    
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    
}


void DEADLOCK_TRIGGER_H(void* pvParams) {
    
    xSemaphoreTake(DEADLOCK_TRIGGER_SEMAPHORE, 0);
    
    while(true) {
        
        xSemaphoreTake(DEADLOCK_TRIGGER_SEMAPHORE, portMAX_DELAY);
        GPIOPinWrite(GPIO_PORTF_BASE, RED, RED); // Start sensing

        // If you sense a key press
        if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0))) {
            
            // Debounce
            do DELAY(5);
            while((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)));
            
            // Then write to UART
            xSemaphoreTake(GLOBAL_MUTEX_U0, portMAX_DELAY);
            GPIOPinWrite(GPIO_PORTF_BASE, WHITE, RED); // Start writing
            SLOW_SEND_TO_UART0("\n\rNow triggering deadlock situation...\n\r", 25);
            GPIOPinWrite(GPIO_PORTF_BASE, RED, ~RED); // End sensing
            xSemaphoreGive(GLOBAL_MUTEX_U0);
            
            // And finally, trigger deadlock by setting this flag
            REQUIRE_OTHER_MUTEX = true;

        }
        
        GPIOPinWrite(GPIO_PORTF_BASE, RED, ~RED); // End sensing
        
    }
    
}


void DEADLOCK_RECOVER_H(void* pvParams) {
    
    xSemaphoreTake(DEADLOCK_RECOVER_SEMAPHORE, 0);
    
    while(true) {
        
        xSemaphoreTake(DEADLOCK_RECOVER_SEMAPHORE, portMAX_DELAY);
        GPIOPinWrite(GPIO_PORTF_BASE, RED, RED); // Start sensing

        // If you sense a key press
        if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4))) {
            
            // Debounce
            do DELAY(5);
            while((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)));
            
            // Let's simulate processor resource preemption
            if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) PREEMPT_SENDER_A = true;
            else if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_A)) PREEMPT_SENDER_B = true;
            else if (SENDER_A_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) PREEMPT_SENDER_A = true;
            else if (SENDER_B_HANDLE == xSemaphoreGetMutexHolder(GLOBAL_MUTEX_B)) PREEMPT_SENDER_B = true;
            else return; // Should never reach here, this means something wrong happened!
            
            // Then write to UART
            xSemaphoreTake(GLOBAL_MUTEX_U0, portMAX_DELAY);
            GPIOPinWrite(GPIO_PORTF_BASE, WHITE, RED); // Start writing
            SLOW_SEND_TO_UART0("\n\rNow attempting to recover from deadlock situation...\n\r", 25);
            GPIOPinWrite(GPIO_PORTF_BASE, RED, ~RED); // End writing
            xSemaphoreGive(GLOBAL_MUTEX_U0);

        }
        
        GPIOPinWrite(GPIO_PORTF_BASE, RED, ~RED); // End sensing
        
    }
    
}


void SLOW_SEND_TO_UART0(char* str, int delay) {

    while (*str) {
        
        // Wait if there's no more room for transmit
        while (!UARTSpaceAvail(UART0_BASE));
        
        // Wait if not done transmitting yet
        while (UARTBusy(UART0_BASE));
        
        // Place first character in string and increment it simultaneously
        UARTCharPut(UART0_BASE, *(str++));
        
        // Delay for a while to simulate long I/O operation
        DELAY(delay);
        
    }
    
    // And some more delay as if closing I/O device
    DELAY(delay * 4);
    
    // Terminate with a new line and carriage return
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');

}


void DELAY(const int ms) {
    
    for (int j = 0; j < ms; j++)
        for (int i = 0; i < 3180; i++);

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
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTDisable(UART0_BASE);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), baudrate, UART_CONFIG_WLEN_8
                                                                | UART_CONFIG_STOP_ONE
                                                                | UART_CONFIG_PAR_NONE);
    UARTEnable(UART0_BASE);
    
}
