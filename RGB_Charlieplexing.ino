/* Four RGB LEDs */

// Colour of each LED; hex digits represent GBR
int rgbBuffer[4] = {0x000, 0x000, 0x000, 0x000};

// 16 PWM steps × 16 possible brightness levels (0–15)
// For each LED channel (R/G/B), store on/off bit for that step
volatile uint16_t pwmLut[4][15];

// Setup

void setup()
{

    displaySetup20k();

    // Quick Test

    const int v[14] = {0x000F, 0x00F0, 0x0F00, 0x00FF, 0x0FF0, 0x0F0F, 0x0FFF, 0x0008, 0x0080, 0x0800, 0x0088, 0x0880, 0x0808, 0x0888};

    // Cycle RGB through LEDs 0-3 in order

    for (int i = 0; i < 14; i++)
    {
        for (int l = 0; l < 4; l++)
        {
            rgbBuffer[l] = v[i];
            buildPWM_LUT();
            delay(250);
            rgbBuffer[l] = 0;
        }
    }
}

// Initialize Timer1 for 20kHz interrupts
void displaySetup20k()
{
    // Init timer
    TCCR1 = 1 << CTC1 | 0b0101 << CS10; // CTC mode; divide by 16 -> 16MHz / 16 = 1MHz
    OCR1C = 49;                         // Divide 1MHz by 50 -> 20kHz
    TIMSK = TIMSK | 1 << OCIE1A;        // Enable overflow interrupt
}

// Pre-computer DDRB and PORTB bits, and build PWM lookup table from RGB buffer values
void buildPWM_LUT()
{
    const uint8_t ddrb_top = DDRB & 0xF0;
    const uint8_t portb_top = PORTB & 0xF0;

    for (int led = 0; led < 4; ++led)
    {
        // r,g,b each have a 4-bit brightness value (0-15)
        uint8_t r = (rgbBuffer[led] >> 0) & 0x0F;
        uint8_t g = (rgbBuffer[led] >> 4) & 0x0F;
        uint8_t b = (rgbBuffer[led] >> 8) & 0x0F;

        // Inverted logic for common-anode! -> 0 is ON, 1 is OFF
        // NB. 15 indexes, 0-14. Each step turns on any LED channel with brightness > step
        for (size_t step = 0; step < 15; ++step)
        {
            uint8_t portb = 0;
            uint8_t ddrb = 0;
            uint8_t bits = (((r > step) ? 1 : 0) << 0) |
                           (((g > step) ? 1 : 0) << 1) |
                           (((b > step) ? 1 : 0) << 2);
            bits += (bits & (0b111 << led));       // Slide bits up to make space for the anode pin for each LED
            ddrb = ddrb_top | (bits | (1 << led)); // Set anode pin to OUTPUT, cathod pins to OUTPUT if ON
            portb = portb_top | (bits ^ 0x0F);     // Anode pin set to HIGH (source), cathod pins to LOW (sink) if ON.

            // Store in LUT
            pwmLut[led][step] = ddrb | (portb << 8);
        }
    }
}

// Timer/Counter1 ISR - multiplexes display - execution duration ~1us
ISR(TIMER1_COMPA_vect)
{
    static uint8_t led = 0;
    static uint8_t pwmStep = 0;

    uint16_t data = pwmLut[led][pwmStep];

    DDRB = data & 0xFF;         // Set to output (1)
    PORTB = (data >> 8) & 0xFF; // Set output state to source (1) or sink (0)

    // Increment PWM step once all LEDs done
    if (++led == 4)
    {
        led = 0;
        if (++pwmStep >= 15)
            pwmStep = 0;
    }
}

// Light show demo **********************************************

int red(int x)
{
    int y = x % 48;
    if (y > 15)
        y = 31 - y;
    return max(y, 0);
}

int green(int x)
{
    return red(x + 32);
}

int blue(int x)
{
    return red(x + 16);
}

void loop()
{
    static int Step = 0;
    // Rainbow : Total duration 50 * 48 = 2400 ms.
    for (int i = 0; i < 4; i++)
    {
        int phase = Step + i * 12;
        rgbBuffer[i] = blue(phase) << 8 | green(phase) << 4 | red(phase);
    }
    buildPWM_LUT();
    delay(50);

    Step++;
    if (Step >= 48)
        Step -= 48;
}
