#include "main.h"

static uint8_t i;
static uint8_t p;

// Send b out the LED pin, going from MSB to LSB,
// with high bits represented as 1.8us high, 0.7us low
// and low bits represented as 0.7us high, 1.8us low
static void sendByte (uint8_t b)
{
    // 10MHz clock:
    //   high bit must be 18 cycles high and 7 cycles low
    //   low bit must be 7 cycles high and 18 cycles low
    
    __asm__ volatile
        (
         "  ldi %0, 8;               \n" // (1) load 8 into the bit count register
         
         "bitLoop:                   \n"
         "  ori %2, %[pinHigh];      \n" // (1) LED output high in temp port buffer
         "  out %[port], %2;         \n" // (1) send port buffer to port (LED HIGH)
         
	     "  sbrc %1, 7;              \n" // (1 if false, 2 if true) test MSB of input byte, skip if 0
	     "  rjmp highBit;            \n" // (2) jump to highBit if the the MSB of input byte was 1
         
         // at this point, it's been 3 cycles since the LED pin went high
	     "lowBit:                    \n"
	     
	     "  nop;                     \n"
	     "  nop;                     \n" // (2)
	     
	     "  andi %2, %[pinLow];      \n" // (1) LED output low in temp port buffer
	     "  out %[port], %2;         \n" // (1) send port buffer to port (LED LOW)
	     
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n" // (5)
	     
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n" // (4)
	     
	     "  lsl %1;                  \n" // (1) shift the input byte left by one
	     "  subi %0, 1;              \n" // (1) subtract one from the bit count
	     
	     "  rjmp loopTest;           \n" // (2) jump to the loop test to see if this was the last bit
	     
	     // at this point, it's been 4 cycles since the LED pin went high
	     "highBit:                   \n"
	     
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n" // (5)
	     
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n"
	     "  nop;                     \n" // (5)
	     
	     "  lsl %1;                  \n" // (1) shift the input byte left by one
	     "  subi %0, 1;              \n" // (1) subtract one from the bit count
	     
	     "  andi %2, %[pinLow];      \n" // (2) LED output low in temp port buffer
	     "  out %[port], %2;         \n" // (1) send port buffer to port (LED LOW)
	     
	     "  nop;                     \n"
	     "  nop;                     \n" // (2)
	     
	     // 13 cycles since the pin went low when coming from lowBit
	     // 2 cycle since the pin went low when coming from highbit
         "loopTest:                  \n"
	     "  cpi %0, 0;               \n" // (1) compare the bit count with 0
	     "  brne bitLoop;            \n" // (1 if end of loop, 2 if jumping to the top again)
	     
	     // output operands
	     : "+r"(i), // bit count register can be any register
	       "+r"(b), // input byte can also be any register
	       "=r"(p)  // LED port buffer
	     // input operands
	     : [pinHigh]""((uint8_t)(1 << LED_NUM)), // output pin 1, all other bits 0
	       [pinLow]""((uint8_t)(~(1 << LED_NUM))), // output pin 0, all other bits 1
	       [port]"I"(_SFR_IO_ADDR(LED_PORT)) // LED PORT
	    );
}

void sendToLED (const uint8_t rgb[3])
{
    // RESET: 24us low
    // 0 bit: 0.7us high, 1.8us low, 200ns tolerance
    // 1 bit: 1.8us high, 0.7us low, 200ns tolerance
    
    // 10MHz clock: 1 cycle per 0.1us
    
    p = LED_PORT;
    
    // send a reset pulse
    clear (LED_PORT, LED_NUM);
    _delay_us (25);
    
    for (uint8_t timerLED = 0; timerLED != NUM_LEDS; timerLED++)
    {
        sendByte (rgb[0]);
        sendByte (rgb[1]);
        sendByte (rgb[2]);
    }
}

