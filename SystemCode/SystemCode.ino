#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial bluetoothSerial(10, 11);

const int sonarTriggerPin = 4;
const int sonarEchoPin = 3;
const int flowSensorPin = 5;
const int speedSensorPin = 2;

// Fuel and flow variables
float FuelLevel = 0;
float FuelInserted = 0;
float initialFuelLevel = 0;

float TotalFuel = 0;

// Speed and distance variables
volatile int holes = 0;
unsigned long lastHoleTime = 0;
const float distancePerHole = 0.025;
float totalDistance = 0;
bool isCarMoving = false;

// Flow sensor variables
volatile int flowPulseCount = 0;
const float pulsesPerLiter = 450.0;
const float timeInterval = 1.0;

void setup() {
  Serial.begin(9600);
  bluetoothSerial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("***Fuel theft***");
  lcd.setCursor(0, 1);
  lcd.print("prevention system");

  pinMode(speedSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(speedSensorPin), countHoles, FALLING);

  pinMode(sonarTriggerPin, OUTPUT);
  pinMode(sonarEchoPin, INPUT);

  pinMode(flowSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), countFlowPulses, FALLING);

  delay(2000);
  lcd.clear();
}

void loop() {
  calculateFlowRate();
  if (FuelInserted > 0) {
    lcd.clear();
    calculateAndPrintFlowRate();
    delay(100);
  } else{

  unsigned long currentTime = millis();
  isCarMoving = (currentTime - lastHoleTime) <= 5000;
  if (!isCarMoving) {
    if (initialFuelLevel == 0) {
      initialFuelLevel = getFuelLevel();
    } else {
      FuelLevel = getFuelLevel();
      if (initialFuelLevel - FuelLevel >= 1) {
        handleFuelLevelDrop();
        delay(5000);
      }
    }
    printFuelLevel();
    printTotalFuel();
    delay(2000);

    if (totalDistance != 0) {
      float fuelConsumed = (FuelLevel - initialFuelLevel) * 41.67;
      float fuelShouldConsumed = totalDistance * 3.33;  //per 15 meter distace, fuel cost is 3.33 ml.
      if (fuelConsumed > fuelShouldConsumed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("***Fuel consumed");
        lcd.setCursor(0, 1);
        lcd.print("more then limit!");
        bluetoothSerial.print("!!!! Fuel consumed more then limit. please check the engine\n");
        delay(5000);
      }
    }
    totalDistance = 0;
  } else {
    initialFuelLevel = 0;
  }

  printCarStatus(currentTime);
  }
}

void printCarStatus(unsigned long currentTime) {
  lcd.setCursor(0, 0);
  lcd.print(isCarMoving ? "Car: Moving     " : "Car: Stopped    ");
  // bluetoothSerial.print(isCarMoving ? "Car: Moving\n" : "Car: Stopped\n");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(totalDistance, 2);
  lcd.print(" m        ");
  // bluetoothSerial.print("Distance: ");
  // bluetoothSerial.print(totalDistance);
  // bluetoothSerial.print(" m\n");
}

void printFuelLevel() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("FLevel: ");
  lcd.print(FuelLevel, 2);
  lcd.print(" cm");
  // bluetoothSerial.print("Fuel Level: ");
  // bluetoothSerial.print(FuelLevel);
  // bluetoothSerial.print(" cm\n");
}
void printTotalFuel() {
  lcd.setCursor(0, 1);
  lcd.print("TFuel: ");
  lcd.print(TotalFuel, 2);
  lcd.print(" ml");
  // bluetoothSerial.print("Total Fuel: ");
  // bluetoothSerial.print(TotalFuel);
  // bluetoothSerial.print(" ml\n");
}

void countHoles() {
  holes++;
  lastHoleTime = millis();
  totalDistance += distancePerHole;
}

void countFlowPulses() {
  flowPulseCount++;
}

float getFuelLevel() {
  digitalWrite(sonarTriggerPin, LOW);
  delayMicroseconds(5);
  digitalWrite(sonarTriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sonarTriggerPin, LOW);

  float duration = pulseIn(sonarEchoPin, HIGH);
  float invertedFuelLevel = 9.65 - (duration * 34300.0 / 2.0 / 1000000.0);

  // Ensure the inverted fuel level is within bounds
  if (invertedFuelLevel < 0) {
    TotalFuel = 0;
    return 0;
  } else if (invertedFuelLevel > 9.65) {
    TotalFuel = 402.11;
    return 9.65;
  } else {
    TotalFuel = invertedFuelLevel * 41.67;
    return invertedFuelLevel;
  }
}

void handleFuelLevelDrop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!!Theft Suspect!!");
  lcd.setCursor(0, 1);
  lcd.print("Fuel Decreasing..");
  bluetoothSerial.print("!!!!!!!!!!!!!!\nALERT\n!!!!!!!!!!!!!!!\n Suspicious oil\n theft detected\n-----------------\n");
}

void calculateFlowRate() {
  int X = pulseIn(flowSensorPin, HIGH);
  int Y = pulseIn(flowSensorPin, LOW);
  float TIME = X + Y;
  float FREQUENCY = 1000000 / TIME;
  float WATER = FREQUENCY / 7.5;
  float LS = WATER / 60;

  if (FREQUENCY >= 0 && !isinf(FREQUENCY)) {
    FuelInserted += LS;
    delay(100);
  }else {
    if(FuelInserted>0){
    delay(3000);
    }
    FuelInserted = 0;
  }
}
void calculateAndPrintFlowRate() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("...Refueling... ");
  lcd.setCursor(0, 1);
  lcd.print("Fuel: ");
  lcd.print(FuelInserted, 2);
  lcd.print(" ml  ");
  // bluetoothSerial.print("Fuel in: ");
  // bluetoothSerial.print(FuelInserted);
  // bluetoothSerial.print(" ml\n");
}
