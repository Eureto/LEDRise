#ifndef DEBUGGING_SHIT_H
#define DEBUGGING_SHIT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void printTimeINFINITELY();
void printTimeTask(void *parameter);

#endif
