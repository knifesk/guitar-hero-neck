#include <Arduino.h>
#include <Wire.h>


#define SERIAL_EN false  // Enable serial output
#define VERBOSE false  // Verbose serial output

#define I2C_ADDRESS 0x10

#define BTN_GREEN  arr[2] & 0x40
#define BTN_RED    arr[2] & 0x01
#define BTN_YELLOW arr[2] & 0x02
#define BTN_BLUE   arr[2] & 0x10
#define BTN_ORANGE arr[2] & 0x20

#define TCH_GREEN  arr[1] & 0x08
#define TCH_RED    arr[1] & 0x04 
#define TCH_YELLOW arr[1] & 0x02 
#define TCH_BLUE   arr[1] & 0x01 
#define TCH_ORANGE arr[2] & 0x80


typedef struct {
  bool fGreen;
  bool fRed;
  bool fYellow;
  bool fBlue;
  bool fOrange;
  bool tGreen;
  bool tRed;
  bool tYellow;
  bool tBlue;
  bool tOrange;
} Buttons;

Buttons fourBytesToButton(uint8_t* arr) {

  /*
  the arr has 4 bytes:
  0B01010010 00011000 00000000 00000001
  MSBbits are packet id, type and some reserved (31 to 20)
  Bits 19-1 are buttons in order
  Bit 0 is always one.

  The guitar has the buttons wired as follows:
  Fret|Touch = BitN => Byte Number
  
  Green
  Fret  = 0x40 => 2
  Touch = 0x08 => 1

  Red
  Fret  = 0x01 => 2
  Touch = 0x04 => 1

  Yellow
  Fret  = 0x02 => 2
  Touch = 0x02 => 1

  Blue
  Fret  = 0x10 => 2
  Touch = 0x01 => 1

  Orange
  Fret  = 0x20 => 2
  Touch = 0x80 => 2
  */
 
  Buttons buttons = {
    .fGreen  = false,
    .fRed    = false,
    .fYellow = false,
    .fBlue   = false,
    .fOrange = false,
    .tGreen  = false,
    .tRed    = false,
    .tYellow = false,
    .tBlue   = false,
    .tOrange = false,
  };

  buttons.fGreen  = BTN_GREEN;
  buttons.fRed    = BTN_RED;
  buttons.fYellow = BTN_YELLOW;
  buttons.fBlue   = BTN_BLUE;
  buttons.fOrange = BTN_ORANGE;
  buttons.tGreen  = TCH_GREEN;
  buttons.tRed    = TCH_RED;
  buttons.tYellow = TCH_YELLOW;
  buttons.tBlue   = TCH_BLUE;
  buttons.tOrange = TCH_ORANGE;

  return buttons;
}

void diagnoseTransmissionError(byte code) {
  // Docs for wire.endtransmission: https://docs.particle.io/cards/firmware/wire-i2c/endtransmission/
  switch(code) {
    case 0:
      Serial.println("success");
      break;
    case 1:
      Serial.println("busy timeout upon entering endTransmission()");
      break;
    case 2:
      Serial.println("START bit generation timeout");
      break;
    case 3:
      Serial.println("end of address transmission timeout");
      break;
    case 4:
      Serial.println("data byte transfer timeout");
      break;
    case 5:
      Serial.println("data byte transfer succeeded, busy timeout immediately after");
      break;
    case 6:
      Serial.println("timeout waiting for peripheral to clear stop bit");
      break;
    default:
      Serial.print("Unknown return from EndTransmission: ");
      Serial.println(code);
  }
}

const char * hex = "0123456789ABCDEF";
void printByteArray(uint8_t* arr, unsigned int len) {
  Serial.print("Read ");
  Serial.print(len);
  Serial.print(" bytes:");
  for(unsigned int i = 0; i < len; i++) {
    Serial.print(" 0x");
    Serial.print(hex[(arr[i]>>4) & 0xF]);
    Serial.print(hex[arr[i] & 0xF]);
  }
  Serial.println("");
}

unsigned int readFromSerial(uint8_t* arr, unsigned int expectedByteCount) {
  unsigned int readCount = 0;
  Wire.requestFrom(I2C_ADDRESS, expectedByteCount);    // request N bytes from peripheral device
  
  while (Wire.available() && (readCount < expectedByteCount)) { // peripheral may send less than requested
    arr[readCount] = Wire.read(); // receive a byte as character
    readCount++;
  }

  return readCount;
}

bool isInitialized = false;
void initNeck() {
  uint8_t packetHello[4] = {0x55, 0x55, 0x55, 0x55};

  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(0x53); // Write Send a Write + Packet #3
  Wire.write(0x10); // Ask the TP to send button status (Register #1 -first nibble-)
  Wire.write(0x00); // Reserved
  Wire.write(0x01); // Just complete the last byte
  byte error = Wire.endTransmission();      // Stop transmitting
  if (error != 0) {
    if (SERIAL_EN) { diagnoseTransmissionError(error); }
    return;
  }

  if (SERIAL_EN && VERBOSE) {
    delay(50);
  } else {
    delay(10); // Manual says Wait 50µs to read. But original board uses 10ms only
  }

  //setup the vars
  uint expectedByteCount = 4;
  uint8_t values[expectedByteCount];
  unsigned int readCount = readFromSerial(values, expectedByteCount);
  if (SERIAL_EN && VERBOSE) { printByteArray(values, readCount); }
  if (readCount != expectedByteCount) {
    if (SERIAL_EN) { Serial.println("Wrong byte count read"); }
    return;
  }

  if (values[0] != 0x55) {
    if (SERIAL_EN && VERBOSE) { 
      Serial.println("Invalid response"); 
      printByteArray(values, readCount);
    }
    
    return;
  }

  if (SERIAL_EN) { 
    Serial.println("Initialized!!"); 
  }
  
  isInitialized = true;
}

void requestDataFromNeck() {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(0x53); // Write Send a Write + Packet #3
  Wire.write(0x10); // Ask the TP to send button status (Register #1 -first nibble-)
  Wire.write(0x00); // Reserved
  Wire.write(0x01); // Just complete the last byte
  byte error = Wire.endTransmission();      // Stop transmitting
  if (error != 0) {
    if (SERIAL_EN) { diagnoseTransmissionError(error); }
    return;
  }

  if (SERIAL_EN && VERBOSE) {
    delay(50);
  } else {
    delay(10); // Manual says Wait 50µs to read. But original board uses 10ms only
  }

  unsigned int expectedByteCount = 4; 
  uint8_t values[expectedByteCount];
  unsigned int readCount = readFromSerial(values, expectedByteCount);
  if (SERIAL_EN && VERBOSE) { printByteArray(values, readCount); }
  if (readCount != expectedByteCount) {
    if (SERIAL_EN) { 
      Serial.println("Wrong byte count read");
    }
    return;
  }

  // If response it's not packet #2, ignore it
  if (values[0] != 0x52) {
    return;
  }

  // Do the magic
}

void setup()
{
  Wire.begin();

  delay(350); // wait for neck powerup
  
  if (SERIAL_EN) {
    Serial.begin(115200);
    Serial.println("Setup complete");
  }
}

void loop() {
  if (SERIAL_EN && VERBOSE) {
    delay(500);
  } else {
    delay(10); // Original board waits 10ms only
  }
  
  if (!isInitialized) {
    initNeck();
  } else {
    requestDataFromNeck();
  }  
}