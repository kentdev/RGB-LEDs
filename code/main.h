#ifndef _MAIN_H
#define _MAIN_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

#define NUM_LEDS 30

// linear potentiometer: dimming
#define SLIDE_DDR  DDRA
#define SLIDE_PORT PORTA
#define SLIDE_NUM  0
#define SLIDE_ADC  0
// valid ADC numbers are 0-7

// rotary potentiometer: color adjustment
#define ROT_DDR  DDRA
#define ROT_PORT PORTA
#define ROT_NUM  1
#define ROT_ADC  1

// button: switch between white/color
#define BUTTON_DDR  DDRA
#define BUTTON_PORT PORTA
#define BUTTON_PIN  PINA
#define BUTTON_NUM  2

// output data line to LED strip
#define LED_DDR  DDRA
#define LED_PORT PORTA
#define LED_NUM  7

// bit macros
#define set(reg,bit)     reg |= (1<<(bit))
#define clear(reg,bit)   reg &= ~(1<<(bit))
#define toggle(reg,bit)  reg ^= (1<<(bit))
#define check(reg,bit)   (bool)(reg & (1<<(bit)))

#endif

