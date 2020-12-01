#define LED_A A1  // +
#define LED_C A0  // -

#define LPF_M 6

#define SIGNAL_THRESHOLD  40
#define SYMBOL_PERIOD     5   // 100Hz / 20Hz = 5
#define SAMPLE_OFFSET     2

#define STX   0x02
#define ETX   0x03

#define DEBUG 0

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

  static uint16_t lpf;
  static boolean start_detected = false;
  static int counter;
  static int bit_pos;
  static char recv_char;
  static boolean packet = false;
  static char recv_buf[256];
  static int recv_buf_pos;

  // Check if the flag was set
  // Should happen at approx. 100Hz
  if (flag) {

    // Low-pass filter
    lpf = lpf - (lpf >> LPF_M) + light_value;

    // Evaluate measurement
    uint16_t lpf_value = lpf >> LPF_M;
    boolean symbol = light_value > lpf_value + SIGNAL_THRESHOLD;

    // Check for start bit
    if (!start_detected && symbol) {

      // Start symbol timing counter
      counter = -SAMPLE_OFFSET - 1;

      // Initialise parser
      start_detected = true;
      bit_pos = 8;
      recv_char = 0;
      
    }

    // Check if we are currently reading a byte
    if (start_detected) {

      // Increment symbol timing counter
      if (++counter == SYMBOL_PERIOD) {
        counter = 0;
      }

      // Check if we are at the sampling point
      if (counter == 0) {

        if (bit_pos == 8) {
          
          // If this is the start bit, it must be set
          if (!symbol) {
            start_detected = false;
            packet = false;
          }
          
        } else if (bit_pos == -1) {

          // Calculate and check parity
          char parity = recv_char ^ (recv_char >> 1);
          parity = parity ^ (parity >> 2);
          parity = parity ^ (parity >> 4);
          if (parity & 1 != !!symbol) {
            start_detected = false;
            packet = false;
          }
          
        } else if (bit_pos == -2) {

          // If this is the stop bit, it must be clear
          if (!symbol) {

            // Check for packet boundaries
            if (recv_char == STX) {

              // Start receiving a packet
              packet = true;
              recv_buf_pos = 0;
              
            } else if (recv_char == ETX) {

              // Terminate the receive buffer and print it
              recv_buf[recv_buf_pos++] = '\0';
              packet = false;
              Serial.print(recv_buf);
              
            } else if (packet) {

              // Append to the receive buffer if there is space
              if (recv_buf_pos < sizeof(recv_buf) - 1) {
                recv_buf[recv_buf_pos++] = recv_char;
              }
              
            }
            
          }

          // Finish receiving character
          start_detected = false;
          
        } else {

          recv_char |= !!symbol << bit_pos;
          
        }

        // Decrement the sampled bit position
        bit_pos--;
        
      }
            
    }

#if DEBUG
    // Print debug data
    Serial.print(lpf_value);
    Serial.print("\t");
    Serial.print(symbol ? 400 : 0);
    Serial.print("\t");
    Serial.print(light_value);
    Serial.print("\t");
    Serial.print((unsigned char) recv_char, HEX);
    Serial.print(":");
    Serial.println(start_detected ? 400 : 0);
#endif

    // Clear flag
    flag = false;

  }

}
