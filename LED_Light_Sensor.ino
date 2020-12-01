#define LED_A A1  // +
#define LED_C A0  // -

void setup() {

  // Start out with LED pins as output
  pinMode(LED_A, OUTPUT);
  pinMode(LED_C, OUTPUT);

  // Setup the serial port
  Serial.begin(115200);

  // Configure Timer 0 to interrupt at approx. 200Hz
  cli();                    //stop interrupts
  TCCR0A = 0;               // set entire TCCR0A register to 0
  TCCR0B = 0;               // same for TCCR0B
  TCNT0  = 0;               // initialize counter to 0
  OCR0A  = 77;              // (16MHz/(200Hz*1024))-1 = 77
  TCCR0A |= (1 << WGM01);   // turn on CTC mode
  TCCR0B |= (1 << CS02) | (1 << CS00);    // Select 1024 prescalar
  TIMSK0 |= (1 << OCIE0A);  // enable timer compare interrupt
  sei();                    //allow interrupts

}

volatile boolean flag = false;
volatile uint16_t light_value;

ISR(TIMER0_COMPA_vect) {
  
  static boolean on_phase = false;

  // Alternate between on and off phases
  if (on_phase = !on_phase) {

    // Measure discharge and set flag
    light_value = analogRead(LED_A);
    flag = true;
    
    // Forward bias the LED
    pinMode(LED_A, OUTPUT);
    digitalWrite(LED_A, HIGH);
    digitalWrite(LED_C, LOW);
    
  } else {

    // Reverse-bias the LED for an instant
    digitalWrite(LED_A, LOW);
    digitalWrite(LED_C, HIGH);
  
    // Set anode to input
    pinMode(LED_A, INPUT);
    
  }
  
}

void loop() {

  // Check if the flag was set
  // Should happen at approx. 100Hz
  if (flag) {

    Serial.println(light_value);

    // Clear flag
    flag = false;

  }

}
