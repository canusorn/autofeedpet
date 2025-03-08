#define BUTTON1 45
#define BUTTON2 70
#define BUTTON3 85
#define BUTTON4 190

#include "HX711.h"
#include <Keypad.h>
#include <ESP32Servo.h>

HX711 scale;

//  adjust pins if needed
uint8_t dataPin = 23;
uint8_t clockPin = 22;

Servo servo1;  // Create a Servo object

const byte ROWS = 4;  //four rows
const byte COLS = 4;  //three columns
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 19, 18, 5, 17 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 16, 4, 0, 2 };    //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned long previousMillis = 0, previousServo;

int pos = 0;  // variable to store the servo position
int posDegrees = 0;
int inc = 1;
int period = 0;
bool servo_on = 0;

void setup() {
  Serial.begin(115200);
  servo1.attach(12);
  scale.begin(dataPin, clockPin);
  //  load cell factor 5 KG
  scale.set_scale(380);  //  TODO you need to calibrate this yourself.
  //  reset the scale to zero = 0
  scale.tare();
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    if (key == '1') {
      servo_on = 1;
      posDegrees = 0;
    } else if (key == '9') {
      servo_on = 1;
      posDegrees = 180;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 2000) {
    previousMillis = currentMillis;

    Serial.println(scale.get_units(2));
  }


  unsigned long currentServo = millis();
  if (currentServo - previousServo >= 25) {
    previousServo = currentServo;

    if (servo_on) {
      servo1.write(posDegrees);
      period++;
      if (period >= 30) {
        period = 0;
        if (posDegrees == 180) posDegrees = 0;
        else posDegrees = 180;
      }
    }
  }
}
