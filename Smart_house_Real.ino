#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Keypad.h>

/* =========================
   LCD + RTC
   ========================= */
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;
bool rtcOK = false;

/* =========================
   Pins
   ========================= */
const int tempPin     = A0;
const int ldrPin      = A1;
const int whiteLedPin = A2;
const int pirPin      = 9;
const int buzzerPin   = 10;
const int redLedPin   = 11;
const int greenLedPin = 12;
const int fanPin      = 13;

/* =========================
   Keypad
   Rows: D2,D3,D4,D5
   Cols: D6,D7,D8,A3
   ========================= */
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'7', '8', '9', '/'},
  {'4', '5', '6', '*'},
  {'1', '2', '3', '-'},
  {'C', '0', '=', '+'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, A3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* =========================
   EEPROM user system
   Each user: 1 active byte + 4 password bytes
   ========================= */
const byte MAX_USERS = 5;
const int EEPROM_MAGIC_ADDR1 = 0;
const int EEPROM_MAGIC_ADDR2 = 1;
const byte EEPROM_MAGIC1 = 0x5A;
const byte EEPROM_MAGIC2 = 0xA5;
const int USER_BASE_ADDR = 10;
const int USER_RECORD_SIZE = 5;

int userAddr(byte slot) {
  return USER_BASE_ADDR + (slot * USER_RECORD_SIZE);
}

void saveUser(byte slot, bool active, const char *pass) {
  int addr = userAddr(slot);

  EEPROM.update(addr, active ? 1 : 0);

  for (int i = 0; i < 4; i++) {
    EEPROM.update(addr + 1 + i, pass[i]);
  }
}

void loadUser(byte slot, bool &active, char pass[5]) {
  int addr = userAddr(slot);

  active = (EEPROM.read(addr) == 1);

  for (int i = 0; i < 4; i++) {
    pass[i] = (char)EEPROM.read(addr + 1 + i);
  }
  pass[4] = '\0';
}

void initUsersIfNeeded() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR1) != EEPROM_MAGIC1 ||
      EEPROM.read(EEPROM_MAGIC_ADDR2) != EEPROM_MAGIC2) {

    EEPROM.write(EEPROM_MAGIC_ADDR1, EEPROM_MAGIC1);
    EEPROM.write(EEPROM_MAGIC_ADDR2, EEPROM_MAGIC2);

    saveUser(0, true, "1234"); // default admin = User 1

    for (byte i = 1; i < MAX_USERS; i++) {
      saveUser(i, false, "0000");
    }
  }
}

bool checkUserPassword(byte userNo, const char *enteredPass) {
  if (userNo < 1 || userNo > MAX_USERS) return false;

  bool active;
  char storedPass[5];
  loadUser(userNo - 1, active, storedPass);

  if (!active) return false;

  for (int i = 0; i < 4; i++) {
    if (storedPass[i] != enteredPass[i]) return false;
  }
  return true;
}

/* =========================
   Screen states
   ========================= */
enum ScreenState {
  SCR_LOGIN_USER,
  SCR_LOGIN_PASS,
  SCR_MAIN,
  SCR_TEMP,
  SCR_CLOCK,
  SCR_LIGHT_MENU,
  SCR_MOTION,
  SCR_AMBIENT,
  SCR_USER_MENU,
  SCR_NEWUSER_SLOT,
  SCR_NEWUSER_PASS
};

ScreenState screen = SCR_LOGIN_USER;

bool loggedIn = false;
byte activeUser = 0;
byte selectedLoginUser = 0;
byte newUserSlot = 0;

char pinBuffer[5] = "";
byte pinLen = 0;

bool needRedraw = true;
unsigned long lastDynamicRefresh = 0;
const unsigned long REFRESH_MS = 1000;
unsigned long greenLedOffAt = 0;

unsigned long lastMotionTime = 0;
const unsigned long motionHoldMs = 5000; 

/* =========================
   Helper functions
   ========================= */
void clearPinBuffer() {
  for (byte i = 0; i < 5; i++) pinBuffer[i] = '\0';
  pinLen = 0;
}

void appendDigit(char k) {
  if (pinLen < 4 && k >= '0' && k <= '9') {
    pinBuffer[pinLen++] = k;
    pinBuffer[pinLen] = '\0';
  }
}

void beepShort() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

void beepError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(120);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
}

void allOutputsOff() {
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(whiteLedPin, LOW);
  digitalWrite(fanPin, LOW);
  digitalWrite(buzzerPin, LOW);
}


int readTemperature10() {
  analogRead(tempPin);
  delay(5);

  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(tempPin);
    delay(2);
  }

  long raw = total / 10;
  long temp10 = (raw * 5000L) / 1023L;   // LM35
  return (int)temp10;
}


int readLDR() {
  analogRead(ldrPin);
  delay(5);

  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(ldrPin);
    delay(2);
  }

  return total / 10;
}

bool isDark() {
  return (readLDR() < 600); 
}

void updateOutputs() {
  // Fan control by temperature only
  if (loggedIn && readTemperature10() >= 300) {
    digitalWrite(fanPin, HIGH);
  } else {
    digitalWrite(fanPin, LOW);
  }

  bool whiteOn = false;

  if (loggedIn) {
    // Ambient mode
    if (screen == SCR_AMBIENT && isDark()) {
      whiteOn = true;
    }

    // Motion mode with hold time
    if (screen == SCR_MOTION) {
      if (digitalRead(pirPin) == HIGH) {
        lastMotionTime = millis();
      }

      if (millis() - lastMotionTime <= motionHoldMs) {
        whiteOn = true;
      }
    }
  }

  digitalWrite(whiteLedPin, whiteOn ? HIGH : LOW);

  // Green LED off after 5 seconds
  if (greenLedOffAt != 0 && millis() >= greenLedOffAt) {
    digitalWrite(greenLedPin, LOW);
    greenLedOffAt = 0;
  }
}

/* =========================
   LCD screen functions
   ========================= */
void showTwoLines(const String &l1, const String &l2) {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(l1);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void drawLoginUser() {
  showTwoLines("Select User 1-5", "Choice:");
}

void drawLoginPass() {
  String stars = "";
  for (byte i = 0; i < pinLen; i++) stars += "*";

  String line1 = "User " + String(selectedLoginUser) + " Pass:";
  showTwoLines(line1, stars);
}

void drawMainMenu() {
  showTwoLines("1:Temp 2:Clock", "3:Light 4:User");
}

void drawTempScreen() {
  int t10 = readTemperature10();
  int tInt = t10 / 10;
  int tDec = t10 % 10;

  String line1 = "Temp:" + String(tInt) + "." + String(tDec) + "C";
  String line2 = (t10 >= 300) ? "Fan:ON C:Back" : "Fan:OFF C:Back";

  showTwoLines(line1, line2);
}

void drawClockScreen() {
  if (rtcOK) {
    DateTime now = rtc.now();

    String hh = (now.hour() < 10 ? "0" : "") + String(now.hour());
    String mm = (now.minute() < 10 ? "0" : "") + String(now.minute());
    String ss = (now.second() < 10 ? "0" : "") + String(now.second());

    showTwoLines("Time:" + hh + ":" + mm + ":" + ss, "C:Back");
  } else {
    showTwoLines("RTC ERROR", "C:Back");
  }
}

void drawLightMenu() {
  showTwoLines("1:Motion 2:Amb", "C:Back");
}

void drawMotionScreen() {
  if (digitalRead(pirPin) == HIGH) {
    showTwoLines("Motion: YES", "LED:ON C:Back");
  } else {
    if (millis() - lastMotionTime <= motionHoldMs) {
      showTwoLines("Motion: NO", "LED:ON C:Back");
    } else {
      showTwoLines("Motion: NO", "LED:OFF C:Back");
    }
  }
}

void drawAmbientScreen() {
  int ldrValue = readLDR();
  String line1 = "LDR:" + String(ldrValue);

  if (isDark()) {
    showTwoLines(line1, "LED:ON C:Back");
  } else {
    showTwoLines(line1, "LED:OFF C:Back");
  }
}

void drawUserMenu() {
  if (activeUser != 1) {
    showTwoLines("Only User1", "C:Back");
  } else {
    showTwoLines("1:New User", "C:Back");
  }
}

void drawNewUserSlot() {
  showTwoLines("New User Slot", "Select 2-5");
}

void drawNewUserPass() {
  String stars = "";
  for (byte i = 0; i < pinLen; i++) stars += "*";

  String line1 = "User " + String(newUserSlot) + " Pass:";
  showTwoLines(line1, stars);
}

void drawScreen() {
  switch (screen) {
    case SCR_LOGIN_USER:   drawLoginUser(); break;
    case SCR_LOGIN_PASS:   drawLoginPass(); break;
    case SCR_MAIN:         drawMainMenu(); break;
    case SCR_TEMP:         drawTempScreen(); break;
    case SCR_CLOCK:        drawClockScreen(); break;
    case SCR_LIGHT_MENU:   drawLightMenu(); break;
    case SCR_MOTION:       drawMotionScreen(); break;
    case SCR_AMBIENT:      drawAmbientScreen(); break;
    case SCR_USER_MENU:    drawUserMenu(); break;
    case SCR_NEWUSER_SLOT: drawNewUserSlot(); break;
    case SCR_NEWUSER_PASS: drawNewUserPass(); break;
  }

  needRedraw = false;
  lastDynamicRefresh = millis();
}

/* =========================
   Login / logout
   ========================= */
void loginSuccess() {
  loggedIn = true;
  activeUser = selectedLoginUser;

  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);
  greenLedOffAt = millis() + 5000;

  showTwoLines("Login Success", "Opening Menu");
  delay(1000);

  screen = SCR_MAIN;
  needRedraw = true;
}

void loginFail() {
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);

  showTwoLines("Wrong Password", "Try Again");
  beepError();
  digitalWrite(redLedPin, LOW);

  selectedLoginUser = 0;
  clearPinBuffer();
  screen = SCR_LOGIN_USER;
  needRedraw = true;
}

void logoutSystem() {
  loggedIn = false;
  activeUser = 0;
  selectedLoginUser = 0;
  newUserSlot = 0;
  clearPinBuffer();
  allOutputsOff();
  greenLedOffAt = 0;
  screen = SCR_LOGIN_USER;
  needRedraw = true;
}

/* =========================
   Key handling
   ========================= */
void handleLoginUserKey(char key) {
  if (key >= '1' && key <= '5') {
    selectedLoginUser = key - '0';
    clearPinBuffer();
    screen = SCR_LOGIN_PASS;
    needRedraw = true;
  }
}

void handleLoginPassKey(char key) {
  if (key == 'C') {
    if (pinLen > 0) {
      clearPinBuffer();
      needRedraw = true;
    } else {
      selectedLoginUser = 0;
      screen = SCR_LOGIN_USER;
      needRedraw = true;
    }
    return;
  }

  if (key >= '0' && key <= '9') {
    appendDigit(key);
    needRedraw = true;

    if (pinLen == 4) {
      if (checkUserPassword(selectedLoginUser, pinBuffer)) {
        loginSuccess();
      } else {
        loginFail();
      }
    }
  }
}

void handleMainMenuKey(char key) {
  if (key == '1') { screen = SCR_TEMP; needRedraw = true; }
  else if (key == '2') { screen = SCR_CLOCK; needRedraw = true; }
  else if (key == '3') { screen = SCR_LIGHT_MENU; needRedraw = true; }
  else if (key == '4') { screen = SCR_USER_MENU; needRedraw = true; }
  else if (key == 'C') { logoutSystem(); }
}

void handleTempClockKey(char key) {
  if (key == 'C') {
    screen = SCR_MAIN;
    needRedraw = true;
  }
}

void handleLightMenuKey(char key) {
  if (key == '1') { screen = SCR_MOTION; needRedraw = true; }
  else if (key == '2') { screen = SCR_AMBIENT; needRedraw = true; }
  else if (key == 'C') { screen = SCR_MAIN; needRedraw = true; }
}

void handleLightSubKey(char key) {
  if (key == 'C') {
    screen = SCR_LIGHT_MENU;
    needRedraw = true;
  }
}

void handleUserMenuKey(char key) {
  if (key == 'C') {
    screen = SCR_MAIN;
    needRedraw = true;
    return;
  }

  if (activeUser != 1) return;

  if (key == '1') {
    newUserSlot = 0;
    clearPinBuffer();
    screen = SCR_NEWUSER_SLOT;
    needRedraw = true;
  }
}

void handleNewUserSlotKey(char key) {
  if (key == 'C') {
    screen = SCR_USER_MENU;
    needRedraw = true;
    return;
  }

  if (key >= '2' && key <= '5') {
    newUserSlot = key - '0';
    clearPinBuffer();
    screen = SCR_NEWUSER_PASS;
    needRedraw = true;
  }
}

void handleNewUserPassKey(char key) {
  if (key == 'C') {
    if (pinLen > 0) {
      clearPinBuffer();
      needRedraw = true;
    } else {
      screen = SCR_NEWUSER_SLOT;
      needRedraw = true;
    }
    return;
  }

  if (key >= '0' && key <= '9') {
    appendDigit(key);
    needRedraw = true;

    if (pinLen == 4) {
      saveUser(newUserSlot - 1, true, pinBuffer);

      showTwoLines("New User Saved", "Success");
      beepShort();
      delay(1000);

      clearPinBuffer();
      screen = SCR_USER_MENU;
      needRedraw = true;
    }
  }
}

void handleKey(char key) {
  switch (screen) {
    case SCR_LOGIN_USER:   handleLoginUserKey(key); break;
    case SCR_LOGIN_PASS:   handleLoginPassKey(key); break;
    case SCR_MAIN:         handleMainMenuKey(key); break;
    case SCR_TEMP:
    case SCR_CLOCK:        handleTempClockKey(key); break;
    case SCR_LIGHT_MENU:   handleLightMenuKey(key); break;
    case SCR_MOTION:
    case SCR_AMBIENT:      handleLightSubKey(key); break;
    case SCR_USER_MENU:    handleUserMenuKey(key); break;
    case SCR_NEWUSER_SLOT: handleNewUserSlotKey(key); break;
    case SCR_NEWUSER_PASS: handleNewUserPassKey(key); break;
  }
}

/* =========================
   Setup / Loop
   ========================= */
void setup() {
  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(whiteLedPin, OUTPUT);

  allOutputsOff();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  initUsersIfNeeded();

  rtcOK = rtc.begin();
  if (rtcOK) {
  }

  screen = SCR_LOGIN_USER;
  needRedraw = true;
}

void loop() {
  updateOutputs();

  char key = keypad.getKey();
  if (key) {
    handleKey(key);
  }

  bool dynamicScreen =
    (screen == SCR_TEMP ||
     screen == SCR_CLOCK ||
     screen == SCR_MOTION ||
     screen == SCR_AMBIENT);

  if (needRedraw || (dynamicScreen && millis() - lastDynamicRefresh > REFRESH_MS)) {
    drawScreen();
  }
}