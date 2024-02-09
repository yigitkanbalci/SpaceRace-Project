#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "asteroid.h"
#include "voice.h"
#include "midi.h"
#include "lcd.h"

#define VOICES 15

void nanoWait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void drive_column(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows()
{
    return (~GPIOC->IDR) & 0xf;
}

Asteroid astro_arr[15];
void crash_screen(void);
void clear_asteroids(void);
const uint8_t midifile2[];
const uint8_t midifile[];
int astro_counter;
MIDI_Player* mp;
extern int score; //for points

int game(void)
{
    //BACKLOG - TODO
    // 1) Collision detection for every asteroid on the game scene --DONE
    // 2) spaceship background leaving trace --DONE
    // 3) asteroids do not disappear at the bottom of the screen --DONE
    // 4) draw all the asteroids within the game loop and not the ISR, this is what is causing the crash --DONE
    // 5) background music and crash sounds after collision --DONE
    // 6) must erase all asteroid on the screen once a score of game is completed --DONE
    // 7) OPTIMIZATION
    // 8) loop music --Done

    // Draw the space.
    //init_tim2(10816);
    nanoWait(1000000000);
    LCD_DrawPicture(0,0,&space);
    int x = 119;
    int y = 304;
    int speed = 3;
    score = 0;
    update_ship(x,y); //spawn
    TIM7->CR1 |= TIM_CR1_CEN;

    int gamenoise = 1;


    if (score == 0)
    {
        update_score(score);
    }
    for(;;)
        for(int c=0; c<4; c++) {
            int dx = 0;
            int dy = 0;

            drive_column(c);
            nanoWait(1000000); // wait 1 ms
            int r = read_rows();
            if (c==0)
            {
                if (r & 8) { // 'A'
                    speed = 7;
                }
                if (r & 4) { // 'B'
                    speed = 9;
                }
                if (r & 2) { // 'C'
                    speed = 11;
                }
            }

            if (c==3) { // leftmost column

                if (r & 4) { // '4'
                    dx -= 1;
                }

            } else if (c == 2) { // column 2
                if (r & 8) { // '2'
                    dy -= 1;
                }
                if (r & 4) { // '5'
                    dy += 1;
                }
            } else if (c == 1) { // column 3

                if (r & 4) { // '6'
                    dx += 1;
                }
                if (r & 1) { // '#'
                    DAC->CR ^= DAC_CR_EN1;
                }

            }


            if (dx !=0 || dy != 0) {
                int tempx = x + speed*dx;
                int tempy = y + speed*dy;
                if(tempx < 9 || tempx > 230) //out of bounds on x-axis
                {
                    y += speed*dy;
                }
                else if (tempy > 310) //out of bounds on bottom of screen
                {
                    x += speed*dx;
                }
                else
                {
                    x += speed*dx;
                    y += speed*dy;
                }

                if (y < 30) //if ball reaches top
                {
                    erase(x,y);
                    y = 310;
                    update_ship(x,y);
                    score++; //to update score
                    update_score(score);
                }
                else
                {
                    update_ship(x,y); //update normally
                }
            }

            for (int j = 0; j < astro_counter; j++)
            {
                Asteroid* asteroid = &(astro_arr[j]);

                if(score < 16){update_ast(asteroid->astro_x, asteroid->astro_y, 5 + score/2);}
                else if(score >= 16 && score < 26){update_astfast(asteroid->astro_x, asteroid->astro_y, 8 + score/3);}
                else if(score >= 26){update_astfaster(asteroid->astro_x, asteroid->astro_y, 9 + score/3);}

                if(asteroid->astro_y > 305)
                {
                    erase(asteroid->astro_x, asteroid->astro_y);
                }
                else if((asteroid->astro_y <= y + 12 && asteroid->astro_y >= y - 15) && (asteroid->astro_x <= x + 8 && asteroid->astro_x >= x - 15))
                {
                    TIM7->CR1 &= ~TIM_CR1_CEN;
                    TIM2->CR1 &= ~TIM_CR1_CEN;
                    for(int i = 0; i < VOICES; i++)
                    {
                        voice[i].in_use = 0;
                        voice[i].chan = 0;
                        voice[i].note = 0;
                        voice[i].volume = 0;
                        voice[i].step = 0;
                        voice[i].offset = 0;
                    }

                    MIDI_Player *mp2 = midi_init(midifile2);
                    TIM2->CR1 |= TIM_CR1_CEN;
                    crash_screen();
                    nanoWait(100000);
                    clear_asteroids();

                    return score;
                }
            }
            if (mp->nexttick >= MAXTICKS)
                midi_init(midifile);
        }
}
