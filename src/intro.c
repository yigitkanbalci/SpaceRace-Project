#include "stm32f0xx.h"
#include <math.h>
#include <stdio.h>
#include "asteroid.h"
#include "lcd.h"

void instructions(void)
{
    LCD_Clear(0);
    LCD_DrawString(10, 102, WHITE, BLACK, "Reach the top to score", 19, 0);
    LCD_DrawString(10,121, WHITE, BLACK, "Use 2/4/5/6 to move", 20, 0);
    LCD_DrawString(10,140, WHITE, BLACK, "Use A/B/C to change speed", 19, 0);
    LCD_DrawString(10,159, WHITE, BLACK, "Use # to mute all sounds", 18, 0);

    for(int count = 1000;count <=6000; count++)
    {
        if(count == 1000){LCD_DrawString(110,178, WHITE, BLACK, "5", 19, 0);}
        if(count == 2000){LCD_DrawString(110,178, WHITE, BLACK, "4", 19, 0);}
        if(count == 3000){LCD_DrawString(110,178, WHITE, BLACK, "3", 19, 0);}
        if(count == 4000){LCD_DrawString(110,178, WHITE, BLACK, "2", 19, 0);}
        if(count == 5000){LCD_DrawString(110,178, WHITE, BLACK, "1", 19, 0);}
        nanoWait(1000000); // wait 1 ms
        //init_tim2(10816);
    }
}

void update_score(int score){
    char* score_str[10];
    sprintf(score_str, "%d", score); //copy the score into a string
    LCD_DrawString(165, 0, WHITE, BLACK, "Score: ", 12, 0);
    LCD_DrawString(216, 0, WHITE, BLACK, score_str, 12,0);
}

Asteroid astro_arr[10];
int astro_counter;

void clear_asteroids(void)
{
    for(int i = 0; i < 10; i++)
    {
        Asteroid curr = astro_arr[i];
        erase(curr.astro_x, curr.astro_y);
    }
    astro_counter = 0;
}

extern const Picture crashedShip; // A 19x19 purple ball with white boundaries
void crash_screen(void)
{
    LCD_Clear(0);
    LCD_DrawString(90,50, WHITE,BLACK,"crashed",19,0);
    LCD_DrawPicture(70,150,&crashedShip);
    nanoWait(1000000000);
    TIM2->CR1 &= ~TIM_CR1_CEN;
    //clear_asteroids();
}

void gameover(int score, int highscore)
{
    char* score_str[10];
    sprintf(score_str, "%d", score); //copy the score into a string

    char* highscore_str[10];
    sprintf(highscore_str, "%d", highscore); //copy the score into a string

    LCD_Clear(0);
    LCD_DrawString(80, 120, WHITE, BLACK, "Game Over", 20, 0);
    LCD_DrawString(65, 138, WHITE, BLACK, "Final Score: ", 18, 0);
    LCD_DrawString(175, 138, WHITE, BLACK, score_str, 18, 0); //write score on the screen
    LCD_DrawLine(30, 160, 210, 160, WHITE);
    LCD_DrawString(50, 165, WHITE, BLACK, "Highest Score: ", 18 , 0);
    LCD_DrawString(180, 165, WHITE, BLACK, highscore_str, 18, 0);

    //play again
    int start = 0;
    LCD_DrawString(30,280, WHITE, BLACK, "Press * to play again", 18, 0);
    for(;;){
                for(int c=0; c<4; c++) {
                    drive_column2(c);
                    nanoWait(1000000); // wait 1 ms
                    int r = read_rows2();
                    if (c==3)
                    {
                        if (r & 1) { // '*'
                            start = 1;
                        }
                    }
                }
                if(start == 1)
                {
                    main();
                }
        }
}
