# 🎰 Arduino Slotmachine Game

A slot machine game built with Arduino, RFID card system, and touchscreen interface. Players use RFID cards to log in and manage credits. The game simulates a traditional slot machine using colored blocks to represent symbols. Winning combinations trigger audio feedback and credit payouts based on color match logic.

## 🛠 Features

- 📇 **RFID Authentication**: Register and track players with unique RFID cards
- 📱 **Touchscreen UI**: Spin, cash out, and interact with the game using a touch display
- 🎨 **Color-Based Symbols**: Randomized colored blocks simulate slot machine reels
- 🔊 **Audio Feedback**: Win and lose melodies via a buzzer
- 🎲 **Payout Logic**: Multipliers based on matching 2 or 3 colors
- 💾 **Player Data Storage**: Credit tracking per RFID user

## ⚙️ Tech Stack

- Arduino UNO / Mega
- C++ (Arduino IDE)
- MFRC522 RFID Reader
- TFT LCD Touchscreen
- Buzzer
- SPI Communication

## 🎮 Gameplay Overview

1. **Scan Card**: Player logs in using their RFID card
2. **Spin**: Tap the touchscreen to start the slot
3. **Match Colors**: If 2 or 3 colors match, the player wins a multiplier
4. **Sound Effects**: Buzzer plays winning or losing tone
5. **Cash Out**: Players can cash out anytime

## 🧠 Learnings & Challenges

- Implemented color-matching logic using raw RGB values
- Created interactive UI with dynamic screen updates
- Managed EEPROM-like credit tracking across sessions
- Developed modular code with maintainability in mind

## 📸 Screenshots

*(Add images of the slot machine UI, hardware setup, or game in action)*

## 📂 Repository Structure

