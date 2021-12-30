void hexDump(char *, uint16_t);
void readEEPROM(char *, uint8_t, uint16_t);
void setDevEUI();
void ble_connect_callback(uint16_t);
void ble_disconnect_callback(uint16_t, uint8_t);
void handleBleuartRx();
void handleCommands();
void readInfo();
void setNickname();

BLEUart g_BleUart;
BLEClientDis clientDis; // device information client
// BLEClientUart g_BleUart;
/** Flag if BLE UART client is connected */
bool g_BleUartConnected = false;

#define EEPROM_ADDR 0x50 // the default address
#define MAXADD 262143 // max address in byte
Adafruit_EEPROM_I2C i2ceeprom;

char devEUI[8] = {0};
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
    if (c == 'd') setDevEUI();
    else if (c == 'r') {
      readInfo();
    } else if (c == 'n') setNickname();
  }
}

void setDevEUI() {
  uint16_t addr = 0x0000;
  uint8_t ix, px = strlen(rawBuf + 2);
  char txtBuf[128];
  sprintf(txtBuf, "Setting devEUI to: `%s`\n", (rawBuf + 2));
  Serial.print(txtBuf);
  if (g_BleUartConnected) g_BleUart.print(txtBuf);
  if (px != 16) {
    Serial.println("Incorrect length! I need exactly 16 hexadecimal digits!");
    if (g_BleUartConnected) g_BleUart.println("Incorrect length! I need exactly 16 hexadecimal digits!");
    return;
  }
  ix = 0;
  for (px = 2; px < 18; px += 2) {
    char c = rawBuf[px];
    if (c >= '0' && c <= '9') c = c - '0';
    else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
    else {
      sprintf(txtBuf, "Not an hexadecimal digit: %c\n", c);
      Serial.print(txtBuf);
      if (g_BleUartConnected) g_BleUart.print(txtBuf);
      return;
    }
    char d = rawBuf[px + 1];
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
    devEUI[ix++] = c;
    i2ceeprom.write(addr++, c);
  }
  Serial.println("Wrote devEUI:");
  hexDump(devEUI, 8);
  if (g_BleUartConnected) g_BleUart.println("Wrote devEUI.");
}

void setNickname() {
  uint8_t ix, px = strlen(rawBuf + 2);
  uint16_t addr = 0x0008;
  for (ix = 0; ix < px; ix++) i2ceeprom.write(addr++, rawBuf[ix + 2]);
  char txtBuf[128];
  sprintf(txtBuf, "Set nickname to: `%s`\n", (rawBuf + 2));
  Serial.print(txtBuf);
  if (g_BleUartConnected) g_BleUart.print(txtBuf);
}

void readEEPROM(char *buf, uint8_t len, uint16_t addr) {
  for (uint8_t ix = 0; ix < len; ix++) buf[ix] = (char)i2ceeprom.read(addr++);
}

uint16_t readEEPROMString(uint16_t addr) {
  char c = 0xFF;
  uint8_t ix = 0;
  while (c != 0) {
    c = (char)i2ceeprom.read(addr++);
    rawBuf[ix++] = c;
  }
  return (ix + 1);
}

void readInfo() {
  Serial.println("DevEUI:");
  readEEPROM(devEUI, 8, 0);
  hexDump(devEUI, 8);
  if (g_BleUartConnected) {
    String s = "DevEUI: ";
    for (uint8_t ix = 0; ix < 8; ix++) {
      char c = devEUI[ix];
      if (c < 16) s = s + "0";
      s = s + String(c, HEX);
    }
    g_BleUart.println(s);
  }
  Serial.print("Nickname: ");
  if (g_BleUartConnected) g_BleUart.print("Nickname: ");
  uint16_t px = readEEPROMString(8);
  Serial.println(rawBuf);
  if (g_BleUartConnected) g_BleUart.println(rawBuf);
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
