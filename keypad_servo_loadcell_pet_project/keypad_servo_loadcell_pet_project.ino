/*
LCD
SCL - 27
SDA - 26

servo motor
สายสีน้ำตาล -> GND
สายสีแดง -> 5V
สายสีส้ม -> 12

virtual pin

v0 - weight
v1 - g to feed

v2 - BUTTON1
v3 - BUTTON2
v4 - BUTTON3
v5 - BUTTON4
v6 - timer
v7 - tare
*/

#define BUTTON1 45
#define BUTTON2 70
#define BUTTON3 85
#define BUTTON4 190

#define LOWFOOD 300

#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6x62Fri32"
#define BLYNK_TEMPLATE_NAME "esp32 keypad feeding"
#define BLYNK_AUTH_TOKEN "v3VBnzocBMdBlKp7zT2RRfBtwVluyfAv"

#include "HX711.h"
#include <Keypad.h>
#include <ESP32Servo.h>
#include <LCD-I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "G6PD";
char pass[] = "570610193";

LCD_I2C lcd(0x27, 16, 2);  // Default address of most PCF8574 modules, change according

HX711 scale;

//  adjust pins if needed
uint8_t dataPin = 23;
uint8_t clockPin = 22;

Servo servo1;  // Create a Servo object

const byte ROWS = 4;  // four rows
const byte COLS = 4;  // three columns
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 19, 18, 5, 17 };  // connect to the row pinouts of the keypad
byte colPins[COLS] = { 16, 4, 0, 2 };    // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String number = "";

unsigned long previousMillis = 0, previousServo;
float weight = 0;

int pos = 0;  // variable to store the servo position
int posDegrees = 0, posStop;
int inc = 1;
int period = 0;
bool servo_on = 0, calebrate = 1;
uint8_t state = 0;  // 0-auto   1-manual

int blynk_feed = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(26, 27, 50000);
  lcd.begin(&Wire);

  lcd.setCursor(0, 0);
  lcd.print("Connecting blynk");
  lcd.display();
  lcd.backlight();

  servo1.attach(12);
  scale.begin(dataPin, clockPin);

  
  //  load cell factor 5 KG
  scale.set_scale(380);  //  TODO you need to calibrate this yourself.
  scale.set_offset(-286575);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  //  reset the scale to zero = 0
  // scale.tare();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight:");
  lcd.setCursor(15, 0);
  lcd.print("g");
  lcd.display();
}

void loop() {
  Blynk.run();

  onKeypad();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 2000 && !servo_on) {  // idle
    previousMillis = currentMillis;

    readWeightoffmotor();
  } else if (currentMillis - previousMillis >= 500 && servo_on) {  // number mode
    previousMillis = currentMillis;

    readWeightonmotor();
  }

  unsigned long currentServo = millis();
  if (currentServo - previousServo >= 20) {
    previousServo = currentServo;

    servo_run();
  }
}

void servo_run() {
  if (servo_on) {
    servo1.write(posDegrees);
    period++;
    if (period >= 10) {
      period = 0;
      if (posDegrees == 180)
        posDegrees = 0;
      else
        posDegrees = 180;
    }
  }
}

void onKeypad() {
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
      } else if (key == 'D') {  //  reset the scale to zero = 0
        scale.tare();
        calebrate = 1;
      } else if (key == 'A') {
        state = 1;
      }

      if (posStop < 0) {
        posStop = 0;
      }
    } else if (state == 1) {

      if (key >= '0' && key <= '9') {  // Check if it's a numeric key
        number += key;                 // Append the key to the number string
        Serial.println(number);        // Echo the key to the serial monitor
      } else if (key == '#') {         // If the '#' key is pressed, end input
        Serial.println("\nInput completed.");
        state = 0;
        int feedWeight = number.toInt();
        number = "";
        Serial.println("input:" + String(feedWeight));
        servo_on = 1;
        posStop = weight - feedWeight;
      } else if (key == '*') {  // Optional: Handle '*' as a backspace
        if (number.length() > 0) {
          number.remove(number.length() - 1);  // Remove the last character
        }
        Serial.println(number);  // Echo the key to the serial monitor
      }

      if (posStop < 0) {
        posStop = 0;
      }
    }

    previousMillis = 0;
  }
}

void readWeightoffmotor() {
  weight = scale.get_units(2);
  Serial.println("Weight: " + String(weight) + " g");
  Blynk.virtualWrite(V0, weight);

  if (calebrate == 1 && weight > LOWFOOD) {
    calebrate = 0;
  } else if (calebrate == 0 && weight < LOWFOOD) {
    Blynk.logEvent("low_food");
  }

  lcd.setCursor(8, 0);
  lcd.print("       ");
  lcd.setCursor(8, 0);
  lcd.print(String(weight, 0));
  lcd.setCursor(0, 1);
  lcd.print("                ");
  if (state == 1) {
    lcd.setCursor(0, 1);
    lcd.print("feed[g]: ");
    lcd.print(number);
  }
  lcd.display();
}

void readWeightonmotor() {
  weight = scale.get_units(2);
  Serial.println("Weight: " + String(weight) + " g\tStop weight: " + String(posStop));
  Blynk.virtualWrite(V0, weight);

  lcd.setCursor(8, 0);
  lcd.print("       ");
  lcd.setCursor(8, 0);
  lcd.print(String(weight, 0));

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("feeding (");
  lcd.print(String(posStop));
  lcd.print(")");

  if (weight <= posStop) {
    posStop = 0;
    servo_on = 0;
    Serial.println("Stop!!");

    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V5, 0);
    Blynk.virtualWrite(V6, 0);

    lcd.setCursor(0, 1);
    lcd.print("              ");
    lcd.setCursor(0, 1);
    lcd.print("Stop feeding");
  }

  lcd.display();
}

BLYNK_CONNECTED() {
  // Sync the value from V1 on startup
  Blynk.syncVirtual(V1);

  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 0);
  Blynk.virtualWrite(V4, 0);
  Blynk.virtualWrite(V5, 0);
  Blynk.virtualWrite(V6, 0);
}

// feeding in gram
BLYNK_WRITE(V1) {
  blynk_feed = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V1: " + String(blynk_feed));
}

BLYNK_WRITE(V2) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V2: " + String(pinValue));

  if (pinValue == 1) {
    servo_on = 1;
    posStop = weight - BUTTON1;
  } else {
    servo_on = 0;
  }
}

BLYNK_WRITE(V3) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V3: " + String(pinValue));

  if (pinValue == 1) {
    servo_on = 1;
    posStop = weight - BUTTON2;
  } else {
    servo_on = 0;
  }
}

BLYNK_WRITE(V4) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V4: " + String(pinValue));

  if (pinValue == 1) {
    servo_on = 1;
    posStop = weight - BUTTON3;
  } else {
    servo_on = 0;
  }
}

BLYNK_WRITE(V5) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V5: " + String(pinValue));

  if (pinValue == 1) {
    servo_on = 1;
    posStop = weight - BUTTON4;
  } else {
    servo_on = 0;
  }
}

BLYNK_WRITE(V6) {
  int pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
  Serial.println("V6: " + String(pinValue));

  if (pinValue) {
    servo_on = 1;
    posStop = weight - blynk_feed;
  } else {
    servo_on = 0;
  }
}

BLYNK_WRITE(V7) {
  scale.tare();
}