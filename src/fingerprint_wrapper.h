// Wrapper for Adafruit Fingerprint library
// to make more easily usable
#include "stm32l432xx.h"
#include "ee14lib.h"
#include "Adafruit_Fingerprint.h"
#include "uart.c"

UART_HandleTypeDef huart1;
Adafruit_Fingerprint finger(&huart1);


void setup_fingerprint(void);
int enroll_fingerprint(uint16_t id);
int match_fingerprint(void);