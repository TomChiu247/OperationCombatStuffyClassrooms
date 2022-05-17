#include <dht.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LCD.h> 
//#include <EEPROM.h>


#define DHT11_PIN A0
const int PWMPin = 3;

float temperature;
float autoMode = 0; // true = fan controlled by temp readings and thresholds, false = fan controlled manually
float lowTemperature = 22.0;
float highTemperature = 24.0;
float manualSpeed = 40; 

const int fan_control_pin = 3;
int count;
unsigned long start_time;
int rpm;
int highFanSpeed = 79;
int lowFanSpeed = 30;
int currentSpeed = 79;
int desiredSpeed = 1;
boolean fanStop = LOW; // HIGH = fan stops, LOW = fan doesn't stop
boolean backLightLED = HIGH; // HIGH = on, LOW = off
float displaySpeed;

//Input & Button Logic
const int numOfInputs = 4;
const int inputPins[numOfInputs] = {8,9,10,11};
int inputState[numOfInputs];
int lastInputState[numOfInputs] = {LOW,LOW,LOW,LOW};
bool inputFlags[numOfInputs] = {LOW,LOW,LOW,LOW};
long lastDebounceTime[numOfInputs] = {0,0,0,0};
long debounceDelay = 5;

//LCD Menu Logic
const int numOfScreens = 5;
int currentScreen = 0;
String screens[numOfScreens][2] = {{"Main Menu","Temperature"}, {"Auto(0) / Manual(1)", "Mode"}, {"Fan Low Temp", "Temperature (C)"}, 
{"Fan High Temp","Temperature (C)"}, {"Manual Set Speed","Speed (%)"}};
float parameters[numOfScreens] = {temperature, autoMode, lowTemperature, highTemperature, manualSpeed};

dht DHT; 
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7);

void setup() {
  
  // generate 25kHz PWM pulse rate on Pin 3
  pinMode(PWMPin, OUTPUT);   // OCR2B sets duty cycle
  // Set up Fast PWM on Pin 3
  TCCR2A = 0x23;     // COM2B1, WGM21, WGM20 
  // Set prescaler  
  TCCR2B = 0x0A;   // WGM21, Prescaler = /8
  // Set TOP and initialize duty cycle to zero(0)
  OCR2A = 79;    // Sets PWM Pulse Rate 
  OCR2B = 0;    // duty cycle for Pin 3 (0-79) generates 1 500nS pulse even when 0 

  for(int i = 0; i < numOfInputs; i++) {
    pinMode(inputPins[i], INPUT);
    digitalWrite(inputPins[i], HIGH); // pull-up 20k
  }

  lcd.begin(20,4);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  int chk = DHT.read11(DHT11_PIN);
  temperature = DHT.temperature;

  if (temperature > highTemperature) {
    if (fanStop) {
      OCR2B = 0;
    }
    else {
      OCR2B = currentSpeed;
    }
  }

  if (temperature < lowTemperature) {
    OCR2B = 0;
  }
  
  setInputFlags();
  resolveInputFlags();
}

void setInputFlags() {
  for(int i = 0; i < numOfInputs; i++) {
    int reading = digitalRead(inputPins[i]);
    if (reading != lastInputState[i]) {
      lastDebounceTime[i] = millis();
    }
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != inputState[i]) {
        inputState[i] = reading;
        if (inputState[i] == HIGH) {
          inputFlags[i] = HIGH;
        }
      }
    }
    lastInputState[i] = reading;
  }
}

void resolveInputFlags() {
  for(int i = 0; i < numOfInputs; i++) {
    if(inputFlags[i] == HIGH) {
      inputAction(i);
      inputFlags[i] = LOW;
      printScreen();
    }
  }
}

void inputAction(int input) {
  if(input == 0) {
    if (currentScreen == 0) {
      currentScreen = numOfScreens-1;
    }else{
      currentScreen--;
    }
  }else if(input == 1) {
    if (currentScreen == numOfScreens-1) {
      currentScreen = 0;
    }else{
      currentScreen++;
    }
  }else if(input == 2) {
    parameterChange(0);
  }else if(input == 3) {
    parameterChange(1);
  }
}

void parameterChange(int key) {
  if(key == 0) {
    parameters[currentScreen]++;
  }else if(key == 1) {
    parameters[currentScreen]--;
  }
}

void printScreen() {
  lcd.clear();
  lcd.print(screens[currentScreen][0]);
  lcd.setCursor(0,1);
  lcd.print(parameters[currentScreen]);
  lcd.print(" ");
  lcd.print(screens[currentScreen][1]);
}
