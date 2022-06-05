#include "tm4c123gh6pm.h"
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

#include <stdio.h>
#include <math.h>


#ifndef SENSOR_H_
#define SENSOR_H_

void initSensor(void);
uint32_t readSensor(void);

#endif