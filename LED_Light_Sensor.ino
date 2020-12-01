#define LED_A A1  // +
#define LED_C A0  // -

void setup() {

  // Start out with LED pins as output
  pinMode(LED_A, OUTPUT);
  pinMode(LED_C, OUTPUT);
  
  Serial.begin(115200);
  
}


void loop() {

  int value;

  // Reverse-bias the LED for an instant
  digitalWrite(LED_A, LOW);
  digitalWrite(LED_C, HIGH);

  // Set anode to input
  pinMode(LED_A, INPUT);

  delay(5);

  // Measure discharge
  value = analogRead(LED_A);

  // Forward bias the LED again
  pinMode(LED_A, OUTPUT);
  digitalWrite(LED_A, HIGH);
  digitalWrite(LED_C, LOW);

  // Print measurements
  Serial.println(value);

  delay(5);
  
}
