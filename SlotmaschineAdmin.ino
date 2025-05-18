#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <MFRC522.h>

void storeNewPlayer(byte *uid, int bonus = 0);
// Pins für TFT-Display
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8

// Kalibrierung Touchscreens
const int XP = 8, XM = A2, YP = A3, YM = 9; // Pins für den Touchscreen
const int TS_LEFT = 829, TS_RT = 135, TS_TOP = 168, TS_BOT = 908; // Kalibrierte Werte

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// TFT Display-Objekt
MCUFRIEND_kbv tft;

// Größe des Displays
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// Positionen der Symbole auf dem Display
int symbol1X, symbol1Y;
int symbol2X, symbol2Y;
int symbol3X, symbol3Y;
int symbolWidth = 60;
int symbolHeight = 60;

// Statusvariable zum Anhalten der Symbole
bool stopSymbols = false;

// Text zur Anzeige des Status
String statusText = "STOP";
bool statusChanged = true;

// Liste der möglichen Farben (RGB)
const uint8_t colors[][3] = {
  {255, 0, 0}, // Rot
  {255, 0, 0},
  {0, 255, 0}, // Grün
  {0, 255, 0},
  {0, 255, 0},
  {0, 0, 255}, //Blau
  {0, 0, 255},
  {0, 0, 255},
  {0, 0, 255},
  {255, 255, 0}, // Gelb
  {255, 255, 0},
  {255, 255, 0},
  {255, 255, 0},
  {255, 255, 0},
  {255, 0, 255}, // Lila
  {255, 0, 255},
  {255, 0, 255},
  {255, 0, 255},
  {255, 0, 255}, 
  {255, 255, 255}, // Weiß
  {255, 255, 255},
  {255, 255, 255},
  {255, 255, 255},
  {255, 255, 255},
  {255, 255, 255},
  {0, 0, 0}, // Schwarz
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};
const int numColors = sizeof(colors) / sizeof(colors[0]);

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Funktion zur Anzeige des Status-Texts
void displayStatusText() {
  tft.fillRect(0, 200, TFT_WIDTH, 40, color565(0, 0, 0)); // Bereich für den Text löschen
  tft.setTextColor(color565(255, 255, 255)); // Textfarbe
  tft.setTextSize(3); // Textgröße
  tft.setCursor(40, 200); // Textposition
  tft.print(statusText); // Status-Text
}

// Funktion zur Anzeige der Buttons
void displayButtons() {
  tft.fillRect(10, 120, 100, 40, color565(0, 0, 255));
  tft.fillRect(135, 120, 100, 40, color565(0, 255, 0));

  tft.setTextColor(color565(255, 255, 255)); // Textfarbe Weiß
  tft.setTextSize(2); // Textgröße
  tft.setCursor(35, 122); // Textposition
  tft.print("Spin");
  tft.setCursor(30, 140); // Textposition 
  tft.print("again");

  tft.setCursor(140, 130); // Textposition
  tft.print("Cash out"); 
}

int pin = 25; 
bool winMelody = false;
bool loseMelody = false;

// MFRC522 RFID
#define SS_PIN 53
#define RST_PIN 24

MFRC522 rfid(SS_PIN, RST_PIN); 

MFRC522::MIFARE_Key key;

// Max players
#define MAX_PLAYERS 100

// Player struktur
struct Player {
  byte uid[4];
  int credit;
  char name[20];
};

// Player array
Player players[MAX_PLAYERS];
int playerCount = 0;

// Variable zur Speicherung des Spielers
int currentPlayerIndex = -1; // Index zuletzt gescannten Spielers

// Flag to check if admin card has been scanned
bool isAdminCardScanned = false;

bool isAdminCard(byte *uid) {
  byte adminUID[4] = {0x53, 0x54, 0x30, 0x3E};
  return memcmp(uid, adminUID, 4) == 0;
}

void setup() {
  Serial.begin(9600);

  SPI.begin(); // SPI initialisieren für MFRC522
  rfid.PCD_Init(); // MFRC522 initialisieren

  tft.reset(); // TFT zurücksetzen
  uint16_t identifier = tft.readID(); // TFT Display ID lesen

  if (identifier == 0x0 || identifier == 0xFFFF) {
    Serial.println("Unknown LCD driver chip.");
    while (1);
  }

  tft.begin(identifier); // TFT initialisieren

  Serial.print("TFT Display ID: 0x");
  Serial.println(identifier, HEX);

  tft.setRotation(0); // Display-Rotation

  tft.fillScreen(color565(0, 0, 0)); // Hintergrundfarbe TFT-Displays Schwarz

  // Positionen Symbole
  symbol1X = 10;  
  symbol1Y = 50;
  symbol2X = 90;  
  symbol2Y = 50;
  symbol3X = 170; 
  symbol3Y = 50;

  // Anzeigen Slotmaschine
  displaySlotMachine();

  // Status-Text
  displayStatusText();

  // Buzzer
  pinMode(pin, OUTPUT);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Anzeige "Scan Card"
  displayScanCard();
}

void loop() {
  static uint32_t lastUpdate = 0;

  if (currentPlayerIndex != -1) {
    if (millis() - lastUpdate > 60 && !stopSymbols) { //Intervall
      lastUpdate = millis();
      testDisplay();
    }
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > ts.pressureThreshhold) {
      if (stopSymbols) {
        if (statusText == "TRY AGAIN") {
          resetGame();
        } else if (statusText == "WINNER!" || statusText == "JACKPOT") {
          int x = map(p.x, TS_LEFT, TS_RT, 0, TFT_WIDTH);
          int y = map(p.y, TS_TOP, TS_BOT, 0, TFT_HEIGHT);
          if (x > -25 && x < 170 && y > 70 && y < 100) {
            // "Spin again" gedrückt
            resetGame();
          } else if (x > 175 && x < 340 && y > 70 && y < 100) {
            // "Cash out" gedrückt
            cashOut();
          }
        }
      } else {
        stopSymbols = true;
        Serial.println("-100");
        checkWin();
      }
    }

    if (statusChanged) {
      displayStatusText();
      statusChanged = false;
    }
    // Buzzer Melodien
    if (winMelody) {
      playWinMelody();
      winMelody = false;
    }
    if (loseMelody) {
      playLoseMelody();
      loseMelody = false;
    }
  }

  // RFID Loop
  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  if (isAdminCard(rfid.uid.uidByte)) {
    isAdminCardScanned = true;
    Serial.println(F("Admin Karte registriert. Wähle Karte zum Aufladen"));
    displayScanCard(); // Update display with new message
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
}


  int playerIndex = findPlayer(rfid.uid.uidByte);
  if (playerIndex == -1) {
    Serial.print(F("New player registriert"));
    if (isAdminCardScanned) {
      if (playerCount < MAX_PLAYERS) {
        storeNewPlayer(rfid.uid.uidByte, 1000); // Store new player with 1000 extra credit
        Serial.println(F(" with 1000 bonus credit."));
        currentPlayerIndex = playerCount - 1; // Setzt den Index des neuen Spielers
        isAdminCardScanned = false; // Reset admin card flag
      } else {
        Serial.println(F("Maximum number of players reached."));
      }
    } else {
      storeNewPlayer(rfid.uid.uidByte);
      currentPlayerIndex = playerCount - 1; // Setzt den Index des neuen Spielers
    }
  } else {
    Serial.println(F("Card already registered."));
    if (isAdminCardScanned) {
      players[playerIndex].credit += 1000; // Add 1000 credit to existing player
      Serial.print(F("1000 credit added to "));
      Serial.println(players[playerIndex].name);
      isAdminCardScanned = false; // Reset admin card flag
    }
    currentPlayerIndex = playerIndex; // Setzt den Index des bestehenden Spielers
  }

  // Spielerinformationen anzeigen
  displayPlayerInfo(currentPlayerIndex);

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption PCD
  rfid.PCD_StopCrypto1();
}

void testDisplay() {
  uint8_t colorIndex1 = random(numColors);
  uint8_t colorIndex2 = random(numColors);
  uint8_t colorIndex3 = random(numColors);

  uint16_t color1 = color565(colors[colorIndex1][0], colors[colorIndex1][1], colors[colorIndex1][2]);
  uint16_t color2 = color565(colors[colorIndex2][0], colors[colorIndex2][1], colors[colorIndex2][2]);
  uint16_t color3 = color565(colors[colorIndex3][0], colors[colorIndex3][1], colors[colorIndex3][2]);

  tft.fillRect(symbol1X, symbol1Y, symbolWidth, symbolHeight, color1); // Zufällige Farbe für Symbol 1
  tft.fillRect(symbol2X, symbol2Y, symbolWidth, symbolHeight, color2); // Zufällige Farbe für Symbol 2
  tft.fillRect(symbol3X, symbol3Y, symbolWidth, symbolHeight, color3); // Zufällige Farbe für Symbol 3
}

// Funktion zur Anzeige der Slotmaschine auf Display
void displaySlotMachine() {
  // Symbole als farbige Rechtecke darstellen
  tft.fillRect(symbol1X, symbol1Y, symbolWidth, symbolHeight, color565(255, 0, 0)); // Symbol 1 in Rot
  tft.fillRect(symbol2X, symbol2Y, symbolWidth, symbolHeight, color565(0, 255, 0)); // Symbol 2 in Grün
  tft.fillRect(symbol3X, symbol3Y, symbolWidth, symbolHeight, color565(0, 0, 255)); // Symbol 3 in Blau
}

// Funktion zur Überprüfung des Gewinns
void checkWin() {
  uint16_t color1 = tft.readPixel(symbol1X + symbolWidth / 2, symbol1Y + symbolHeight / 2);
  uint16_t color2 = tft.readPixel(symbol2X + symbolWidth / 2, symbol2Y + symbolHeight / 2);
  uint16_t color3 = tft.readPixel(symbol3X + symbolWidth / 2, symbol3Y + symbolHeight / 2);

  int payout = 0;

  if (color1 == color2 && color2 == color3) {
    if (color1 == color565(255, 0, 0)) {
      statusText = "JACKPOT"; // Status-Text auf "JACKPOT" setzen
    } else {
      statusText = "WINNER!"; // Status-Text auf "WINNER!" setzen
    }
    winMelody = true;
    loseMelody = false;

    // Berechnung des Gewinns
    payout = calculatePayout(color1, color2, color3, true); // Berechnung des Gewinns bei 3 gleichen Farben

  } else if (color1 == color2 || color1 == color3 || color2 == color3) {
    statusText = "WINNER!"; // Status-Text "WINNER!"
    winMelody = true;
    loseMelody = false;

    // Berechnung des Gewinns
    payout = calculatePayout(color1, color2, color3, false); // Berechnung des Gewinns bei 2 gleichen Farben

  } else {
    statusText = "TRY AGAIN"; // Status-Text "TRY AGAIN"
    winMelody = false;
    loseMelody = true;
    payout = 0;
  }

  if (currentPlayerIndex != -1) {
    // Credit aktualisieren
    players[currentPlayerIndex].credit -= 100; // Einsatz abziehen

    // Überprüfen ob der Gewinn größer ist als der Einsatz
    if (payout > 100) {
      players[currentPlayerIndex].credit += payout; // Gewinn hinzufügen
      if (statusText != "JACKPOT") {
        statusText = "WINNER!";
      }
      winMelody = true;
      loseMelody = false;
      displayButtons(); // Buttons anzeigen, wenn Gewinn größer als Einsatz
    } else {
      statusText = "TRY AGAIN";
      winMelody = false;
      loseMelody = true;
      // Buttons nicht anzeigen, wenn Gewinn kleiner als Einsatz
    }

    // Serial Monitor Output
    Serial.print("+ ");
    Serial.println(payout);
    Serial.print("Neuer Credit für ");
    Serial.print(players[currentPlayerIndex].name); // Verwendet den Namen des aktuellen Spielers (Player X)
    Serial.print(": ");
    Serial.println(players[currentPlayerIndex].credit);
    Serial.println();
  } else {
    Serial.println("Kein Spieler registriert!");
  }

  statusChanged = true; // Status geändert

}

// Funktion zum Berechnen des Gewinns basierend auf den Farben
int calculatePayout(uint16_t color1, uint16_t color2, uint16_t color3, bool threeSame) {
  int payout = 0;

  // Multiplikatoren
  float multiplier2 = 1.5f;
  float multiplier3 = 2.5f;

  // Farben Werte
  int valueRed = 500;
  int valueGreen = 200;
  int valueBlue = 100;
  int valueYellow = 75;
  int valueMagenta = 50;
  int valueWhite = 10;
  int valueBlack = 5;

  if (threeSame) {
    if (color1 == color565(255, 0, 0)) payout = valueRed * multiplier3;
    else if (color1 == color565(0, 255, 0)) payout = valueGreen * multiplier3;
    else if (color1 == color565(0, 0, 255)) payout = valueBlue * multiplier3;
    else if (color1 == color565(255, 255, 0)) payout = valueYellow * multiplier3;
    else if (color1 == color565(255, 0, 255)) payout = valueMagenta * multiplier3;
    else if (color1 == color565(255, 255, 255)) payout = valueWhite * multiplier3;
    else if (color1 == color565(0, 0, 0)) payout = valueBlack * multiplier3;
  } else {
    if (color1 == color2 || color1 == color3) {
      if (color1 == color565(255, 0, 0)) payout = valueRed * multiplier2;
      else if (color1 == color565(0, 255, 0)) payout = valueGreen * multiplier2;
      else if (color1 == color565(0, 0, 255)) payout = valueBlue * multiplier2;
      else if (color1 == color565(255, 255, 0)) payout = valueYellow * multiplier2;
      else if (color1 == color565(255, 0, 255)) payout = valueMagenta * multiplier2;
      else if (color1 == color565(255, 255, 255)) payout = valueWhite * multiplier2;
      else if (color1 == color565(0, 0, 0)) payout = valueBlack * multiplier2;
    } else if (color2 == color3) {
      if (color2 == color565(255, 0, 0)) payout = valueRed * multiplier2;
      else if (color2 == color565(0, 255, 0)) payout = valueGreen * multiplier2;
      else if (color2 == color565(0, 0, 255)) payout = valueBlue * multiplier2;
      else if (color2 == color565(255, 255, 0)) payout = valueYellow * multiplier2;
      else if (color2 == color565(255, 0, 255)) payout = valueMagenta * multiplier2;
      else if (color2 == color565(255, 255, 255)) payout = valueWhite * multiplier2;
      else if (color2 == color565(0, 0, 0)) payout = valueBlack * multiplier2;
    }
  }

  return payout;
}

// Funktion fürs Spiel Zurücksetzen
void resetGame() {
  stopSymbols = false;
  statusText = "STOP";
  statusChanged = true;
  tft.fillScreen(color565(0, 0, 0)); // Bildschirm löschen
  displaySlotMachine(); // Slotmaschine erneut anzeigen
  displayPlayerInfo(currentPlayerIndex); // Spielerinformationen wieder anzeigen
}

// Funktion zum Auszahlen des Gewinns und Beenden des Spiels
void cashOut() {
  tft.fillScreen(color565(0, 0, 0));
  tft.setTextColor(color565(255, 255, 255));
  tft.setTextSize(3);
  tft.setCursor(60, 100);
  tft.print("CASH OUT");
  Serial.println("CASH OUT");

  // Speichern des aktualisierten Spielercredits
  Serial.print("Player ");
  Serial.print(currentPlayerIndex + 1);
  Serial.print(": Credit = ");
  Serial.println(players[currentPlayerIndex].credit);

  // Reset des aktuellen Spielers
  currentPlayerIndex = -1;
}

// Funktion zur Wiedergabe der Gewinnmelodie
void playWinMelody() {
  for (int repeat = 0; repeat <= 110; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(956);
    digitalWrite(pin, LOW);
    delayMicroseconds(956);
  }
  delay(230);
  for (int repeat = 0; repeat <= 70; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1276);
    digitalWrite(pin, LOW);
    delayMicroseconds(1276);
  }
  delay(10);
  for (int repeat = 0; repeat <= 70; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1276);
    digitalWrite(pin, LOW);
    delayMicroseconds(1276);
  }
  delay(50);
  for (int repeat = 0; repeat <= 100; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1136);
    digitalWrite(pin, LOW);
    delayMicroseconds(1136);
  }
  delay(200);
  for (int repeat = 0; repeat <= 100; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1276);
    digitalWrite(pin, LOW);
    delayMicroseconds(1276);
  }
  delay(600);
  for (int repeat = 0; repeat <= 100; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1012);
    digitalWrite(pin, LOW);
    delayMicroseconds(1012);
  }
  delay(210);
  for (int repeat = 0; repeat <= 100; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(956);
    digitalWrite(pin, LOW);
    delayMicroseconds(956);
  }
}

// Funktion zur Wiedergabe der Verlustmelodie
void playLoseMelody() {
  for (int repeat = 0; repeat <= 60; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(3597);
    digitalWrite(pin, LOW);
    delayMicroseconds(3597);
  }
  delay(50);
  for (int repeat = 0; repeat <= 60; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(3817);
    digitalWrite(pin, LOW);
    delayMicroseconds(3817);
  }
  delay(50);
  for (int repeat = 0; repeat <= 60; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(4065);
    digitalWrite(pin, LOW);
    delayMicroseconds(4065);
  }
  delay(50);
  for (int repeat = 0; repeat <= 160; repeat++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(4274);
    digitalWrite(pin, LOW);
    delayMicroseconds(4274);
  }
}


int findPlayer(byte *uid) {
  for (int i = 0; i < playerCount; i++) {
    if (memcmp(players[i].uid, uid, 4) == 0) {
      return i;
    }
  }
  return -1;
}

// new player mit 1000 credits
void storeNewPlayer(byte *uid, int bonus) {
  memcpy(players[playerCount].uid, uid, 4);
  players[playerCount].credit = 1000 + bonus; // Anfangscresit mit Bonus
  sprintf(players[playerCount].name, "Player %d", playerCount + 1);
  playerCount++;

  // Print new player info
  displayPlayerInfo(playerCount - 1);
}


 // player info anzeigen

void displayPlayerInfo(int playerIndex) {
  // Hintergrund für Spielerinformationen löschent
  tft.fillRect(0, 280, TFT_WIDTH, 40, color565(0, 0, 0));
  
  // Spielerinformationen anzeigen
  tft.setTextColor(color565(255, 255, 255));
  tft.setTextSize(2);
  tft.setCursor(10, 280);
  tft.print(players[playerIndex].name);
  tft.println();
  tft.setCursor(10, 300);
  tft.print("Credit: ");
  tft.print(players[playerIndex].credit);
}


 // "Scan Card" anzeigen
 
void displayScanCard() {
    tft.fillRect(0, 200, TFT_WIDTH, 40, color565(0, 0, 0)); // Bereich für Text löschen
    tft.setTextColor(color565(255, 255, 255)); // Textfarbe Weiß
    tft.setTextSize(3); // Textgröße
    tft.setCursor(40, 200); // Textposition

    if (isAdminCardScanned) {
        if (!isAdminCardScanned){
          tft.print("STOP");
        }
    } else {
        tft.print("Scan Card");
    }
}



// Hex Zahlen zu Serial umwandeln

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}


