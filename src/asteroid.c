#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>

#include "lcd.h"
#include "asteroid.h"

void move_asteroid(Asteroid* astro);
int drawing;
int astro_drawing;

Asteroid spawn_asteroid(void){
    Asteroid new_astro;
    new_astro.astro_x = (rand() % 211) + 14;
    new_astro.astro_y = 25;
    return new_astro;
}

