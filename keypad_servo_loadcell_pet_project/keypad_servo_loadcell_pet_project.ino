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
float weight = 0;

int pos = 0;  // variable to store the servo position
int posDegrees = 0, posStop;
int inc = 1;
int period = 0;
bool servo_on = 0;
uint8_t state = 0;

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
    Serial.println("press:" + key);
    if (state == 0) {
      if (key == '1') {
        servo_on = 1;
        posStop = weight - BUTTON1;
      } else if (key == '2') {
        servo_on = 1;
        posStop = weight - BUTTON2;
      } else if (key == '3') {
        servo_on = 1;
        posStop = weight - BUTTON3;
      } else if (key == '4') {
        servo_on = 1;
        posStop = weight - BUTTON4;
      }
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 2000 && !servo_on) {  // idle
    previousMillis = currentMillis;

    weight = scale.get_units(2);
    Serial.println("Weight: " + String(weight) + " g");

  } else if (currentMillis - previousMillis >= 500 && servo_on) {  // number mode
    previousMillis = currentMillis;

    weight = scale.get_units(2);
    Serial.println("Weight: " + String(weight) + " g\tStop weight: " + String(posStop));

    if (weight <= posStop) {
      posStop = 0;
      servo_on = 0;
      Serial.println("Stop!!");
    }
  }


  unsigned long currentServo = millis();
  if (currentServo - previousServo >= 25) {
    previousServo = currentServo;

    if (servo_on) {
      servo1.write(posDegrees);
      period++;
      if (period >= 20) {
        period = 0;
        if (posDegrees == 180) posDegrees = 0;
        else posDegrees = 180;
      }
    }
  }
}
