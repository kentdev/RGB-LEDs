#include "main.h"

void sendToLED (uint8_t rgb[3]);

// HSV to RGB code taken from https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
// index is from 0-767, sat and bright are 0-255
static void hsb2rgbAN2 (uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
    uint8_t temp[5], n = (index >> 8);// % 3;
// %3 not needed if input is constrained, but may be useful for color cycling and/or if modulo constant is fast
    uint8_t x = ((((index & 255) * sat) >> 8) * bright) >> 8;
// shifts may be added for added speed and precision at the end if fast 32 bit calculation is available
    uint8_t s = ((256 - sat) * bright) >> 8;
    temp[0] = temp[3] =              s;
    temp[1] = temp[4] =          x + s;
    temp[2] =          bright - x    ;
    color[0] = temp[n + 2];
    color[1] = temp[n + 1];
    color[2] = temp[n    ];
}

int main()
{
    // set the input pins as inputs with no pull-ups
    clear (SLIDE_PORT,  SLIDE_NUM);
    clear (SLIDE_DDR,   SLIDE_NUM);
    clear (ROT_PORT,    ROT_NUM);
    clear (ROT_DDR,     ROT_NUM);
    clear (BUTTON_PORT, BUTTON_NUM);
    clear (BUTTON_DDR,  BUTTON_NUM);
    
    // set the LED output pin as an output, starting out low
    clear (LED_PORT, LED_NUM);
    set   (LED_DDR,  LED_NUM);
    
    // set up the ADC
    ADMUX = 0;  // VCC used as reference, ADC0 selected as current channel
    ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS2);  // /64 prescaler for 156.25kHz ADC clock
    DIDR0 = (1 << ADC0D) | (1 << ADC1D);  // disable digital inputs for ADC0 and ADC1
    
    // initialize state variables
    bool colorMode = false;
    bool oldButtonState = false;
    uint16_t old_slide_adc = 0;
    uint16_t old_pot_adc = 0;
    
    for (;;)
    {
        uint16_t slide_adc;
        uint16_t pot_adc;
        
        bool colorChanged = false;
        
        // check for a button press
        if (check (BUTTON_PIN, BUTTON_NUM))
        {  // button is pressed
            _delay_ms (1);  // debounce pause
            
            if (check (BUTTON_PIN, BUTTON_NUM))
            {
                colorChanged = true;
                colorMode = !colorMode;
                while (check (BUTTON_PIN, BUTTON_NUM));  // wait for the button to be released
            }
        }
        
        // read from the ADC
        ADMUX = SLIDE_ADC;  // set the input to ADC0
        set (ADCSRA, ADSC);  // start a conversion
        while (check (ADCSRA, ADSC));  // wait for the conversion to complete
        slide_adc = ADC;
        
        if (colorMode)
        {
            ADMUX = ROT_ADC;  // set the input to ADC1
            set (ADCSRA, ADSC);  // start a conversion
            while (check (ADCSRA, ADSC));  // wait for the conversion to complete
            pot_adc = ADC;
        }
        
        // if there was a change to the inputs, re-send data to the LEDs
        if (old_slide_adc != slide_adc || colorChanged || (colorMode && (old_pot_adc != pot_adc)))
        {
            uint8_t rgb[3];
            old_slide_adc = slide_adc;
            old_pot_adc = pot_adc;
            
            if (colorMode)
            {
                // pot_adc is between 0 and 1023, needs to be converted to between 0 and 767
                pot_adc *= 3;
                pot_adc /= 4;
                
                hsb2rgbAN2 (pot_adc, 255, (uint8_t)(slide_adc >> 2), rgb);
                sendToLED (rgb);
            }
            else
            {
                const uint8_t val = (uint8_t)(slide_adc >> 2);
                r = g = b = val;
                sendToLED (rgb);
            }
        }
        
    }
    
    return 0;
}

