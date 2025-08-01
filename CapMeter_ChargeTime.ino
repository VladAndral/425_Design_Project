/*

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 https://docs.arduino.cc/learn/electronics/lcd-displays

*/

// include the library code:
#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/*
  My code
*/

// const byte capVoltagePin = 0; // Sensing pin
const byte r100Pin = 6; // Power pins
const byte r10kPin = 7;
const byte r100kPin = 8;
const byte r1MPin = 9;
const byte r10MPin = 10;

const byte pinArray[] = {r100Pin, r10kPin, r100kPin, r1MPin, r10MPin};
// const byte pinArray[] = {r100Pin, r10kPin};
const unsigned long resistorValues[] = {100, 10000, 100000, 1000000, 10000000};
int pinEnable = 0;

const double tauVoltage = (5*0.632);

double currVoltage;

unsigned long startTime = -1;
unsigned long endTime = -1;
const int maxChargeTime = 500; // Check if millis or micros

void dischargeCapacitor() {
  pinMode(r100Pin, OUTPUT);
  digitalWrite(r100Pin , LOW);
  delay(100);
  pinMode(r100Pin, INPUT_PULLUP);
}

void switchToNextPin() {
  int pinArraySize = sizeof(pinArray) / sizeof(pinArray[0]);
  
  pinEnable++;
  pinEnable = pinEnable % pinArraySize;
  Serial.print("Switching to pin ");
  Serial.println(pinEnable + 6);

  for (int i = 0; i < pinArraySize; i++) {
    pinMode(pinArray[i], OUTPUT);
    digitalWrite(pinArray[i], LOW);
  } 

  for (int i = 0; i < pinArraySize; i++) {
    if (i == pinEnable) {
      digitalWrite(pinArray[i], HIGH);
    } else {
      pinMode(pinArray[i], INPUT_PULLUP);
      // digitalWrite(pinArray[i], LOW);
    }
  }
  // dischargeCapacitor();
}


void setup() {

  // Baud rate MUST match this one!
  Serial.begin(500000);

  pinMode(r100Pin, INPUT_PULLUP);
  pinMode(r10kPin, OUTPUT);
  pinMode(r100kPin, INPUT_PULLUP);
  pinMode(r1MPin, INPUT_PULLUP);
  pinMode(r10MPin, INPUT_PULLUP);

  dischargeCapacitor();

  digitalWrite(r100Pin, HIGH);

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  lcd.print("Frequency");

}

void loop() {
  
  if (startTime == -1) {
    startTime = millis(); // try with micros()
  }

  currVoltage = (5.0/1023.0)*analogRead(A0);

  endTime = millis();

  if (currVoltage >= tauVoltage) {
    Serial.print("\nCap Voltage is: ");
    Serial.println(currVoltage);
    Serial.print("And tauVoltage is: ");
    Serial.println(tauVoltage);

    if (endTime - startTime < 10) {
      Serial.print("\nTook too short, time was ");
      Serial.println(startTime - endTime);
      dischargeCapacitor();
      switchToNextPin();
      startTime = -1;
    } else {
      // Serial.println(currVoltage);
      double tau = (endTime - startTime)/1.0;
      unsigned long R = resistorValues[pinEnable];

      double C = (tau/R)*1000;
      Serial.print("\n****************");
      Serial.print("\nCapacitance is ");
      Serial.print(C);
      Serial.print("µF");
      Serial.print(" for pin ");
      Serial.println(pinEnable + 6);
      Serial.print("Tau was: ");
      Serial.println(tau);
      Serial.print("R was: ");
      Serial.println(R);
      Serial.println("");

      dischargeCapacitor();
      switchToNextPin();
      delay(100);
      startTime = -1;
    }
  } else if (endTime >= startTime + maxChargeTime) {
    Serial.print("\nTook too long, charge was ");
    Serial.println(currVoltage);
    Serial.print("End time = ");
    Serial.println(endTime);
    Serial.println("Start time + maxChargeTime = ");
    Serial.print(startTime);
    Serial.print(", ");
    Serial.println(maxChargeTime);

    dischargeCapacitor();
    switchToNextPin();
    startTime = -1;
    delay(2000);
  }
}
