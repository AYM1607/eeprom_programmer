#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define EEPROM_WE 13

#define LAST_OP_WRITE 0
#define LAST_OP_READ 1

#define READ_MODE true
#define WRITE_MODE false

// Digital pins 0 to 7 correspond to PD0 to PD7
// Digital pins 8 to 13 correspond to PB0 to PB5

byte lastOp;

void latchOutput() {
  static byte latchMask = 0b00010000;

  PORTD &= ~latchMask;
  delayMicroseconds(1);
  PORTD |= latchMask;
  delayMicroseconds(1);
  PORTD  &= ~latchMask;
}

void commitWrite() {
  static byte writeEnableMask = 0b00100000;
  
  delayMicroseconds(1);
  PORTB &= ~writeEnableMask;
  delayMicroseconds(1);
  PORTB |= writeEnableMask;
}

void setBusMode(bool busMode) {
  // When dealing with DDR registers. 1 is output and 0 is input.
  if (busMode == READ_MODE) {
    // When reading we want the pins to be inputs.
    DDRD &= 0b00011111;
    DDRB &= 0b11100000;
  } else {
    // When writing we want the pins to be outputs.
    DDRD |= 0b11100000;
    DDRB |= 0b00011111;
  }
}

void setAddress(int address, bool outputEnable) {
  // Position of the output enable bit withing the data to be shifted out. 
  static byte outputEnableMask = 0x8000;
  // Position of the data bit within PORTD.
  static byte dataMask = 0b00000100;
  // Position of the clock bit within PORTD.
  static byte clockMask = 0b00001000;

  if (outputEnable) {
    // Clear output enable bit.
    address &= ~outputEnableMask;
  } else {
    // Set output enable bit.
    address |= outputEnableMask;
  }
  
  // Make sure the clock pin is low.
  PORTD &= ~clockMask;  

  // Shift the data out LSB first.
  for (uint8_t i = 0; i < 16; i++, address >>= 1) {
    if (address & 1) {
      PORTD |= dataMask;
    } else {
      PORTD &= ~dataMask;
    }

    // Clock the bit out.
    PORTD |= clockMask;
    delayMicroseconds(1);
    PORTD &= ~clockMask;
    delayMicroseconds(1);
  }

  latchOutput();
}

byte readEEPROM(int address) {
  // TODO: Think about adding a delay if the last operation was a write and where it is appropriate to put it.
  // If the last operation wasn't a read we need to set our pins as input.
  if (lastOp != LAST_OP_READ) {
    setBusMode(READ_MODE);
  }

  // In contrast with the write function, it's okay to set output enable after setting the
  // pins as inputs. This way, the arduino pins are already ready to sink current when the
  // EEPROM starts driving the bus.
  byte result = (PINB << 3) | (PIND >> 5) ;

  lastOp = LAST_OP_READ;
  return result;
}


/**
 * Writes a single byte of data to the specified address.
 * If cooldown is true, the function adds a delay to avoid
 * reading incorrect data after a write.
 */
void writeEEPROM(int address, byte data, bool cooldown) {
  // For the write case, must turn output enable off before setting the pins as outputs.
  // If we don't do this, for a brief period of time, both the arduino and the EEPROM would
  // drive the bus and this could case problems.
  setAddress(address, false);
  
  // If the last operation wasn't a write we need to set our pins as output.
  if (lastOp != LAST_OP_WRITE) {
    setBusMode(WRITE_MODE);
  }
  
  // EEPROM_D0 to D2 correspond to PD5 to PD7
  // EEPROM_D3 to D7 correspond to PB0 to PB4
  PORTD &= (data << 5) & 0b00011111;
  PORTB &= (data >> 3) & 0b11100000;

  commitWrite();
  lastOp = LAST_OP_WRITE;
  
  if (cooldown) {
    delay(10);
  }
}


void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  digitalWrite(EEPROM_WE, HIGH);
  pinMode(EEPROM_WE, OUTPUT);
  
  // Setting an invalid value.
  lastOp = -1;

  Serial.begin(9600);
  

}

void loop() {
  // put your main code here, to run repeatedly:

}
