#ifndef MAIN_H
#define MAIN_H

void enableI2CPower();

void disableI2CPower();

void enterDeepSleep();

void blinkLED(int times, uint32_t color);

void ledOn(uint32_t color);

void ledOff();

void printWakeupReason();

int getBatteryLevel();

#endif