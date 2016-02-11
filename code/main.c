#include "main.h"
#include "sendToLED.h"

#define SLIDE_DELTA 3
#define ROT_DELTA 3

//-----------------------------------------------------------------------------

static uint16_t slideADC = 0;
static uint16_t rotADC = 0;

static uint8_t mux = 0;

static void initADC (void)
{
    // set up the ADC
    DIDR0 = (1 << SLIDE_ADC) | (1 << ROT_ADC);  // disable digital inputs for the slide and pot pins
    ADMUX = 0;  // VCC used as reference, ADC0 selected as current channel
    ADCSRA = (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2) | (1 << ADIE);  // /128 prescaler for 78.125kHz ADC clock, interrupt enabled
    
    mux = 0;
    ADCSRA |= (1 << ADEN);  // enable ADC
    ADCSRA |= (1 << ADSC);  // start the first conversion
}

ISR (ADC_vect)
{
    if (mux == 0)
    {
        slideADC = ADC;
        mux = 1;
    }
    else
    {
        rotADC = ADC;
        mux = 0;
    }
    
    ADMUX = mux;  // switch to the other ADC input
    ADCSRA |= (1 << ADSC);  // start a new conversion
}

static uint16_t readADC (uint8_t ch)
{
    uint16_t result;
    
    cli();
    if (ch == 0)
        result = slideADC;
    else
        result = rotADC;
    sei();
    
    return result;
}

//-----------------------------------------------------------------------------

static uint8_t scale (const uint8_t in, const uint8_t numerator, const uint8_t denominator)
{
    uint16_t x = in;
    x *= (uint16_t)numerator;
    x /= (uint16_t)denominator;
    
    if (x > 255)
        x = 255;
    
    return (uint8_t)x;
}

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
    
    // Color balance for a better white:
    // Ideally, I'd actually adjust the color temperature and then convert to RGB from there, but
    // doing that is very slow (involves logarithms), so I'll just wing it.
    color[0] = scale (color[0], 7, 10);  // scale down red intensity to 70%
    color[1] = scale (color[1], 1, 1);  // keep green as-is
    color[2] = scale (color[2], 9, 10);  // scale down blue slightly
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

static void setLEDs (const bool colorMode, const uint16_t slideADC, const uint16_t rotADC)
{
    const uint16_t DEAD_BAND = 48;
    
    // slideADC goes from 0-1023, but we want to ignore a dead band near 0 and scale from 0-255
    uint8_t scaledSlide = 0;
    if (slideADC > DEAD_BAND)
    {
        const uint32_t bandedSlide = slideADC - DEAD_BAND;
        const uint32_t origRange = 1023 - DEAD_BAND;
        const uint32_t destRange = 255;
        scaledSlide = (uint8_t)(bandedSlide * destRange / origRange);
    }
    
    // rotADC is between 0 and 1023, needs to be converted to between 0 and 767
    const uint16_t scaledRotary = (rotADC * 3) / 4;
    
    if (colorMode)
    {
        uint8_t rgb[3];
        
        hsb2rgbAN2 (scaledRotary, 255, scaledSlide, rgb);
        sendToLEDs (rgb);
    }
    else
    {
        uint8_t rgb[3];
        
        hsb2rgbAN2 (0, 0, scaledSlide, rgb);
        sendToLEDs (rgb);
    }
}

/*static uint16_t lowPassSlide (const uint16_t in)
{
    static int16_t prev = 0;
    const int16_t alpha = 50; // out of 100
    
    int16_t new = prev + (alpha * ((int16_t)in - prev) / 100);
    if (new < 0)
        new = 0;
    
    prev = new;
    return (uint16_t)new;
}*/

static void lightControls (void)
{
    // initialize state variables
    static bool colorMode = false;
    static bool buttonWasDown[2] = {false, false};
    
    static int32_t timeout = 0;  // counts the milliseconds since the LED values were last adjusted, negative if timeout has registered
    
    uint16_t oldSlideValue = readADC (SLIDE_ADC);
    uint16_t oldRotaryValue = readADC (ROT_ADC);
    
    for (;;)
    {
        toggle (PORTA, 6);
        
        const uint16_t slideValue = readADC (SLIDE_ADC);
        const uint16_t rotaryValue = readADC (ROT_ADC);
        
        // check for a button press event
        // button is registered as pressed if down for at least 2 loops (20ms), then up
        const bool buttonIsDown = check (BUTTON_PIN, BUTTON_NUM);
        const bool buttonEvent = (buttonWasDown[0] && buttonWasDown[1] && !buttonIsDown);
        
        buttonWasDown[1] = buttonWasDown[0];
        buttonWasDown[0] = buttonIsDown;
        
        // toggle color mode on button press event
        if (buttonEvent)
            colorMode = !colorMode;
        
        const bool slidePositionChanged = ((slideValue > oldSlideValue) ?
                                                (slideValue - oldSlideValue > SLIDE_DELTA) :
                                                (oldSlideValue - slideValue > SLIDE_DELTA));
        
        const bool rotaryPositionChanged = ((rotaryValue > oldRotaryValue) ?
                                                (rotaryValue - oldRotaryValue > ROT_DELTA) :
                                                (oldRotaryValue - rotaryValue > ROT_DELTA));
        
        if (slidePositionChanged)
            oldSlideValue = slideValue;
        
        if (rotaryPositionChanged)
            oldRotaryValue = rotaryValue;
        
        if (buttonEvent || slidePositionChanged || rotaryPositionChanged)
        {  // timeout reset
            timeout = 0;
            setLEDs (colorMode, slideValue, rotaryValue);
        }
        else if (timeout > (uint32_t)1000 * (uint32_t)3600 * (uint32_t)2)
        {  // timeout event
            const uint8_t zeroRGB[3] = {0, 0, 0};
            sendToLEDs (zeroRGB);
        }
        else if (timeout < 0)
        {  // timeout event has already happened, waiting for timeout reset
            const uint8_t zeroRGB[3] = {0, 0, 0};
            cli();
            pushLED (zeroRGB);  // slowly send zeros to ensure the LEDs are off
            sei();
        }
        else
        {  // no timeout, but also no timeout reset
            setLEDs (colorMode, slideValue, rotaryValue);
            timeout += 10;
        }
        
        // TODO: Implement low-power mode
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
    
    // debug port (toggles every 10ms as long as the microcontroller is working)
    set (DDRA, 6);
    clear(PORTA, 6);
    
    // set the LED output pin as an output, starting out low
    clear (LED_PORT, LED_NUM);
    set   (LED_DDR,  LED_NUM);
    
    initADC();
    
    sei();
    
#ifdef DEMO
    colorDemo();
#else
    lightControls();
#endif
    
    for (;;);  // should never reach here
    
    return 0;
}

