#include <util/delay.h>
#include <avr/io.h>
#include <usart.h>

void enableSound();

void soundOff();

void soundDelay(int microSecs);

void playTone(float tone, uint32_t duration);
