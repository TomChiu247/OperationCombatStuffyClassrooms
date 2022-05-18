#include <dht.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LCD.h>
#include <EEPROM.h>


#define DHT11_PIN A0
const int PWMPin = 3;

//General values used for logic
int currentSpeed = 79; //speed of fan
float temperature; //variable for storing temperature
float humidity; //variable for storing humidity

//Variables stored in EEPROM
float manualSpeed = 1; //speed that fan goes at, adjusted manually
float lowTemperature = 22.0; //if temp reaches below this temp, turn off fan
float highTemperature = 24.0; //if temp reaches above this temp, turn on fan
boolean autoMode = true; // true = fan controlled by temp readings and thresholds, false = fan controlled manually
boolean backLightOn = true; // true = backlight on display is tuned on, false = turned off
boolean inCelsius = true; //true = temperature displayed in celsius, false = in fahrenheit

//Input & Button Logic
const int numOfInputs = 4;
const int inputPins[numOfInputs] = {8, 9, 10, 11};
bool buttonState[numOfInputs] = {LOW, LOW, LOW, LOW};

//LCD Menu Logic
const int numOfScreens = 7;
int currentScreen = 0;
String screens[numOfScreens][2] = {{"Main Menu", "Temp Mode"}, {"Auto/Manual", "Mode"}, {"Low Temp", "Temperature"},
  {"High Temp", "Temperature"}, {"Set Speed", "Speed"}, {"Light ON/OFF", "Mode"}, {"Save Data","Last Action"}};

String parameters[numOfScreens];

dht DHT;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

void setup() {

  //Initialization of Fan PWM control
  //generate 25kHz PWM pulse rate on Pin 3
  pinMode(PWMPin, OUTPUT);   // OCR2B sets duty cycle
  //Set up Fast PWM on Pin 3
  TCCR2A = 0x23;     //COM2B1, WGM21, WGM20
  //Set prescaler
  TCCR2B = 0x0A;   //WGM21, Prescaler = /8
  //Set TOP and initialize duty cycle to zero(0)
  OCR2A = 79;    //Sets PWM Pulse Rate
  OCR2B = 0;    //duty cycle for Pin 3 (0-79) generates 1 500nS pulse even when 0

  //Setting up button pins
  for (int i = 0; i < numOfInputs; i++) {
    pinMode(inputPins[i], INPUT);
  }

  //Setting up led display
  lcd.begin(20, 4);
  lcd.setBacklightPin(3, POSITIVE);
  currentSpeed = 79;
  eepromRead();
  
  printScreen();
  //Loading in saved settings from last session, otherwise, using defaults
}

void loop() {
  //Get readings from the sensor
  int chk = DHT.read11(DHT11_PIN);
  humidity = DHT.humidity;

  //store temperature based on temperature mode setting
  if (inCelsius) {
    temperature = DHT.temperature;
  } else {
    temperature = (DHT.temperature * 1.8) + (32);
  }

  //operate fan based on autoMode setting
  if (autoMode == true || humidity > 50) {
    if (temperature >= highTemperature) {
      OCR2B = currentSpeed;
    }

    if (temperature < lowTemperature) {
      OCR2B = 0;
    }
  } else {
    OCR2B = currentSpeed;
  }

  //Look out for button presses
  resolveButtonPress();
}

//Debounce function to avoid false readings
boolean debounceButton(boolean state, int inputPin)
{
  boolean stateNow = digitalRead(inputPin);
  if (state != stateNow)
  {
    delay(10);
    stateNow = digitalRead(inputPin);
  }
  return stateNow;

}

//Resolve button presses
void resolveButtonPress() {
  for (int i = 0; i < numOfInputs; i++) {
    if (debounceButton(buttonState[i], inputPins[i]) == HIGH && buttonState[i] == LOW)
    {
      inputAction(i);
      buttonState[i] = HIGH;
      printScreen();
    }
    else if (debounceButton(buttonState[i], inputPins[i]) == LOW && buttonState[i] == HIGH)
    {
      buttonState[i] = LOW;
      printScreen();
    }
  }
}

//Respective changes made depending on the button press and which menu is displayed
void inputAction(int input) {
  if (input == 3) {
    if (currentScreen == 0) {
      currentScreen = numOfScreens - 1;
    } else {
      currentScreen--;
    }
  } else if (input == 2) {
    if (currentScreen == numOfScreens - 1) {
      currentScreen = 0;
    } else {
      currentScreen++;
    }
  } else if (input == 1) {
    parameterChange(0);
  } else if (input == 0) {
    parameterChange(1);
  }
}

void parameterChange(int key) {
  if (key == 0) {
    switch (currentScreen) {
      case 0:
        if (inCelsius) {
          inCelsius = true;
          parameters[currentScreen] = "Celcius";
          break;
        } else {
          inCelsius = true;
          parameters[currentScreen] = "Celcius";
          parameters[2] = String((lowTemperature - 32) * (5/9)) + "C";
          parameters[3] = String((highTemperature - 32) * (5/9)) + "C";
          break;
        }
      case 1:
          autoMode = true;
          parameters[currentScreen] = "Auto";
          break;
      case 2:
        if (inCelsius == true) {
          if (lowTemperature == 0) {
            parameters[currentScreen] = String(lowTemperature) + "C";
            break;
          } else {
            lowTemperature--;
            parameters[currentScreen] = String(lowTemperature) + "C";
            break;
          }
        } else {
          if (lowTemperature == 32) {
            parameters[currentScreen] = String(lowTemperature) + "F";
            break;
          } else {
            lowTemperature--;
            parameters[currentScreen] = String(lowTemperature) + "F";
            break;
          }
        }
      case 3:
        if (inCelsius == true) {
          if (highTemperature == 0) {
            parameters[currentScreen] = String(highTemperature) + "C";
            break;
          } else {
            highTemperature--;
            parameters[currentScreen] = String(highTemperature) + "C";
            break;
          }
        } else {
          if (highTemperature == 32) {
            parameters[currentScreen] = String(highTemperature) + "F";
            break;
          } else {
            highTemperature--;
            parameters[currentScreen] = String(highTemperature) + "F";
            break;
          }
        }
      case 4:
        if (manualSpeed == 0) {
          currentSpeed = int(79 * manualSpeed);
          parameters[currentScreen] = String(manualSpeed * 100) + "%";
          break;
        } else {
          manualSpeed = manualSpeed - 0.2;
          currentSpeed = int(79 * manualSpeed);
          parameters[currentScreen] = String(manualSpeed * 100) + "%";
          break;
        }
      case 5:
        if (backLightOn) {
          parameters[currentScreen] = "ON";
          break;
        } else {
          backLightOn = true;
          lcd.setBacklight(HIGH);
          parameters[currentScreen] = "ON";
          break;
        }
      case 6:
        eepromSave();
        parameters[currentScreen] = "Save";
        break;
      default:
        break;
    }
  } else if (key == 1) {
    switch (currentScreen) {
      case 0:
        if (inCelsius == true) {
          inCelsius = false;
          parameters[currentScreen] = "Fahrenheit";
          parameters[2] = String((lowTemperature * 1.8) + (32)) + "F";
          parameters[3] = String((highTemperature * 1.8) + (32)) + "F";
          break;
        } else {
          inCelsius = false;
          parameters[currentScreen] = "Fahrenheit";
          break;
        }
      case 1:
          autoMode = false;
          parameters[currentScreen] = "Manual";
          break;
      case 2:
        if (inCelsius == true) {
          if (lowTemperature == 50) {
            parameters[currentScreen] = String(lowTemperature) + " C";
            break;
          } else {
            lowTemperature++;
            parameters[currentScreen] = String(lowTemperature) + " C";
            break;
          }
        } else {
          if (lowTemperature == 122) {
            parameters[currentScreen] = String(lowTemperature) + " F";
            break;
          } else {
            lowTemperature++;
            parameters[currentScreen] = String(lowTemperature) + " F";
            break;
          }
        }
      case 3:
        if (inCelsius == true) {
          if (highTemperature == 50) {
            parameters[currentScreen] = String(highTemperature) + " C";
            break;
          } else {
            highTemperature++;
            parameters[currentScreen] = String(highTemperature) + " C";
            break;
          }
        } else {
          if (highTemperature == 122) {
            parameters[currentScreen] = String(highTemperature) + " F";
            break;
          } else {
            highTemperature++;
            parameters[currentScreen] = String(highTemperature) + " F";
            break;
          }
        }
      case 4:
        if (manualSpeed == 1) {
          currentSpeed = int(79 * manualSpeed);
          parameters[currentScreen] = String(manualSpeed * 100) + "%";
          break;
        } else {
          manualSpeed = manualSpeed + 0.2;
          currentSpeed = int(79 * manualSpeed);
          parameters[currentScreen] = String(manualSpeed * 100) + "%";
          break;
        }
      case 5:
        if (backLightOn) {
          backLightOn = false;
          lcd.setBacklight(LOW);
          parameters[currentScreen] = "OFF";
          break;
        } else {
          break;
        }
      case 6:
        eepromErase();
        parameters[currentScreen] = "Cleared";
        break;
      default:
        break;
    }
  }
}

//Function to update the liquid display screen
void printScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(screens[currentScreen][0]);
  lcd.print(" Fan:" + String(((currentSpeed * 100) / 79)) + "%");
  lcd.setCursor(0, 1);
  lcd.print("Tem:" + String(temperature, 1));

  if (inCelsius == true) {
    lcd.print("C ");
  } else {
    lcd.print("F ");
  }

  lcd.print("Humi:" + String(humidity, 1) + "%");
  lcd.setCursor(0, 2);
  lcd.print(screens[currentScreen][1]);
  lcd.print(" ");
  lcd.print(parameters[currentScreen]);
  lcd.setCursor(0, 3);
  switch (currentScreen) {
    case 0:
      lcd.print(" Toggle   Cel  Fahr ");
      break;
    case 1:
      lcd.print("     On     Off     ");
      break;
    case 2:
      lcd.print("      -      +      ");
      break;
    case 3:
      lcd.print("      -      +      ");
      break;
    case 4:
      lcd.print("      -      +      ");
      break;
    case 5:
      lcd.print("     On     Off     ");
      break;
    case 6:
      lcd.print("   Save     Clear   ");
    default:
      break;
  }
}

void eepromErase() {
  // routing not used in this program. Just here in case you want to clear the rom.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Erase EEPROM");
  lcd.setCursor(0, 1);
  lcd.print("Size= "); lcd.print(EEPROM.length());
  delay(1000);

  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

//Function to save data to eeprom, used EEPROM.put instead of .write to store values other than integers
void eepromSave() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving to EEPROM");

  //Spaced out addresses due to floats taking up more bytes 
  EEPROM.put(0, 254); // this is the value I'll be looking for to know this data is correct
  EEPROM.put(5, lowTemperature);
  EEPROM.put(10, highTemperature);
  EEPROM.put(15, manualSpeed);
  if (autoMode) {
    EEPROM.put(20, 1);
  } else {
    EEPROM.put(20, 0);
  }

  if (backLightOn) {
    EEPROM.put(25, 1);
  } else {
    EEPROM.put(25, 0);
  }

  if (inCelsius) {
    EEPROM.put(30, 1);
  } else {
    EEPROM.put(30, 0);
  }

  lcd.setCursor(0, 1);
  lcd.print("Done!");
  delay(1000);
}

//Function called on startup to load in saved settings and values from EEPROM
void eepromRead() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading EEPROM");

  if (EEPROM.read(0) == 254) {
    EEPROM.get(5, lowTemperature);
    EEPROM.get(10, highTemperature);
    EEPROM.get(15, manualSpeed);
    EEPROM.get(20, autoMode);
    EEPROM.get(25, backLightOn);
    EEPROM.get(30, inCelsius);
    delay(500);
    if (backLightOn) {
      lcd.setBacklight(HIGH);
      parameters[5] = "ON";
    } else {
      lcd.setBacklight(LOW);
      parameters[5] = "ON";
    }
    
    if (inCelsius) {
      inCelsius = true;
      parameters[0] = "Celcius";
      parameters[2] = String(lowTemperature) + " C";
      parameters[3] = String(highTemperature) + " C";
    } else {
      inCelsius = false;
      parameters[0] = "Fahrenheit";
      parameters[2] = String(lowTemperature) + " F";
      parameters[3] = String(highTemperature) + " F";
    }
   
    if (autoMode) {
      autoMode = true;
      parameters[1] = "Auto";
    } else {
      autoMode = false;
      parameters[1] = "Manual";
    }
    
    parameters[4] = String(manualSpeed * 100) + "%";
    
    lcd.setCursor(0, 1);
    lcd.print("Done!");
    delay(1000);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Defaults used.");
    delay(1000);
  }
}
