/**
 * @device 24xx01 Serial EEPROM
 */
#define SDA_PIN 2
#define CLK_PIN 3

#define BYTE_WRITE_CONTROL_CODE 0xA0
#define BYTE_READ_CONTROL_CODE 0xA1

#define EEPROM_SIZE_BYTES 128
#define EEPROM_PAGE_COUNT 16
#define EEPROM_PAGE_SIZE_BYTES (EEPROM_SIZE_BYTES / EEPROM_PAGE_COUNT)

// Change these flags depending on your use-case
#define DEBUG_ENABLED true  // Whether to print out debug statements on the Serial bus (this slows down the data transfer)
#define DELAYS_ENABLED true  // Whether to include delays between signals. May be necessary to intentionally slow down the signals from higher speed arduinos

const byte BLDC_PATTERN[] = {
  0x88,  // 100 010 00
  0x90,  // 100 100 00
  0x50,  // 010 100 00
  0x44,  // 010 001 00
  0x24,  // 001 001 00
  0x28   // 001 010 00
};
const size_t BLDC_PATTERN_LENGTH = sizeof(BLDC_PATTERN);

uint8_t clkState = LOW;
uint8_t sdaState = LOW;
uint8_t sdaMode = OUTPUT;

void setClkState(uint8_t state) {
  if (clkState == state) {
    return;
  }
  clkState = state;
  digitalWrite(CLK_PIN, state);
  if (DELAYS_ENABLED) delayMicroseconds(1);
}

void setSdaState(uint8_t state) {
  if (sdaState == state && sdaMode == OUTPUT) {
    return;
  }
  if (sdaMode == INPUT) {
    pinMode(SDA_PIN, OUTPUT);
    sdaMode = OUTPUT;
  }
  sdaState = state;
  digitalWrite(SDA_PIN, state);
  if (DELAYS_ENABLED) delayMicroseconds(1);
}

uint8_t readSda() {
  if (sdaMode == OUTPUT) {
    pinMode(SDA_PIN, INPUT);
    sdaMode = INPUT;
  }
  return digitalRead(SDA_PIN);
}

///////////////////////////////////////////////////////////////// SETUP
void setup() {
  pinMode(CLK_PIN, OUTPUT);
  pinMode(SDA_PIN, OUTPUT);
  stopCondition();

  if (DEBUG_ENABLED) Serial.begin(115200);

  readFullEEPROM();
  //writeBLDCStepPattern();
}
///////////////////////////////////////////////////////////////// SETUP

// Unused
void loop() {}

void startCondition() {
  setClkState(HIGH);
  setSdaState(HIGH);
  setSdaState(LOW);
}

// Pulling both the CLK and SDA pins high (in that order) indicates a stop condition
void stopCondition() {
  setClkState(HIGH);
  setSdaState(HIGH);
}

void writeBLDCStepPattern() {
  // These EEPROMS are organized in pages, and do not allow you to write more than one page length at a time
  // This outer loop iterates over all pages in the EEPROM, then writes a byte to each address in every page
  for (uint8_t page = 0; page < EEPROM_PAGE_COUNT; page++) {
    uint8_t pageIndex = page * EEPROM_PAGE_SIZE_BYTES;
    startWrite(pageIndex);
    for (uint8_t i = 0; i < EEPROM_PAGE_SIZE_BYTES; i++) {
      uint8_t byteIndex = pageIndex + i;
      writeSerialByte(BLDC_PATTERN[byteIndex % BLDC_PATTERN_LENGTH]);
    }
    stopCondition();
    commitWrite();
  }
}

// Initialize a write operation at the specified index
void startWrite(uint8_t address) {
  if (DEBUG_ENABLED) {
    Serial.print("Starting page write at address ");
    Serial.println(address);
  }
  startCondition();
  writeSerialByte(BYTE_WRITE_CONTROL_CODE);
  writeSerialByte(address);
  if (DEBUG_ENABLED) {
    Serial.println("----------------------------------------");
  }
}

void commitWrite() {
  bool acknowledged = false;
  if (DEBUG_ENABLED) Serial.println("___Starting Write Cycle___");
  do {
    startCondition();
    acknowledged = writeSerialByte(BYTE_WRITE_CONTROL_CODE);
    Serial.print(acknowledged ? "Done!" : ". ");
  } while (!acknowledged);
}


// Write a byte of data on the serial line, sending most significant bit first
// Returns whether the write was acknowledged
bool writeSerialByte(byte data) {
  setClkState(LOW);
  for (byte bidx = 0x80; bidx > 0; bidx /= 2) {
    if (DEBUG_ENABLED) Serial.print((data & bidx) > 0);
    setSdaState((data & bidx) > 0);
    setClkState(HIGH);
    setClkState(LOW);
  }
  if (DEBUG_ENABLED) {
    Serial.println("");
  }

  // We purposefully set sda low, then throw it into read mode before pulsing the clock high to read the acknowledge bit
  // This is so the device has full control over sda when the clock swings high (mainly so probing this with a scope looks cleaner)
  setSdaState(LOW);
  readSda();

  setClkState(HIGH);
  bool acknowledged = readSda() == LOW;

  // Override sda state to force it low before the next clock pulse
  setSdaState(LOW);

  setClkState(LOW);
  if (DEBUG_ENABLED) Serial.println(acknowledged ? "Acknowledged" : "NOT ACKNOWLEDGED");
  return acknowledged;
}

// Read every address in the EEPROM in order. Print them out if debug is enabled
void readFullEEPROM() {
  startRead(0);
  for (uint8_t i = 0; i < EEPROM_SIZE_BYTES; i++) {
    byte value = readSerialByte(i + 1 < EEPROM_SIZE_BYTES);
    if (DEBUG_ENABLED) {
      Serial.print("Result: ");
      Serial.println(value);
    }
    if (i + 1 < EEPROM_SIZE_BYTES) {
      // Send acknowledge bit after each byte recieved
      // We don't need to send an acknowledge bit after we're finished, hence the logic to skip the last iteration
    }
  }
  stopCondition();
}

// Initialize a read operation at the specified address
void startRead(uint8_t address) {
  // Per the spec sheet, we must populate the address pointer by starting a write operation and supplying the address we would like to read from
  startWrite(address);
  // Instead of actually writing to the EEPROM, we force another "start condition" here,
  // then send the "byte read" control code to start reading from the address that we just stored in the pointer
  startCondition();
  writeSerialByte(BYTE_READ_CONTROL_CODE);
}

byte readSerialByte() {
  return readSerialByte(true);
}

// Read a byte from the serial data line and return it
byte readSerialByte(bool sendAck) {
  byte value = 0x00;
  setClkState(LOW);
  readSda();
  if (DEBUG_ENABLED) Serial.print("Reading Byte: ");
  for (uint8_t i = 0; i < 8; i++) {
    setClkState(HIGH);
    uint8_t bit = readSda();
    value = (value << 1) | bit;
    if (sendAck && i == 7) {
      setSdaState(LOW);
    }
    setClkState(LOW);
    if (DEBUG_ENABLED) Serial.print(bit);
  }
  if (DEBUG_ENABLED) Serial.println("");
  if (sendAck) {
    setClkState(HIGH);
    setClkState(LOW);
  }
  return value;
}