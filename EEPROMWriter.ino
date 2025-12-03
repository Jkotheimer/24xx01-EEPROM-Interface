/**
 * This sketch is meant to write to the 24xx01 EEPROM using a serial interface
 */
#define SDA_PIN 2
#define CLK_PIN 3

#define INIT_WRITE_BYTE 0xA0
#define INIT_READ_BYTE 0xA1

byte sequence[] = {
  0x00
};

void setup() {
  Serial.begin(115200);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(SDA_PIN, OUTPUT);

  // Per spec, "Bus Not Busy" state is where both data and clock lines remain high
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(SDA_PIN, HIGH);

  startDataTransfer(true);
}

void loop() {}

/**
 * @param write - If true, data transfer request is in write mode
 */
void startDataTransfer(bool write) {
  digitalWrite(SDA_PIN, LOW);
  delayMicroseconds(1);
  // Write device addressing sequence
  writeByte(INIT_WRITE_BYTE);
}

void writeByte(byte data) {
  digitalWrite(CLK_PIN, LOW);
  for (uint8_t bidx = 1; bidx != 256; bidx *= 2) {
    digitalWrite(SDA_PIN, data & bidx);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK_PIN, LOW);
  }
  digitalWrite(CLK_PIN, HIGH);
}