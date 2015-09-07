#include "main.h"

void sendToLED (const uint8_t rgb[3])
{
    // RESET: 24us low
    // 0 bit: 0.7us high, 1.8us low, 200ns tolerance
    // 1 bit: 1.8us high, 0.7us low, 200ns tolerance
    
    // 10MHz clock: 1 cycle per 0.1us
    
    register uint8_t p = LED_PORT;
    
    register uint8_t byteCounter = 0;
    register uint8_t bitCounter = 0;
    register uint8_t workingByte = 0;
    
    // send a reset pulse
    clear (LED_PORT, LED_NUM);
    _delay_us (25);
    
    for (uint8_t timerLED = 0; timerLED != NUM_LEDS; timerLED++)
    {
        const uint8_t *x = rgb;
        
        // 10MHz clock:
        //   high bit must be 18 cycles high and 7 cycles low
        //   low bit must be 7 cycles high and 18 cycles low
        // There should be no delay between the RGB bytes an individual LED
        
        __asm__ volatile
        (
         "ldi %[byteCount], 3;         \n" // (1) load 3 into the byte count register
         "ld %[byte], X+;              \n" // (2) load byte from rgb[] and increment pointer
         
         // this runs on the first bit of every byte
         "byteLoop:                    \n"
         "  ldi %[bitCount], 8;        \n" // (1) load 8 into the bit count register
         "  ori %[buffer], %[pinHigh]; \n" // (1) LED output high in temp port buffer
         "  out %[port], %[buffer];    \n" // (1) send port buffer to port (LED HIGH)
         
	     "  sbrs %[byte], 7;           \n" // (1 if false, 2 if true) test MSB of input byte, skip if 1
	     "  rjmp lowBit;               \n" // (2) jump to lowBit if the MSB was 0
	     "  rjmp highBit;              \n" // (2) jump to highBit if the MSB was 1
         
         // this runs for all bits except the first
         "bitLoop:                     \n"
         "  nop;                       \n"
         "  nop;                       \n"
	     "  nop;                       \n" // (3)
         "  ori %[buffer], %[pinHigh]; \n" // (1) LED output high in temp port buffer
         "  out %[port], %[buffer];    \n" // (1) send port buffer to port (LED HIGH)
         
	     "  nop;                       \n" // (1)
	     "  sbrc %[byte], 7;           \n" // (1 if false, 2 if true) test MSB of input byte, skip if 0
	     "  rjmp highBit;              \n" // (2) jump to highBit if the MSB was 1
         
         // -------------------------------------------------------------------
         // at this point, it's been 3 cycles since the LED pin went high
	     "lowBit:                      \n"
	     
	     "  nop;                       \n" // (1)
	     
	     "  andi %[buffer], %[pinLow]; \n" // (1) LED output low in temp port buffer
	     "  cpi %[bitCount], 1;        \n" // (1) compare the bit count with 1 (final bit)
	     "  out %[port], %[buffer];    \n" // (1) send port buffer to port (LED LOW)
	     
	     // while we're waiting, check if this is the final bit
	     // if it is, take this time to load the next byte and decrement the byte counter
	                                       // compare happens before output is sent low, for timing
	     "  brne notLastLow;           \n" // (1 if final bit, 2 if not final bit)
	     "  ld %[byte], X+;            \n" // (2) load byte from rgb[] and increment pointer
	     "  dec %[byteCount];          \n" // (1) subtract one from the byte count
	     "  rjmp lastLow;              \n" // (2) jump to the business end of the low bit code
	     
	     "notLastLow:                  \n"
	     "  lsl %[byte];               \n" // (1) shift the input byte left by one
	     "  nop;                       \n"
	     "  nop;                       \n"
	     "  nop;                       \n" // (3)
	     "lastLow:                     \n"
	     
	     "  nop;                       \n" // (1)
	     "  dec %[bitCount];           \n" // (1) subtract one from the bit count
	     "  cpi %[bitCount], 0;        \n" // (1) compare the bit count with 0 for the loop test
	     "  rjmp loops;                \n" // (2) jump to the loop test to see if this was the last bit
	     
	     // -------------------------------------------------------------------
	     // at this point, it's been 4 cycles since the LED pin went high
	     "highBit:                     \n"
	     
	     // while we're waiting, check if this is the final bit
	     // if it is, take this time to load the next byte and decrement the byte counter
	     "  cpi %[bitCount], 1;        \n" // (1) compare the bit count with 1 (final bit)
	     "  brne notLastHigh;          \n" // (1 if final bit, 2 if not final bit)
	     "  ld %[byte], X+;            \n" // (2) load byte from rgb[] and increment pointer
	     "  dec %[byteCount];          \n" // (1) subtract one from the byte count
	     "  rjmp lastHigh;             \n" // (2) jump to the business end of the high bit code
	     
	     "notLastHigh:                 \n"
	     "  lsl %[byte];               \n" // (1) shift the input byte left by one
	     "  nop;                       \n"
	     "  nop;                       \n"
	     "  nop;                       \n" // (3)
	     
	     "lastHigh:                    \n"
	     "  nop;                       \n"
	     "  nop;                       \n"
	     "  nop;                       \n" // (3)
	     
	     "  dec %[bitCount];           \n" // (1) subtract one from the bit count
	     
	     "  andi %[buffer], %[pinLow]; \n" // (1) LED output low in temp port buffer
	     "  cpi %[bitCount], 0;        \n" // (1) compare the bit count with 0 for the loop test
	     "  out %[port], %[buffer];    \n" // (1) send port buffer to port (LED LOW)
	     
	     // -------------------------------------------------------------------
	     // 11 cycles since the pin went low when coming from lowBit
	     // 0 cycles since the pin went low when coming from highbit
         "loops:                       \n"
	                                       // we compared bitCount with 0 above
	     "  brne bitLoop;              \n" // (1 if end of byte, 2 if jumping to the bit loop again)
	     
	     "  cpi %[byteCount], 0;       \n" // (1) compare the byte count with 0
	     "  brne byteLoop;             \n" // (1 if end of RGB sequence, 2 if jumping to the next byte)
	     
	     
	     // output operands
	     : [X]"+x"(x),
	       [byteCount]"+d"(byteCounter), // byte count must be an upper register
	       [bitCount]"+d"(bitCounter), // bit count must be an upper register
	       [byte]"+d"(workingByte), // input byte must be an upper register
	       [buffer]"=d"(p)  // LED port buffer must be an upper register
	     // input operands
	     : [pinHigh]""((uint8_t)(1 << LED_NUM)), // output pin 1, all other bits 0
	       [pinLow]""((uint8_t)(~(1 << LED_NUM))), // output pin 0, all other bits 1
	       [port]"I"(_SFR_IO_ADDR(LED_PORT)) // LED PORT
	    );
    }
}

