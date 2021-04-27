
// Copyright (c) 2021, NSDAPS
// http://www.nsdaps.edu.ph/
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

// Link libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <stdarg.h>
#include "pitches.h"

// Global variables for RFID-RC522
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

// Global variables for LCD 1602
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Global variables for HC-SR04
const int trigPin = 7;
const int echoPin = 6;
long duration;
int distanceCm, distanceInch;

// LED SETUP
int redLed = 2;
int greenLed = 5;
int yellowLed = 4;

// Buzzer Setup
int speakerPin = 8;

void setup()
{
  Serial.begin(9600);
  SPI.begin();

  // Setup LCD
  lcd.begin();
  lcd.backlight();

  // Setup HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Setup RFID RC-522
  mfrc522.PCD_Init();

  // Setup Light Pins
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);

  // Setup Buzzer
  pinMode(speakerPin, OUTPUT);
}

void loop()
{
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  MFRC522::StatusCode status;
  byte block;
  byte len;
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;
  distanceInch = duration * 0.0133 / 2;

  lcd.clear();

  if (distanceCm >= 100)
  {
    String content = "";
    String idString = "";
    String surname = "";

    resetIndicators();

    digitalWrite(speakerPin, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Please Scan");
    lcd.setCursor(0, 1);
    lcd.print("your ID");
    digitalWrite(yellowLed, HIGH);

    if (!mfrc522.PICC_IsNewCardPresent())
    {
      delay(500);
      return;
    }

    if (!mfrc522.PICC_ReadCardSerial())
    {
      delay(500);
      return;
    }

    lcd.setCursor(0, 0);
    lcd.print("Verifying ID");
    lcd.setCursor(0, 1);
    lcd.print("Please wait...");
    digitalWrite(yellowLed, HIGH);

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();

    byte buffer1[18];
    block = 4;
    len = 18;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }

    status = mfrc522.MIFARE_Read(block, buffer1, &len);
    if (status != MFRC522::STATUS_OK)
    {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }

    for (uint8_t i = 0; i < 16; i++)
    {
      if (buffer1[i] != 32)
      {
        char tmp_idChar = char(buffer1[i]);
        if (tmp_idChar != '\n' && tmp_idChar != '\r')
        {
          idString += tmp_idChar;
        }
      }
    }

    byte buffer2[18];
    block = 1;

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
    if (status != MFRC522::STATUS_OK)
    {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }

    status = mfrc522.MIFARE_Read(block, buffer2, &len);
    if (status != MFRC522::STATUS_OK)
    {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }

    for (uint8_t i = 0; i < 16; i++)
    {
      char tmp_surnameChar = char(buffer2[i]);
      if (tmp_surnameChar != '\n' && tmp_surnameChar != '\r')
      {
        surname += tmp_surnameChar;
      }
    }

    String res = "";
    res.concat(content.substring(1) + "|");
    res.concat(idString + "|");
    res.concat(surname);

    if (idString == "" || surname == "")
    {
      resetIndicators();
      lcd.setCursor(0, 0);
      lcd.print("Invalid ID");
      lcd.setCursor(0, 1);
      lcd.print("Format");
      digitalWrite(redLed, HIGH);
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      tone(speakerPin, NOTE_G4);
      delay(1000);
      noTone(speakerPin);
      delay(500);
      tone(speakerPin, NOTE_G4);
      delay(1000);
      noTone(speakerPin);
      delay(500);
      tone(speakerPin, NOTE_G4);
      delay(1000);
      noTone(speakerPin);
      return;
    }

    Serial.println(res);

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    // Play a sound when a card was detected.
    tone(speakerPin, 350);
    delay(175);
    noTone(speakerPin);

    // Change LCD Screen
    resetIndicators();
    lcd.setCursor(0, 0);
    lcd.print("Awaiting for");
    lcd.setCursor(0, 1);
    lcd.print("Admin Response");

    while (true)
    {
      int boolResponseFromServer = Serial.read();
      if (boolResponseFromServer != -1 && boolResponseFromServer != 10)
      {
        if (boolResponseFromServer == 49)
        { // Indicator for 1
          resetIndicators();

          lcd.setCursor(0, 0);
          lcd.print(idString);
          lcd.setCursor(0, 1);
          lcd.print(String("Welcome, " + surname));
          digitalWrite(greenLed, HIGH);
          playSuccessSound();

          Serial.flush();
          break;
        }
        else if (boolResponseFromServer == 48)
        { // Indicator for 0
          resetIndicators();
          lcd.setCursor(0, 0);
          lcd.print("Unauthorized");
          lcd.setCursor(0, 1);
          lcd.print("Access");
          digitalWrite(redLed, HIGH);
          playErrorSound();
          Serial.flush();
          break;
        }
      }
    }
  }
  else
  {
    resetIndicators();
    digitalWrite(speakerPin, HIGH);
    digitalWrite(redLed, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Social Distancing");
    lcd.setCursor(0, 1);
    lcd.print("Not Observed");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Step Back");
    lcd.setCursor(0, 1);
    lcd.print(String(100 - distanceCm) + " cm away");
  }
  Serial.flush();
  delay(1000);
}

void playSuccessSound()
{
  digitalWrite(speakerPin, HIGH);
  delay(175);
  noTone(speakerPin);
  delay(300);
}

void playErrorSound()
{
  tone(speakerPin, NOTE_C3);
  delay(150);
  tone(speakerPin, NOTE_G4);
  delay(725);
  noTone(speakerPin);
  delay(700);
}

void resetIndicators()
{
  Serial.flush();
  lcd.clear();
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  noTone(speakerPin);
  digitalWrite(speakerPin, LOW);
}
