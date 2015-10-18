#include "main.h"
#include "sendToLED.h"

//#define DEMO

// HSV to RGB code taken from https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
// index is from 0-767, sat and bright are 0-255
static void hsb2rgbAN2 (uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
    uint8_t temp[5], n = (index >> 8) % 3;
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

#ifdef DEMO
static void colorDemo (void)
{
    uint16_t hue = 0;
    uint8_t saturation = 255;
    uint8_t value = 64;
    int8_t valueDir = -1;
    uint8_t rgb[3];
    uint8_t rgbChain[NUM_LEDS][3];
    
    // initialize all to one color
    hsb2rgbAN2 (hue, saturation, value, rgb);
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        rgbChain[i][0] = rgb[0];
        rgbChain[i][1] = rgb[1];
        rgbChain[i][2] = rgb[2];
    }
    
    for (;;)
    {
	    hue += 20;
        if (hue >= 768)
            hue -= 768;
        
        value += valueDir;
        if (value == 0 || value == 64)
            valueDir = -valueDir;
        
        hsb2rgbAN2 (hue, saturation, value, rgb);
        
        for (uint8_t i = NUM_LEDS - 1; i > 0; i--)
        {
            rgbChain[i][0] = rgbChain[i - 1][0];
            rgbChain[i][1] = rgbChain[i - 1][1];
            rgbChain[i][2] = rgbChain[i - 1][2];
        }
        rgbChain[0][0] = rgb[0];
        rgbChain[0][1] = rgb[1];
        rgbChain[0][2] = rgb[2];
        
        resetLEDs();
        for (uint8_t i = 0; i < NUM_LEDS; i++)
            pushLED (rgbChain[i]);
        
        _delay_ms (50);
    }
}
#endif

static void setLEDs (const bool colorMode, uint16_t slideADC, uint16_t rotADC)
{
    static bool first = true;
    static bool oldColorMode;
    static uint16_t oldSlideADC = 0;
    static uint16_t oldRotADC = 0;
    
    // slideADC goes from 0-1023, but we want 0-255
    const uint8_t slide8 = slideADC / 4;
    
    // if there was a change to the inputs, re-send data to the LEDs
    if (first ||
        oldSlideADC != slideADC ||
        colorMode != oldColorMode ||
        (colorMode && (oldRotADC != rotADC)))
    {
        first = false;
        oldColorMode = colorMode;
        oldSlideADC = slideADC;
        oldRotADC = rotADC;
        
        if (colorMode)
        {
            uint8_t rgb[3];
            
            // potADC is between 0 and 1023, needs to be converted to between 0 and 767
            rotADC *= 3;
            rotADC /= 4;
            
            hsb2rgbAN2 (rotADC, 255, slide8, rgb);
            sendToLEDs (rgb);
        }
        else
        {
            uint8_t rgb[3] = {slide8, slide8, slide8};
            sendToLEDs (rgb);
        }
    }
}

static uint16_t readADC (const uint8_t num)
{
    uint16_t result = 0;
    
    ADMUX = num & 0b00111111;  // set the mux bits (max of ADC7)
    set (ADCSRA, ADSC);  // start a conversion
    while (check (ADCSRA, ADSC));  // wait for the conversion to complete
    
    result = ADCL & 0xff;
    result |= ((ADCH << 8) & 0xff00);
    
    return result;
}

static void lightControls (void)
{
    // initialize state variables
    bool colorMode = false;
    bool buttonPressed = false;
    
    for (;;)
    {
        // check for a button press
        if (!buttonPressed && check (BUTTON_PIN, BUTTON_NUM))
        {  // button has just been pressed
            _delay_ms (1);  // debounce pause
            
            if (check (BUTTON_PIN, BUTTON_NUM))
            {
                buttonPressed = true;
                colorMode = !colorMode;
            }
        }
        else if (buttonPressed && !check (BUTTON_PIN, BUTTON_NUM))
        {  // button was pressed, but has just been released
            buttonPressed = false;
        }

        setLEDs (colorMode, readADC (SLIDE_ADC), readADC (ROT_ADC));
        
        _delay_ms (10);
    }
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
    ADCSRA = (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);  // /128 prescaler for 78.125kHz ADC clock
    DIDR0 = (1 << SLIDE_ADC) | (1 << ROT_ADC);  // disable digital inputs for the slide and pot pins
    
#ifdef DEMO
    colorDemo();
#else
    lightControls();
#endif
    
    return 0;
}

