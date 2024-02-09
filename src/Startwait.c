#include "stm32f0xx.h"

#include "lcd.h"

/*static void nano_wait2(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}*/

void nanoWait(unsigned int);

void drive_column2(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows2()
{
    return (~GPIOC->IDR) & 0xf;
}

void wait_start(void)
{
    int start = 0;
    LCD_Clear(0);
    LCD_DrawString(60,121, WHITE, BLACK, "Press *", 20, 0);
    LCD_DrawString(60,140, WHITE, BLACK, "to start", 19, 0);
    for(;;){
            for(int c=0; c<4; c++) {
                drive_column2(c);
                nanoWait(1000000); // wait 1 ms
                int r = read_rows2();
                if (c==3)
                {
                    if (r & 1) { // 'A'
                        start = 1;
                    }
                }
            }
            if(start == 1)
            {
                break;
            }
    }
}

