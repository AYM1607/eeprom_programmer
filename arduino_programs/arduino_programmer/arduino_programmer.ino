// This code is meant to be used on an Arduino Nano or Uno.

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
int lastAddressWritten;
byte lastDataWritten;
bool isFirstWrite = true;

static char printBuff[128];

void program();
void dump(); 
void erase();

void dumpFirts256Bytes() {
  byte data;
  Serial.println("Reading EEPROM");
  for (int addr = 0; addr < 256; addr += 16) {
    sprintf(printBuff, "%04x:", addr);
    Serial.print(printBuff);
    for (int offset = 0; offset < 16; offset++) {
      sprintf(printBuff, " %02x", readEEPROM(addr + offset)); 
      Serial.print(printBuff);
    }
    Serial.println();
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

  Serial.begin(115200);
}

void loop() {
  static byte command;
  // The loop function only process commands.
  while (Serial.available() == 0);

  command = Serial.read();

  switch(command) {
    case 0x00:
    // Program the eeprom.
      program();      
    break;
    case 0x01:
    // Dump the eeprom.
      dump();
    break;
    case 0x02:
    // Erase the EEPROM (fill with 0xFF).
      erase();
    default:
    // Ignore invalid commands.
    break;
  }
}

void latchOutput() {
  static byte latchMask = 0b00010000;

  PORTD &= ~latchMask;
  delayMicroseconds(1);
  PORTD |= latchMask;
  delayMicroseconds(1);
  PORTD  &= ~latchMask;
  delayMicroseconds(1);
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
  static int outputEnableMask = 0x8000;
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

byte readBus(int address) {
  // In contrast with the write function, it's okay to set output enable after setting the
  // pins as inputs. This way, the arduino pins are already ready to sink current when the
  // EEPROM starts driving the bus.
  setAddress(address, true);
  // For some reason, PINB takes a long time to update, a delay of a minimum of 2 microsends
  // Must be added. The datasheet of the AT28C256 specifies a time from address to output of 150ns max
  // and a time from output enabled to output of 70ns max, thus it makes no sense to wait 2000ns, but
  // if it is not done, the most significant nibble is always 0xF.
  delayMicroseconds(2);
  return (PINB << 3) | (PIND >> 5);
}

byte readEEPROM(int address) {
  // If the last operation wasn't a read we need to set our pins as input
  if (lastOp != LAST_OP_READ) {
    setBusMode(READ_MODE);
    // If last op was a write we need to poll for valid data.
    if (lastOp == LAST_OP_WRITE) {
      while (readBus(lastAddressWritten) != lastDataWritten);
    }
  }
  
  lastOp = LAST_OP_READ;
  return readBus(address);
}

void writeEEPROM(int address, byte data, bool pollOnPageChange) {
  // Since we're performing page writes, we must poll the data when we change page.
  if (
    ((address & 0xFFC0) != (lastAddressWritten & 0xFFC0))
    && isFirstWrite == false
    && pollOnPageChange
  ) {
    while(readEEPROM(lastAddressWritten) != lastDataWritten);
  }
  
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
  PORTD = (PORTD & 0b00011111 ) | (data << 5);
  PORTB = (PORTB & 0b11100000) | (data >> 3) ;

  // Slow version.
  //for (int pin = EEPROM_D0; pin < EEPROM_D7; pin++, data >>= 1) {
  //  digitalWrite(pin, data & 1);
  //}
  
  commitWrite();
  lastOp = LAST_OP_WRITE;
  isFirstWrite = false;

  lastAddressWritten = address;
  lastDataWritten = data;
}


void disableSoftwareProtection() {

  writeEEPROM(0x5555, 0xAA, false);
  writeEEPROM(0x2AAA, 0x55, false);
  writeEEPROM(0x5555, 0x80, false);
  writeEEPROM(0x5555, 0xAA, false);
  writeEEPROM(0x2AAA, 0x55, false);
  writeEEPROM(0x5555, 0x20, false);

  delay(10);
}

void erase() {
  for (long addr = 0; addr < 32768; addr++) {
    writeEEPROM(addr, 0xFF, true);
  }
  // Ack the operation.
  Serial.write(0xFF);
}

void program() {
  // For now, the programmer will always write to the whole eeprom.
  byte value;
  // The program will write byte by byte.
  for (long addr = 0; addr < 32768; addr++) {
    // Wait for next value.
    while(Serial.available() == 0);
    value = Serial.read();
    writeEEPROM(addr, value, true);
  }
}

void dump() {
  unsigned long startAddress = 0;
  unsigned long byteCount = 0;
  byte value;
  // Wait and read the starting address to dump.
  while (Serial.available() < 2);
  startAddress |= Serial.read();
  startAddress |= Serial.read() << 8;

  // Wait and read the byte count.
  while (Serial.available() < 2);
  byteCount |= Serial.read();
  byteCount |= Serial.read() << 8;

  for (int address = startAddress; address < startAddress + byteCount; address++ ) {
    value = readEEPROM(address);
    Serial.write(value);
  }
}
