#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- PIN CONFIGURATION ----------

#define VOLTAGE_SENSOR_PIN 34
#define CURRENT_SENSOR_PIN 35
#define SOLAR_SENSOR_PIN   32
#define EB_SENSOR_PIN      33

#define BMS_STATUS_PIN     25
#define RELAY_PIN          26
#define EMERGENCY_PIN      27

#define TEMP_SENSOR_PIN    4

// ---------- LCD ----------

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- TEMPERATURE SENSOR ----------

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);

// ---------- SAFETY LIMITS ----------

const float MAX_TEMPERATURE = 55.0;
const float WARNING_TEMPERATURE = 45.0;

const float MAX_CURRENT = 10.0;
const float MAX_VOLTAGE = 250.0;

// ---------- VARIABLES ----------

float voltage = 0.0;
float current = 0.0;
float temperature = 0.0;

float solarVoltage = 0.0;
float ebVoltage = 0.0;

String chargingMode = "OFF";

// ---------- SETUP ----------

void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(EMERGENCY_PIN, INPUT_PULLUP);
  pinMode(BMS_STATUS_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, LOW);

  lcd.init();
  lcd.backlight();

  tempSensor.begin();

  lcd.setCursor(0, 0);
  lcd.print("VOLTARA EV");
  lcd.setCursor(0, 1);
  lcd.print("System Start");

  delay(2000);
  lcd.clear();
}

// ---------- MAIN LOOP ----------

void loop() {

  // Read sensors
  readSensors();

  // Emergency switch
  if (digitalRead(EMERGENCY_PIN) == LOW) {
    stopCharging("EMERGENCY");
    return;
  }

  // BMS protection status
  if (digitalRead(BMS_STATUS_PIN) == LOW) {
    stopCharging("BMS FAULT");
    return;
  }

  // Temperature protection
  if (temperature >= MAX_TEMPERATURE) {
    stopCharging("TEMP HIGH");
    return;
  }

  // Over-current protection
  if (current > MAX_CURRENT) {
    stopCharging("OVER CURRENT");
    return;
  }

  // Over-voltage protection
  if (voltage > MAX_VOLTAGE) {
    stopCharging("OVER VOLTAGE");
    return;
  }

  // Temperature warning
  if (temperature >= WARNING_TEMPERATURE) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TEMP WARNING");

    lcd.setCursor(0, 1);
    lcd.print(temperature, 1);
    lcd.print(" C");

    Serial.println("Warning: High temperature");

    delay(1500);
  }

  // Select charging source
  selectChargingMode();

  // Display information
  displayData();

  // Serial monitoring
  printData();

  delay(1000);
}

// ---------- SENSOR READING ----------

void readSensors() {

  tempSensor.requestTemperatures();

  temperature = tempSensor.getTempCByIndex(0);

  voltage = readVoltage();

  current = readCurrent();

  solarVoltage = readSolarVoltage();

  ebVoltage = readEBVoltage();
}

// ---------- VOLTAGE SENSOR ----------

float readVoltage() {

  int rawValue = analogRead(VOLTAGE_SENSOR_PIN);

  // Prototype conversion.
  // Calibrate according to your actual voltage sensor.
  float measuredVoltage =
      (rawValue / 4095.0) * 250.0;

  return measuredVoltage;
}

// ---------- CURRENT SENSOR ----------

float readCurrent() {

  int rawValue = analogRead(CURRENT_SENSOR_PIN);

  // Prototype conversion.
  // Calibrate according to actual INA219/ACS712 sensor.
  float measuredCurrent =
      (rawValue / 4095.0) * 15.0;

  return measuredCurrent;
}

// ---------- SOLAR SENSOR ----------

float readSolarVoltage() {

  int rawValue = analogRead(SOLAR_SENSOR_PIN);

  float measuredSolarVoltage =
      (rawValue / 4095.0) * 12.0;

  return measuredSolarVoltage;
}

// ---------- EB SENSOR ----------

float readEBVoltage() {

  int rawValue = analogRead(EB_SENSOR_PIN);

  float measuredEBVoltage =
      (rawValue / 4095.0) * 250.0;

  return measuredEBVoltage;
}

// ---------- CHARGING MODE ----------

void selectChargingMode() {

  bool solarAvailable = solarVoltage > 4.0;
  bool ebAvailable = ebVoltage > 200.0;

  // Solar + EB Hybrid
  if (solarAvailable && ebAvailable) {

    chargingMode = "HYBRID";

    digitalWrite(RELAY_PIN, HIGH);
  }

  // Solar only
  else if (solarAvailable) {

    chargingMode = "SOLAR";

    digitalWrite(RELAY_PIN, HIGH);
  }

  // EB only
  else if (ebAvailable) {

    chargingMode = "EB";

    digitalWrite(RELAY_PIN, HIGH);
  }

  // No power
  else {

    chargingMode = "OFF";

    digitalWrite(RELAY_PIN, LOW);
  }
}

// ---------- LCD DISPLAY ----------

void displayData() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(chargingMode);

  lcd.setCursor(0, 1);

  lcd.print("V:");
  lcd.print(voltage, 0);

  lcd.print(" I:");
  lcd.print(current, 1);
}

// ---------- SAFETY SHUTDOWN ----------

void stopCharging(String reason) {

  digitalWrite(RELAY_PIN, LOW);

  chargingMode = "OFF";

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("CHARGING STOP");

  lcd.setCursor(0, 1);
  lcd.print(reason);

  Serial.print("Charging stopped: ");
  Serial.println(reason);

  delay(2000);
}

// ---------- SERIAL MONITOR ----------

void printData() {

  Serial.println("========== VOLTARA ==========");

  Serial.print("Charging Mode : ");
  Serial.println(chargingMode);

  Serial.print("Voltage       : ");
  Serial.print(voltage);
  Serial.println(" V");

  Serial.print("Current       : ");
  Serial.print(current);
  Serial.println(" A");

  Serial.print("Temperature   : ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Solar Voltage : ");
  Serial.print(solarVoltage);
  Serial.println(" V");

  Serial.print("EB Voltage    : ");
  Serial.print(ebVoltage);
  Serial.println(" V");

  Serial.println("=============================");
}
