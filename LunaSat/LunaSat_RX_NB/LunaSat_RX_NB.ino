/*
  Transmitter code
  Description: LunaSat Reciever
  Date Created: 07/18/2023

  LED1 flashing: UART output
  LED2 flashing: RF recieved data
*/


#include "TMP117.h"
#include "MPU6000.h"
#include "MLX90395.h"
#include "CAP.h"
#include "TPIS1385.h"

#include <RadioLib.h>

#define NSS_PIN 10
#define DIO1_PIN 3
#define DIO0_PIN 2
#define RESET_PIN 9

SX1272 radio = new Module(NSS_PIN, DIO0_PIN, RESET_PIN, DIO1_PIN);

// 23094
// LunaRadio Rad;
TMP117 thermometer(1, false); //22940
MPU6000 accelerometer(2, false);//22998
MLX90395 magnetometer(3, false);//20904
CAP capacitive(4, false);//23044
TPIS1385 thermopile(5, false);//23086

// RF id
const String RF_ID = "2A";
const String DEVICE_ID = "2B";

// Delay to start RF
const unsigned long WAIT_HOUR = 2;
const unsigned long WAIT_MIN = 30;
const unsigned long WAIT_SECOND = 60;
const unsigned long WAIT_MILLSEC = 1000;
const unsigned long DELAY_START = WAIT_MILLSEC * WAIT_SECOND * WAIT_MIN * WAIT_HOUR;
bool rf_enable = false;

// Pin Definitions
const int LED1_PIN = 4;
const int LED2_PIN = 5;

// Sensor Constants
const int TIME_BETWEEN_SAMPLE = 2000; 

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
  
  initializeSensors();
}

void loop() {
  // unsigned long st = millis();
  
  // Enable RF
  if ( !rf_enable & (millis() > DELAY_START) ){
    rf_enable = true;
    radio.begin();
    radio.setFrequency(915.0);
    radio.setOutputPower(17);
    radio.setBandwidth(250);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(8);
  }

  digitalWrite(LED1_PIN, HIGH);
  SensorData data;
  readSensorData(data);

  digitalWrite(LED1_PIN, LOW);
  printSensorData(data);

  

  // RF disable will add some delay for LED
  if (!rf_enable){
    unsigned long elapsedTime = millis() - data.timestamp;
    unsigned long remainingTime = TIME_BETWEEN_SAMPLE - elapsedTime;
    // Reading data time is longer than time between sample
    if (elapsedTime > TIME_BETWEEN_SAMPLE){
      Serial.print(F("***** Error: elapsedTime > TIME_BETWEEN_SAMPLE! ")); 
      Serial.print(elapsedTime); Serial.print(F(" > ")); Serial.print(TIME_BETWEEN_SAMPLE); 
      Serial.println(F(" *****"));
      delay(TIME_BETWEEN_SAMPLE);
    }
    else {
      delay(remainingTime);
    }
  }
  // Serial.println(millis() - st);
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
  Serial.print(F(",RFRX,"));Serial.print(rf_enable);
  
  if (rf_enable){
    // RF recieve
    digitalWrite(LED2_PIN, LOW);
    String tmp_str;
    radio.receive(tmp_str);
    if (tmp_str.substring(0,2) == RF_ID){
      Serial.print(F(",MsgRX,")); Serial.print(tmp_str);
      Serial.print(F(",Strength,")); Serial.print(radio.getRSSI());
      digitalWrite(LED2_PIN, HIGH);
    }
  }

  Serial.println();
}