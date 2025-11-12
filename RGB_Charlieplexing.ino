/* Four RGB LEDs */

// Colour of each LED; hex digits represent GBR
int Buffer[4] = { 0x000, 0x000, 0x000, 0x000 };

// 16 PWM steps × 16 possible brightness levels (0–15)
// For each LED channel (R/G/B), store on/off bit for that step
// bits[LED][channel][step] -> 1 if LED should be on
volatile uint8_t PWM_LUT[4][3][15];

// Setup

void setup() {

  DisplaySetup20k();

  // Quick Test

  const int v[14] = { 0x000F, 0x00F0, 0x0F00, 0x00FF, 0x0FF0, 0x0F0F, 0x0FFF, 0x0008, 0x0080, 0x0800, 0x0088, 0x0880, 0x0808, 0x0888 };

  // Cycle RGB through LEDs 0-3 in order

  for (int i = 0; i < 14; i++) {
    for (int l = 0; l < 4; l++) {
      Buffer[l] = v[i];
    }
    buildPWM_LUT();
    delay(1000);
  }
}

// Display multiplexer

void DisplaySetup20k() {
  // Init timer
  TCCR1 = 1 << CTC1 | 5 << CS10;  // CTC mode; divide by 16
  OCR1C = 24;                     // Divide by 25 -> 20kHz
  TIMSK = TIMSK | 1 << OCIE1A;    // Enable overflow interrupt
}

void buildPWM_LUT() {
    for (int led = 0; led < 4; led++) {
        for (int ch = 0; ch < 3; ch++) {
            int colour = (Buffer[led] >> (ch*4)) & 0x0F;
            for (int step = 0; step < 15; step++) {
                // Inverted logic for common-anode! -> 0 is ON, 1 is OFF
                PWM_LUT[led][ch][step] = (step < colour) ? 1 : 0;
            }
        }
    }
}

// Timer/Counter1 interrupt - multiplexes display
ISR(TIMER1_COMPA_vect) {
  static uint8_t led = 0;
  static uint8_t pwmStep = 0;

  // Lookup each channel from LUT
  uint8_t r = PWM_LUT[led][0][pwmStep];
  uint8_t g = PWM_LUT[led][1][pwmStep];
  uint8_t b = PWM_LUT[led][2][pwmStep];

  // Combine into bitmask for PORTB / DDRB - Cathods LOW 
  uint8_t bits = (r << 0) | (g << 1) | (b << 2) ;

  // Slide bits up to make space for the anode pin for each LED
  bits += (bits & (0b111<<led));  

  DDRB  = (DDRB & 0xF0) | bits | (1 << led); // Set to output (1)
  PORTB = (PORTB & 0xF0) | (bits ^ 0x0F); // Set output state to source (1) or sink (0)

  // Next LED
  ++led;

  // Increment PWM step once all LEDs done
  if (led == 4) {
    led = 0;
    ++pwmStep;
    if (pwmStep >= 15)
      pwmStep = 0;
  }
}

// Light show demo **********************************************

int red(int x) {
  int y = x % 48;
  if (y > 15) y = 31 - y;
  return max(y, 0);
}

int green(int x) {
  return red(x + 32);
}

int blue(int x) {
  return red(x + 16);
}

void loop() {
  static int Step = 0;
  // Rainbow : Total duration 50 * 48 = 2400 ms.
  for (int i = 0; i < 4; i++) {
    int phase = Step + i*12;
    Buffer[i] = blue(phase) << 8 | green(phase) << 4 | red(phase);
  }
  buildPWM_LUT();
  delay(50);

  Step++;
  if (Step >= 48)
    Step -= 48;
}
