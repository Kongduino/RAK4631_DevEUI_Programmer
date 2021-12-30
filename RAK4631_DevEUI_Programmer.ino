#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
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
  initEEPROM();
  Serial.println("=====================================");
  Serial.println("            BLE Setup");
  initBLE();
  Serial.println("=====================================");
  readInfo(devEUI, nickname);
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
