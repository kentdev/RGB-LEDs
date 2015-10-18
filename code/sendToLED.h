#ifndef SEND_TO_LED_H
#define SEND_TO_LED_H

void resetLEDs (void);
void pushLED (const uint8_t rgb[3]);
void sendToLEDs (const uint8_t rgb[3]);

#endif

