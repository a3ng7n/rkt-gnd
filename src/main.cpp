#include <Arduino.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// SX1276 has the following connections:
// CS pin:   10
// DIO0 pin:  3
// RESET pin: 2
// DIO1 pin:  4 <-- not doing this
SX1276 radio = new Module(10, 3, 2);

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setup() {
  // while (!Serial) {
  //   // wait for Arduino Serial Monitor to be ready
  //   delay(1000);
  // }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  // text display tests
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // initialize SX1276 with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

    // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1276] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void comboPrint(String msg) {
  Serial.print(msg);
  display.print(msg);
}

void comboPrintln(String msg) {
  Serial.println(msg);
  display.println(msg);
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio.getPacketLength();
      int state = radio.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE) {
      display.clearDisplay();
      display.setCursor(0,0);

      // packet was successfully received
      comboPrint(F("RX:"));

      // print the data of the packet
      // comboPrint(F(""));
      comboPrintln(str);

      // print the RSSI (Received Signal Strength Indicator)
      // of the last received packet
      comboPrint(F("RSSI:"));
      comboPrint(radio.getRSSI());
      comboPrint(F("dBm|"));

      // print the SNR (Signal-to-Noise Ratio)
      // of the last received packet
      comboPrint(F("SNR:"));
      comboPrint(radio.getSNR());
      comboPrint(F("dB|"));

      // print frequency error
      // of the last received packet
      comboPrint(F("ferr:"));
      comboPrint(radio.getFrequencyError());
      comboPrintln(F("Hz"));
      display.display();

    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      // timeout occurred while waiting for a packet
      Serial.println(F("timeout!"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
}