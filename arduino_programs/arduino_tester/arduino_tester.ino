// This program is supposed to be used with an arduino Mega.
#define DATA_0 2
#define DATA_7 9

#define OUTPUT_ENABLE 52
// Address pins jump increment by 2 for easier connections.
#define ADDRESS_0 22
#define ADDRESS_15 50

#define ADDRESS_CLOCK 18
#define WRITE_CLOCK 19

/*

   PORT mappings for data pins:

    2   PORTE 4
    3   PORTE 5
    4   PORTG 5
    5   PORTE 3
    6   PORTH 3
    7   PORTH 4
    8   PORTH 5
    9   PORTH 6

*/

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
  // TODO: Find a way of making this more readable.
  Serial.print((PINH & 0x40) >> 6);
  Serial.print((PINH & 0x20) >> 5);
  Serial.print((PINH & 0x10) >> 4);
  Serial.print((PINH & 0x08) >> 3);
  Serial.print((PINE & 0x08) >> 3);
  Serial.print((PING & 0x20) >> 5);
  Serial.print((PINE & 0x20) >> 5);
  Serial.print((PINE & 0x10) >> 4);
  Serial.println();
}

void loop() {

}
