#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_EEPROM_I2C.h" // Click here to get the library: http://librarymanager/All#Adafruit_EEPROM_I2C
#include "Helper.h"

void setup() {
  time_t timeout = millis();
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.begin(115200);
  delay(300);
  Serial.println("=====================================");
  Serial.println("        DevEUI EEPROM Storage");
  Serial.println("=====================================");
  Serial.println("          Turning on module");
  pinMode(WB_IO2, INPUT_PULLUP); // EPD
  digitalWrite(WB_IO2, HIGH);
  delay(300);
  if (i2ceeprom.begin(EEPROM_ADDR)) {
    // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C EEPROM");
  } else {
    Serial.println("I2C EEPROM not identified ... check your connections?\r\n");
    while (1) {
      delay(10);
    }
  }
  Serial.println("=====================================");
  Serial.println("            BLE Setup");
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.configPrphConn(92, BLE_GAP_EVENT_LENGTH_MIN, 16, 16);
  Bluefruit.begin(1, 0);
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  // Set the BLE device name
  Bluefruit.setName("RAK_DevEUI_BLE");
  Bluefruit.Periph.setConnectCallback(ble_connect_callback);
  Bluefruit.Periph.setDisconnectCallback(ble_disconnect_callback);
  // Configure and Start BLE Uart Service
  clientDis.begin();
  g_BleUart.begin();
  // Set up and start advertising
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30); // number of seconds in fast mode
  Bluefruit.Advertising.start(0); // 0 = Don't stop advertising after n seconds
  Serial.println("=====================================");

  readInfo();
}

void loop() {
  if (Serial.available()) {
    memset(rawBuf, 0, 256);
    uint8_t ix = 0;
    while (Serial.available()) {
      char c = Serial.read();
      delay(10);
      if (c > 31) rawBuf[ix++] = c;
      else rawBuf[ix++] = 0;
    }
    Serial.println("Incoming:");
    hexDump(rawBuf, ix);
    handleCommands();
  }
  if (g_BleUart.available()) {
    handleBleuartRx();
  }
}
