/**
 * This sketch is meant to write to the 24xx01 EEPROM using a serial interface
 */
#define SDA_PIN 2
#define CLK_PIN 3

#define BYTE_WRITE_CONTROL_CODE 0xA0
#define BYTE_READ_CONTROL_CODE 0xA1

#define EEPROM_SIZE_BYTES 128
#define EEPROM_PAGE_COUNT 16
#define EEPROM_PAGE_SIZE_BYTES (EEPROM_SIZE_BYTES / EEPROM_PAGE_COUNT)

// Change these flags depending on your use-case
#define DEBUG_ENABLED true   // Whether to print out debug statements on the Serial bus (this slows down the data transfer)
#define DELAYS_ENABLED true  // Whether to include delays between signals. May be necessary to intentionally slow down the signals from higher speed arduinos

void setup() {
  Serial.begin(115200);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(SDA_PIN, OUTPUT);

  // Per spec, "Bus Not Busy" state is where both data and clock lines remain high
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(SDA_PIN, HIGH);
  delayMicroseconds(1);

  readFullEEPROM();
  //writeFullEEPROM();
}

// Unused
void loop() {}

// Read every address in the EEPROM in order. Print them out if debug is enabled
void readFullEEPROM() {
  startRead();
  for (uint8_t i = 0; i < EEPROM_SIZE_BYTES; i++) {
    byte value = readSerialByte();
    if (DEBUG_ENABLED) {
      Serial.print("Result: ");
      Serial.println(value);
    }
    if (i + 1 < EEPROM_SIZE_BYTES) {
      acknowledge();
    }
  }
  stopDataTransfer();
}

// Write data to every address in the EEPROM.
// Right now, this just writes the address number to that addresses value
void writeFullEEPROM() {
  // These EEPROMS are organized in pages, and do not allow you to write more than one page length at a time
  // This outer loop iterates over all pages in the EEPROM, then writes a byte to each address in every page
  for (uint8_t page = 0; page < EEPROM_PAGE_COUNT; page++) {
    uint8_t pageIndex = page * EEPROM_PAGE_SIZE_BYTES;
    startWrite(pageIndex);
    for (uint8_t i = 0; i < EEPROM_PAGE_SIZE_BYTES; i++) {
      writeSerialByte(pageIndex + i);
    }
    stopDataTransfer();
  }
}

// Initialize a read operation at the specified address
void startRead(uint8_t address) {
  // Per the spec sheet, we must populate the address pointer by starting a write operation and supplying the address we would like to read from
  startWrite(address);
  // Instead of actually writing to the EEPROM, we force another "start condition" here,
  // then send the "byte read" control code to start reading from the address that we just stored in the pointer
  digitalWrite(SDA_PIN, HIGH);
  digitalWrite(CLK_PIN, HIGH);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  digitalWrite(SDA_PIN, LOW);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  writeSerialByte(BYTE_READ_CONTROL_CODE);
}

// Initialize a write operation at the specified index
void startWrite(uint8_t address) {
  if (DEBUG_ENABLED) {
    Serial.print("Starting page write at address ");
    Serial.println(address);
  }
  digitalWrite(SDA_PIN, LOW);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  writeSerialByte(BYTE_WRITE_CONTROL_CODE);
  writeSerialByte(address);
}

// Pulling both the CLK and SDA pins high (in that order) indicates a stop condition
void stopDataTransfer() {
  if (DELAYS_ENABLED) delayMicroseconds(1);
  digitalWrite(CLK_PIN, HIGH);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  digitalWrite(SDA_PIN, HIGH);
  if (DELAYS_ENABLED) delayMicroseconds(1);
}

// Read a byte from the serial data line and return it
byte readSerialByte() {
  byte value = 0x00;
  digitalWrite(CLK_PIN, LOW);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  pinMode(SDA_PIN, INPUT);
  if (DEBUG_ENABLED) Serial.print("Reading Byte: ");
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(CLK_PIN, HIGH);
    uint8_t bit = digitalRead(SDA_PIN);
    if (DEBUG_ENABLED) Serial.print(bit);
    value = (value << 1) | bit;
    digitalWrite(CLK_PIN, LOW);
    if (DELAYS_ENABLED) delayMicroseconds(1);
  }
  if (DEBUG_ENABLED) {
    Serial.println("");
  }
  pinMode(SDA_PIN, OUTPUT);
  return value;
}

// Write a byte of data on the serial line, sending most significant bit first
void writeSerialByte(byte data) {
  digitalWrite(CLK_PIN, LOW);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  if (DEBUG_ENABLED) {
    Serial.print("Sending Byte: ");
    Serial.println(data);
  }
  for (byte bidx = 0x80; bidx > 0; bidx /= 2) {
    if (DEBUG_ENABLED) Serial.print((data & bidx) > 0);
    digitalWrite(SDA_PIN, (data & bidx) > 0);
    digitalWrite(CLK_PIN, HIGH);
    if (DELAYS_ENABLED) delayMicroseconds(1);
    digitalWrite(CLK_PIN, LOW);
    if (DELAYS_ENABLED) delayMicroseconds(1);
  }
  if (DEBUG_ENABLED) {
    Serial.println("");
  }

  // Every write must be acknowledged by the EEPROM
  acknowledge();
}

// Pulse an acknowledge bit. Leaves both SDA and CLK low
void acknowledge() {
  digitalWrite(SDA_PIN, LOW);
  digitalWrite(CLK_PIN, HIGH);
  if (DELAYS_ENABLED) delayMicroseconds(1);
  digitalWrite(CLK_PIN, LOW);
  if (DELAYS_ENABLED) delayMicroseconds(1);
}