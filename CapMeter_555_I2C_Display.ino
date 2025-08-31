//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// For the Serial out (must match dropdown selection)
const long baudRate = 115200;

// Pin that senses 555 timer output (can be any digital input)
const byte inputPin = 9;

// unsigned long previousCountMillis = millis();
// const long countMillis = 1000L;

// Max time to sense output pulse high/low before timing out
long pulseIn_timeout = -1;

// R1 connects from VCC to Discharge
const double R1 = 1000.0;
// R2 connects from Discharge to Threshold/capacitor
double R2 = -1.0;
// Calculated capacitance
double capacitance = 0.0;

// Digital pins for mux selector (can be any digital pins)
const byte A = 10;
const byte B = 11;
const byte C = 12;
const byte inhibit = 13;

// Concatenating mux code
byte muxCode[] = {0, 0, 0};

// Which resistor in the resistorValues array is currently enabled
byte currentEnabledResistor = -1;
const unsigned long resistorValues[] = {10e6, 1e6, 100e3, 10e3, 1e3, 100e0, 10e0, 0};
// const unsigned long resistorValues[] = {0e0, 10e0, 100e0, 1e3, 10e3, 100e3, 1e6, 10e6};

// Scaling calculated capacitance (micro = 1e6, nano = 1e9, pico = 1e12)
unsigned long long capacitanceScalar = 1;
// For pico measurements, compensating observed capacitance added by circuit itself
const float parasitic_capacitance = 50.0;

// 555 timer out high pulse sense duration
unsigned long highTime = 0;
// 555 timer out low pulse sense duration
unsigned long lowTime = 0;
// 555 timer out total signal period
unsigned long period = 0;
// 555 timer out total frequency (1/period)
float frequency = 0.0;

// Disable the mux entirely
void disableMux() {
  digitalWrite(inhibit, HIGH);
}

// Enable the multiplexer
void enableMux() {
  digitalWrite(inhibit, LOW);
}

/// @brief Set the resistor to enable
/// @param pinToEnable which resistor to enable
void enableResistor(int pinToEnable) {
  if (pinToEnable > 7) {
    for (int i = 0; i < 4; i++) {
      muxCode[i] = 0;
    }

    Serial.print("'nERROR: ");
    Serial.print(pinToEnable);
    Serial.println(" is not in the range of 0-7");
  } else {
    int manualBinary = pinToEnable;
    manualBinary = -(manualBinary-7);

    for (int i = 0; i < 4; i++) {
      muxCode[i] = manualBinary % 2;
      manualBinary /= 2;
    }
    
    if (muxCode[0]) {
      digitalWrite(A, HIGH);
    } else {
      digitalWrite(A, LOW);
    }

    if (muxCode[1]) {
      digitalWrite(B, HIGH);
    } else {
      digitalWrite(B, LOW);
    }
    if (muxCode[2]) {
      digitalWrite(C, HIGH);
    } else {
      digitalWrite(C, LOW);
    }

    currentEnabledResistor = pinToEnable;
    Serial.print("CurrentEnabledResistor: ");
    Serial.print(currentEnabledResistor);
    R2 = resistorValues[pinToEnable];
  }
}

// Switch to the next pin, increment
void switchToNextPin() {
  currentEnabledResistor++;
  currentEnabledResistor %= 8;
  // Serial.print("Switching to resistor: ");
  // Serial.println(currentEnabledResistor);
  enableResistor(currentEnabledResistor);
  Serial.print("Switched to pin ");
  Serial.println(currentEnabledResistor);
}

// Clear the frequency and capacitance displayed
void resetDisplay(){
  // Row 0
  lcd.setCursor(0, 0);
  lcd.print("                    ");
  lcd.setCursor(0, 0);
  lcd.print("Frequency");

  // Row 1
  lcd.setCursor(0, 1);
  lcd.print("                    ");

  // Row 2
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("Capacitance");

  // Row 3
  lcd.setCursor(0, 3);
  lcd.print("                    ");
}

// Display measured frequency and capacitance, as well as enabled resistors (both Serial and I2C display)
// All variables are global, so no parameters needed
void displayInfo() {
  Serial.print("Capacitance Scalar: ");
  Serial.println(capacitanceScalar/1e6); 

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print("                    "); // 20 spaces b/c 20x4
  lcd.setCursor(0, 1);
  lcd.print(frequency);
  lcd.print(" Hz");
  lcd.print(" R2: "); // 20 spaces b/c 20x4
  lcd.print(R2);
  lcd.setCursor(0, 3);
  lcd.print("                "); // 20 spaces b/c 20x4
  lcd.setCursor(0, 3);
  lcd.print(capacitance);

  // show Hz on Serial too if available
  Serial.println("");
  Serial.println(period); 
  Serial.println("");
  Serial.print(frequency); 
  Serial.println(" Hz");
  Serial.print(capacitance);

  if (capacitanceScalar == 1e6) {
    Serial.println(" uF");
    lcd.print(" uF");
  } else if (capacitanceScalar == 1e9) {
    Serial.println(" nF");
    lcd.print(" nF");
  } else {
    Serial.println(" pF");
    lcd.print(" pF");
  }

  Serial.print("R1: ");
  Serial.println(R1); 
  Serial.print("R2: ");
  Serial.println(R2);
}

void setup() {

  // Set up mux pins
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(inhibit, OUTPUT);

  // Set up LCD
  lcd.init();
  // Turn backlight on (default off)
  lcd.backlight();

  resetDisplay();

  enableMux();
  enableResistor(3);
}

void loop() {
  /// You MUST!! begin serial every time loop runs!
  Serial.begin(baudRate);

  // resetDisplay();

  // Set the appropriate timeout
  if (currentEnabledResistor < 6) {
    pulseIn_timeout = 500e3; // 500ms
  } else if (currentEnabledResistor == 6) {
    pulseIn_timeout = 800e3; // 800ms
  } else {
    pulseIn_timeout = 10e6; // 10s
  }

  // sense pulse high time and low time
  highTime = pulseIn(inputPin, HIGH, pulseIn_timeout);
  lowTime = pulseIn(inputPin, LOW, pulseIn_timeout);

  // The total period is the sum of the HIGH and LOW times.
  period = highTime + lowTime;

  Serial.println(""); // IDK why, but if this line isn't here, program stalls at enabling resistor 4 specifically

  if (period > 15) {
    // Serial.println(period);
    frequency = 1000000.0 / period;

    if (frequency < 20 && currentEnabledResistor < 6) {
      resetDisplay();
      switchToNextPin();
      // return;
    } else {


      if (currentEnabledResistor > 4) {
        // Micro
        capacitanceScalar = 1e6;
      } else if (currentEnabledResistor < 2) {
        // Pico
        capacitanceScalar = 1e12;
      } else {
        // Nano
        capacitanceScalar = 1e9;
      }
      
      /*
        Use the rearranged 555 timer formula to calculate capacitance.
        C = 1.44 / ((R1 + 2 * R2) * f)
        
        R2 + 700 b/c I ran calculations, and measured frequency wasn't matching up with
        what it should be based on Nx555 data sheet
        https://www.ti.com/lit/ds/symlink/ne555.pdf
      */
      capacitance = (1.44 * capacitanceScalar) / ((R1 + 2.0 * (R2+700)) * frequency);
      capacitance += capacitance * 0.1;

      // If pico, subtract board's parasitic capacitance
      if (capacitanceScalar == 1e12) {
        capacitance -= parasitic_capacitance;
      }
      
      // Serial.print(1.44 / ((R1 + 2.0 * R2) * frequency));
      
      // Capacitance, frequency, and resistors are all global variables, so display has all info already
      resetDisplay();
      displayInfo();
      delay(100);
      // switchToNextPin();
    }

  } else {
    Serial.println("\nERROR: Period is out of range");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Measuring...");
    switchToNextPin();
  }
}