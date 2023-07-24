/*
  Description: One LunaSat conected to one RaspberryPi #4
  Date Created: 07/12/2023

*/

#include "TMP117.h"
#include "MPU6000.h"
#include "MLX90395.h"
#include "CAP.h"
#include "TPIS1385.h"

// Device ID
const String DEVICE_ID = "04";
bool rf_enable = false;

// Pin Definitions
// const int LED1_PIN = 4;
// const int LED2_PIN = 5;
const int LED1_PIN = 5;
const int LED2_PIN = 4;

// Sensor Constants
const int TIME_BETWEEN_SAMPLE = 1000;  // Time between samples in ms

// Sensor Objects
TMP117 thermometer(1, false);
MPU6000 accelerometer(2, false);
MLX90395 magnetometer(3, false);
CAP capacitive(4, false);
TPIS1385 thermopile(5, false);

// Data Structure to Hold Sensor Readings
struct SensorData {
  unsigned long timestamp;
  float temperatureC;
  float accelX;
  float accelY;
  float accelZ;
  float magX;
  float magY;
  float magZ;
  int capacitance;
  float objectTemp;
  float ambientTemp;
};

// Function Declarations
void initializeSensors();
void readSensorData(SensorData& data);
void printSensorData(const SensorData& data);

void setup() {
  Serial.begin(9600);
  
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  // digitalWrite(LED1_PIN, HIGH);

  initializeSensors();
}

void loop() {
  SensorData data;
  digitalWrite(LED2_PIN, HIGH);
  readSensorData(data);
  printSensorData(data);

  digitalWrite(LED2_PIN, LOW);

  unsigned long elapsedTime = millis() - data.timestamp;
  unsigned long remainingTime = TIME_BETWEEN_SAMPLE - elapsedTime;
  // 280-290 300ms
  // Serial.println(elapsedTime); 
  if (elapsedTime > TIME_BETWEEN_SAMPLE){
    Serial.print("***** Error: elapsedTime > TIME_BETWEEN_SAMPLE! "); 
    Serial.print(elapsedTime); Serial.print(" > "); Serial.print(TIME_BETWEEN_SAMPLE); 
    Serial.println(" *****");
    delay(TIME_BETWEEN_SAMPLE);
  }
  else {
    delay(remainingTime);
  }
}

void initializeSensors() {
  accelerometer.begin();
  accelerometer.initialize();
  accelerometer.setAccelRange(MPU6000_RANGE_2_G);

  magnetometer.begin_I2C();
  magnetometer.setGain(8);
  magnetometer.setResolution(MLX90395_RES_17, MLX90395_RES_17, MLX90395_RES_17);
  magnetometer.setOSR(MLX90395_OSR_4);
  magnetometer.setFilter(MLX90395_FILTER_8);
  
  capacitive.begin();
  thermopile.begin();
  thermopile.readEEprom();
}

void readSensorData(SensorData& data) {
  data.timestamp = millis();
  data.temperatureC = thermometer.getTemperatureC();
  data.accelX = accelerometer.getSample().x;
  data.accelY = accelerometer.getSample().y;
  data.accelZ = accelerometer.getSample().z;
  data.magX = magnetometer.getSample().magnetic.x;
  data.magY = magnetometer.getSample().magnetic.y;
  data.magZ = magnetometer.getSample().magnetic.z;
  data.capacitance = capacitive.getRawData();
  data.objectTemp = thermopile.getSample().object;
  data.ambientTemp = thermopile.getSample().ambient;
}

void printSensorData(const SensorData& data) {
  Serial.print(F("DEVICE_ID,"));Serial.print(DEVICE_ID);
  Serial.print(F(",Timestamp,"));Serial.print(data.timestamp);
  Serial.print(F(",TempC,"));Serial.print(data.temperatureC,5);
  Serial.print(F(",AccX,"));Serial.print(data.accelX, 5);
  Serial.print(F(",AccY,"));Serial.print(data.accelY, 5);
  Serial.print(F(",AccZ,"));Serial.print(data.accelZ, 5);
  Serial.print(F(",MagX,"));Serial.print(data.magX, 5);
  Serial.print(F(",MagY,"));Serial.print(data.magY, 5);
  Serial.print(F(",MagZ,"));Serial.print(data.magZ, 5);
  Serial.print(F(",Cap,"));Serial.print(data.capacitance);
  Serial.print(F(",ObjTemp,"));Serial.print(data.objectTemp, 5);
  Serial.print(F(",AmbTemp,"));Serial.print(data.ambientTemp, 5);
  Serial.print(F(",RFTX,"));Serial.print(rf_enable);

  Serial.println();
}

