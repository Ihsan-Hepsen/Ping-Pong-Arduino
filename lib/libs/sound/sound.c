#include <util/delay.h>
#include <avr/io.h>
#include <usart.h>

#define BUZZER PD3
#define DO 523.250
#define RE 587.330
#define MI 659.250
#define FA 698.460
#define SOL 783.990
#define LA 880.00
#define SI 987.770
#define DO2 1046.500


void enableSound() {
    DDRD |= (1 << BUZZER);
}

void soundOff() {
    PORTD |= (1 << BUZZER);
}

void soundDelay(int microSecs) {
    while (0 < microSecs) {
        _delay_us(1);
        --microSecs;
    }
}

void playTone(float tone, uint32_t duration) {
    uint32_t microPeriod = (uint32_t) (1000000 / tone);
    uint32_t microDuration = duration * 1000; // micro seconds
    for (uint32_t time = 0; time < microDuration; time += microPeriod) {
        PORTD &= ~(1 << BUZZER);         
        soundDelay(microPeriod / 2); 
        PORTD |= (1 << BUZZER);          
        soundDelay(microPeriod / 2); 
    }
}
