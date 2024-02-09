#include "stm32f0xx.h"
#include "lcd.h"
#include "asteroid.h"

void init_lcd_spi(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x30c30000;
    GPIOB->MODER |= 0x10410000;
    GPIOB->ODR |= 0x4900;

    GPIOB->MODER &= ~0x00000cc0;
    GPIOB->MODER |= 0x00000880;

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR2 = SPI_DataSize_8b;
    SPI1->CR1 |= SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SSM;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void setup_tim7(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 48000-1;
    TIM7->ARR = 250 - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1<<TIM7_IRQn);
    //TIM7->CR1 |= TIM_CR1_CEN;
}

extern Asteroid astro_arr[15];
extern int timer_count = 0;
extern int astro_counter = 0;
int score;


void TIM7_IRQHandler()
{
    TIM7->SR &= ~TIM_SR_UIF;
    timer_count++;
    if (timer_count % 4 == 0 && astro_counter < 15)
    {
        Asteroid new_asteroid = spawn_asteroid();
        astro_arr[astro_counter] = new_asteroid;
        astro_counter++;
    }

    for(int i = 0; i < astro_counter; i++)
    {
        Asteroid* asteroid = &(astro_arr[i]);
        if (asteroid->astro_y < 310)
        {
            if(score <= 30){asteroid->astro_y += 5 + score/2;}
            else{asteroid->astro_y += 20;}
        }
        else
        {
            asteroid->astro_y = 25;
            asteroid->astro_x = (rand() % 211) + 14;
        }
    }
}

void setup_buttons(void)
{
    //enable RCC clock
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    //pc4-7 as outputs
    GPIOC->MODER &= ~0x0000ff00;
    GPIOC->MODER |=  0x00005500;

    //pc4-7 as open-drain
    GPIOC->OTYPER |= 0x00f0;

    //pc0-3 as inputs
    GPIOC->MODER &= ~0x000000ff;

    //pc0-3 pulled high
    GPIOC->PUPDR &= ~0x000000ff;
    GPIOC->PUPDR |=  0x00000055;
}


#include "stm32f0xx.h"
#include <stdint.h>
#include "midi.h"
#include "midiplay.h"
#include "voice.h"

// We'll use the Timer 6 IRQ to recompute samples and feed those
// samples into the DAC.
void TIM6_DAC_IRQHandler(void)
{
    // TODO: Remember to acknowledge the interrupt right here.
    TIM6->SR &= ~TIM_SR_UIF;

    int sample = 0;
    for(int x=0; x < sizeof voice / sizeof voice[0]; x++) {
        if (voice[x].in_use) {
            voice[x].offset += voice[x].step;
            if (voice[x].offset >= N<<16)
                voice[x].offset -= N<<16;
            sample += (wavetable[voice[x].offset>>16] * voice[x].volume) >> 4;
        }
    }
    /*
    if (sound_effect) {
        sample += effect_array[effect_offset];
        effect_offset ++;
        if (effect_offset >= sizeof effect_array) {
            sound_effect = 0;
            effect_offset = 0;
        }
    }*/
    sample = (sample >> 10) + 2048;
    if (sample > 4095)
        sample = 4095;
    else if (sample < 0)
        sample = 0;
    DAC->DHR12R1 = sample;
}

// Initialize the DAC so that it can output analog samples
// on PA4.  Configure it to be triggered by TIM6 TRGO.
void init_dac(void)
{
    // TODO: you fill this in.
    //Enable the RCC clock for the DAC
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    //Disable DAC channel 1
    DAC->CR &= ~DAC_CR_EN1;

    //Select a TIM6 TRGO trigger for the DAC with the MMS field of the CR2 register
    DAC->CR &= ~DAC_CR_TSEL1;

    //Enable the trigger for the DAC
    DAC->CR |= DAC_CR_TEN1;

    //Enable the DAC
    DAC->CR |= DAC_CR_EN1;
}

// Initialize Timer 6 so that it calls TIM6_DAC_IRQHandler
// at exactly RATE times per second.  You'll need to select
// a PSC value and then do some math on the system clock rate
// to determine the value to set for ARR.  Set it to trigger
// the DAC by enabling the Update Tigger in the CR2 MMS field.
void init_tim6(void)
{
    // TODO: you fill this in.
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC = 0;
    TIM6->ARR = 48000000/((TIM6->PSC+1)*RATE)-1;

    TIM6->CR2 &= ~TIM_CR2_MMS;
    TIM6->CR2 |= TIM_CR2_MMS_1;

    TIM6->DIER |= TIM_DIER_UIE;

    TIM6->CR1 |= TIM_CR1_CEN;

    NVIC->ISER[0] = 1<<TIM6_DAC_IRQn;

    NVIC_SetPriority(TIM6_DAC_IRQn, 0);
}

// Find the voice current playing a note, and turn it off.
void note_off(int time, int chan, int key, int velo)
{
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use && voice[n].note == key) {
            voice[n].in_use = 0; // disable it first...
            voice[n].chan = 0;   // ...then clear its values
            voice[n].note = key;
            voice[n].step = step[key];
            return;
        }
    }
}

// Find an unused voice, and use it to play a note.
void note_on(int time, int chan, int key, int velo)
{
    if (velo == 0) {
        note_off(time, chan, key, velo);
        return;
    }
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use == 0) {
            voice[n].note = key;
            voice[n].step = step[key];
            voice[n].offset = 0;
            voice[n].volume = velo;
            voice[n].chan = chan;
            voice[n].in_use = 1;
            return;
        }
    }
}

void set_tempo(int time, int value, const MIDI_Header *hdr)
{
    // This assumes that the TIM2 prescaler divides by 48.
    // It sets the timer to produce an interrupt every N
    // microseconds, where N is the new tempo (value) divided by
    // the number of divisions per beat specified in the MIDI file header.
    TIM2->ARR = value/hdr->divisions - 1;
}


uint8_t notes[] = { 60,62,64,65,67,69,71,72,71,69,67,65,64,62,60,0 };
uint8_t num = sizeof notes / sizeof notes[0] - 1;
void TIM2_IRQHandler(void)
{
    // TODO: remember to acknowledge the interrupt here!
    TIM2->SR &= ~TIM_SR_UIF;
    midi_play();

}
// Configure timer 2 so that it invokes the Update interrupt
// every n microseconds.  To do so, set the prescaler to divide
// by 48.  Then the CNT will count microseconds up to the ARR value.
// Basically ARR = n-1
// Set the ARPE bit in the CR1 so that the timer waits until the next
// update before changing the effective ARR value.
// Call NVIC_SetPriority() to set a low priority for Timer 2 interrupt.
// See the lab 6 text to understand how to do so.
void init_tim2(int n) {
    // TODO: you fill this in.
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 &= ~TIM_CR1_CEN;

    TIM2->PSC = 48-1;
    TIM2->ARR = n-1;

    TIM2->CR1 |= TIM_CR1_ARPE;

    TIM2->DIER |= TIM_DIER_UIE;

    TIM2->CR1 |= TIM_CR1_CEN;

    NVIC->ISER[0] = 1<<TIM2_IRQn;

    NVIC_SetPriority(TIM2_IRQn, 3);
}


void wait_start(void);
int game(void);
void gameover(int, int);
void instructions(void);
extern int highest_score = 0;
extern MIDI_Player* mp;

int main(void)
{
    setup_buttons();
    LCD_Setup(); // this will call init_lcd_spi()
    init_wavetable_hybrid2(); // set up the wavetable
    init_dac();         // initialize the DAC
    init_tim6();        // initialize TIM6
    //MIDI_Player *mp = midi_init(midifile);
    mp = midi_init(midifile);
        // The default rate for a MIDI file is 2 beats per second
        // with 48 ticks per beat.  That's 500000/48 microseconds.  // initialize TIM2
    wait_start();
    instructions();
    init_tim2(10816);
    setup_tim7();

    int score = game(); //game returns player's score as an int
    if (score >= highest_score)
    {
        highest_score = score;
    }
    gameover(score, highest_score); //gameover could take player score as a parameter to display on the screen
}
