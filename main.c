#include <stdio.h>
#include "ee14lib.h"

unsigned delay();

int main() {
    // Note to future self: increase duty => dimmer, pwm controlling how long 
    // it's grounded bc you're always receiving power

    timer_config_pwm(TIM2, 50);

    // Initialize duty cycle values and increment step rates
    volatile int duty = 51;
    volatile int step = 1;

    while (1) {
        // Update duty cycles
        // gpio_config_mode(A7, OUTPUT);
        // gpio_config_otype(A7, OPEN_DRAIN);
        timer_config_channel_pwm(TIM2, A7, duty);

        // Vary them for the next loop
        duty += step;

        // If duty cycle reaches 1023 (max), start subtracting the step
        // Or if duty cycle reaches 0 (min), start adding the step again
        // Otherwise continue adding/subtracting as you were
        if (duty >= 102)
            step = -1;
        else if (duty <= 51) 
            step = 1;

        // Slow down so we can see the color changes
        delay();
    }
}

// delays any function occurring
unsigned delay() {
    volatile int i = 0;
    // add zeroes to x for i < x to make it slower
    while (i < 10000) {
        i++;
    }
    return 1;
 }
 