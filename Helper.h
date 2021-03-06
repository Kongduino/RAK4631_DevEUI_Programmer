void hexDump(char *, uint16_t);
void initEEPROM();
void readEEPROM(char *, uint8_t, uint16_t);
void setDevEUI(char *, char *);
void initBLE();
void ble_connect_callback(uint16_t);
void ble_disconnect_callback(uint16_t, uint8_t);
void handleBleuartRx();
void handleCommands();
void readInfo(char *, char *);
uint16_t readEEPROMString(char *, uint16_t);
void setNickname(char *);

#include <bluefruit.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_EEPROM_I2C.h" // Click here to get the library: http://librarymanager/All#Adafruit_EEPROM_I2C

BLEUart g_BleUart;
BLEClientDis clientDis; // device information client
// BLEClientUart g_BleUart;
/** Flag if BLE UART client is connected */
bool g_BleUartConnected = false;

#define EEPROM_ADDR 0x50 // the default address
#define MAXADD 262143 // max address in byte
Adafruit_EEPROM_I2C i2ceeprom;

char devEUI[8] = {0};
char nickname[33] = {0};
char rawBuf[256] = {0};

void ble_connect_callback(uint16_t conn_handle) {
  (void)conn_handle;
  g_BleUartConnected = true;
  Serial.println("BLE client connected");
}

void ble_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;
  g_BleUartConnected = false;
  Serial.println("BLE client disconnected");
}

void handleBleuartRx() {
  Serial.print("[RX]: ");
  uint8_t ix = 0;
  while (g_BleUart.available()) {
    uint8_t ch = (uint8_t) g_BleUart.read();
    if (ch > 31) {
      Serial.write(ch);
      rawBuf[ix++] = ch;
    }
    rawBuf[ix] = '\0';
  }
  Serial.write('\n');
  handleCommands();
}

void handleCommands() {
  if (rawBuf[0] == '/') {
    char c = rawBuf[1];
    if (c == 'd') setDevEUI(rawBuf + 2, devEUI);
    else if (c == 'r') {
      readInfo(devEUI, nickname);
    } else if (c == 'n') setNickname(rawBuf + 2);
  }
}

void initBLE() {
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
}

void initEEPROM() {
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
}

void setDevEUI(char *src, char *dest) {
  uint16_t addr = 0x0000;
  uint8_t ix, px = strlen(src);
  char txtBuf[64];
  sprintf(txtBuf, "Setting devEUI to: `%s`\n", (src));
  Serial.print(txtBuf);
  if (g_BleUartConnected) g_BleUart.print(txtBuf);
  if (px != 16) {
    Serial.println("Incorrect length! I need exactly 16 hexadecimal digits!");
    if (g_BleUartConnected) g_BleUart.println("Incorrect length! I need exactly 16 hexadecimal digits!");
    return;
  }
  ix = 0;
  for (px = 0; px < 16; px += 2) {
    char c = src[px];
    if (c >= '0' && c <= '9') c = c - '0';
    else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
    else {
      sprintf(txtBuf, "Not an hexadecimal digit: %c\n", c);
      Serial.print(txtBuf);
      if (g_BleUartConnected) g_BleUart.print(txtBuf);
      return;
    }
    char d = src[px + 1];
    if (d >= '0' && d <= '9') d = d - '0';
    else if (d >= 'a' && d <= 'f') d = d - 'a' + 10;
    else if (d >= 'A' && d <= 'F') d = d - 'A' + 10;
    else {
      sprintf(txtBuf, "Not an hexadecimal digit: %c\n", c);
      Serial.print(txtBuf);
      if (g_BleUartConnected) g_BleUart.print(txtBuf);
      return;
    }
    // shift 4 to make space for new digit, and add the 4 bits of the new digit
    c = (c << 4) | d;
    dest[ix++] = c;
    i2ceeprom.write(addr++, c);
  }
  Serial.println("Wrote devEUI:");
  hexDump(dest, 8);
  if (g_BleUartConnected) g_BleUart.println("Wrote devEUI.");
}

void setNickname(char *buf) {
  uint8_t ix, px = strlen(rawBuf + 2);
  uint16_t addr = 0x0008;
  for (ix = 0; ix < px; ix++) i2ceeprom.write(addr++, buf[ix + 2]);
  char txtBuf[128];
  sprintf(txtBuf, "Set nickname to: `%s`\n", (buf + 2));
  Serial.print(txtBuf);
  if (g_BleUartConnected) g_BleUart.print(txtBuf);
}

void readEEPROM(char *buf, uint8_t len, uint16_t addr) {
  for (uint8_t ix = 0; ix < len; ix++) buf[ix] = (char)i2ceeprom.read(addr++);
}

uint16_t readEEPROMString(char *buf, uint16_t addr) {
  char c = 0xFF;
  uint8_t ix = 0;
  while (c != 0) {
    c = (char)i2ceeprom.read(addr++);
    buf[ix++] = c;
  }
  return (ix + 1);
}

void readInfo(char *eui, char *buf) {
  Serial.println("DevEUI:");
  readEEPROM(eui, 8, 0);
  hexDump(eui, 8);
  if (g_BleUartConnected) {
    String s = "DevEUI: ";
    for (uint8_t ix = 0; ix < 8; ix++) {
      char c = eui[ix];
      if (c < 16) s = s + "0";
      s = s + String(c, HEX);
    }
    g_BleUart.println(s);
  }
  Serial.print("Nickname: ");
  if (g_BleUartConnected) g_BleUart.print("Nickname: ");
  uint16_t px = readEEPROMString(buf, 8);
  Serial.println(buf);
  if (g_BleUartConnected) g_BleUart.println(buf);
}

void hexDump(char *buf, uint16_t len) {
  String s = "|", t = "| |";
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
  Serial.println(F("  |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f |"));
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
  for (uint16_t i = 0; i < len; i += 16) {
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j >= len) {
        s = s + "   "; t = t + " ";
      } else {
        char c = buf[i + j];
        if (c < 16) s = s + "0";
        s = s + String(c, HEX) + " ";
        if (c < 32 || c > 127) t = t + ".";
        else t = t + (String(c));
      }
    }
    uint8_t index = i / 16;
    Serial.print(index, HEX); Serial.write('.');
    Serial.println(s + t + "|");
    s = "|"; t = "| |";
  }
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
}
