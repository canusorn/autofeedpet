/*
LCD
SCL - 27
SDA - 26


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
String number = "";

unsigned long previousMillis = 0, previousServo;
float weight = 0;

int pos = 0;  // variable to store the servo position
int posDegrees = 0, posStop;
int inc = 1;
int period = 0;
bool servo_on = 0, calebrate = 1;
uint8_t state = 0;  // 0-auto   1-manual

void setup() {
  Serial.begin(115200);

  Wire.begin(26, 27, 50000);
  lcd.begin(&Wire);
  lcd.print("Weight:");
  lcd.setCursor(15, 0);
  lcd.print("g");
  lcd.setCursor(0, 1);
  lcd.print("Connecting blynk");
  lcd.display();
  lcd.backlight();

  servo1.attach(12);
  scale.begin(dataPin, clockPin);
  //  load cell factor 5 KG
  scale.set_scale(380);  //  TODO you need to calibrate this yourself.
  //  reset the scale to zero = 0
  scale.tare();


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

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

      if (posStop < 0) { posStop = 0; }
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


      if (posStop < 0) { posStop = 0; }
    }

    previousMillis = 0;
  }


  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 2000 && !servo_on) {  // idle
    previousMillis = currentMillis;

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


  } else if (currentMillis - previousMillis >= 500 && servo_on) {  // number mode
    previousMillis = currentMillis;

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

      lcd.setCursor(0, 1);
      lcd.print("              ");
      lcd.setCursor(0, 1);
      lcd.print("Stop feeding");
    }

    lcd.display();
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