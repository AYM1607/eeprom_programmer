// This program is supposed to be used with an arduino Mega.
#define DATA_0 2
#define DATA_7 9

#define OUTPUT_ENABLE 52
// Address pins jump increment by 2 for easier connections.
#define ADDRESS_0 22
#define ADDRESS_15 50

#define ADDRESS_CLOCK 18
#define WRITE_CLOCK 19

void setup() {
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(ADDRESS_CLOCK), onAddressClock, RISING);
  attachInterrupt(digitalPinToInterrupt(WRITE_CLOCK), onWriteClock, FALLING);
}

void onAddressClock() {
  delayMicroseconds(1);
  Serial.print("Address: 0b");
  Serial.print(digitalRead(OUTPUT_ENABLE));
  for (int pin = ADDRESS_15; pin >= ADDRESS_0; pin -= 2) {
    Serial.print(digitalRead(pin));
  }
  Serial.println();
}

void onWriteClock() {
  Serial.print("Data: 0b"); 
  for (int pin = DATA_7; pin >= DATA_0; pin--) {
    Serial.print(digitalRead(pin));
  }
  Serial.println();
}

void loop() {

}
