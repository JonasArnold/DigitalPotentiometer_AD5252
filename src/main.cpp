#include <Arduino.h>
#include <Wire.h>

///** PIN DEFINITIONS **///

// ESP8266 (NodeMcu V1 Pins)
#define PIN_WIRE_SDA (4) // SDA = GPIO4 = D2
#define PIN_WIRE_SCL (5) // SCL = GPIO5 = D1


///** CONSTANTS **///

// AD5252 I2C base address is 0x2C(44)
#define I2C_Address 0x2C
// resistance from a to b pin on AD5252
#define R_AB_OHM 1080
// Disables Serial when set to 0
#define DEBUG 1


///** HELPER FUNCTIONS **///

/// @brief converts 8bit unsigned integer to resistance
/// @param value 0...255
/// @return resistance [ohm] 
float uint8ToResistance(uint8_t value){

  return (((float)(256-value))/256.0)*R_AB_OHM;
}

/// @brief converts resistance to 8bit unsigned integer
/// @param value resistance [ohm]
/// @return 0...255
uint8_t resistanceToUint8(float value){
  // parameter guarding
  if(value > R_AB_OHM || value < 0){
#if DEBUG
    Serial.println("'resistanceToFloat()' Parameter out of range");
#endif
    return 127;  //return something other than 0 or 255
  }
  return (256 - (uint8_t)((value*256.0)/R_AB_OHM));
}

/// @brief sets RDAC on AD5252 using I2C
/// @param channel can be 1 / 3 for RDAC1 or RDAC3
/// @param value 0...255 
/// @return 0 on success
uint8_t writeRDAC(uint8_t channel, uint8_t value){
  // parameter guarding
  if(channel != 1 && channel != 3){
#if DEBUG
    Serial.println("'writeRDAC()' channel out of range");
#endif
    return 1; // error
  }

  Wire.beginTransmission(I2C_Address);
  // Select channel with R/~W = 0, CMD/~REG = 0, EE/~RDAC = 0
  Wire.write(channel);
  // Set 8 bit wiper setting
  Wire.write(value);
  // make transmission and check for errors
  byte error = Wire.endTransmission();
  if(error != 0){
#if DEBUG
    Serial.print("'writeRDAC()' got following error code from I2C transmission: ");
    Serial.println(error);
#endif
    return error;
  }

#if DEBUG
  // print update
  Serial.print("'writeRDAC()' set RDAC");
  Serial.print(channel);
  Serial.print(" to ");
  Serial.print(uint8ToResistance(value));
  Serial.print(" Ohm (raw=");
  Serial.print(value);
  Serial.println(")");
#endif

  return 0; // success
}

/// @brief reads RDAC value from EEMEM on AD5252 using I2C
/// @param channel can be 1 / 3 for RDAC1 or RDAC3
/// @param value pointer to where value will be stored
/// @return 0 on success
uint8_t readRDAC(uint8_t channel, uint8_t *value){
  // parameter guarding
  if(channel != 1 && channel != 3){
#if DEBUG
    Serial.println("'readRDAC()' channel out of range");
#endif
    return 1; // error
  }

  Wire.beginTransmission(I2C_Address);
  // Select channel with R/~W = 0, CMD/~REG = 0, EE/~RDAC = 0
  Wire.write(channel);
  // Stop I2C transmission
  Wire.endTransmission();
  // make transmission and check for errors
  byte error = Wire.endTransmission();
  if(error != 0){
#if DEBUG
    Serial.print("'readRDAC()' got following error code from I2C transmission (select channel): ");
    Serial.println(error);
#endif
    return error;
  }

  // Request 1 byte of data (first byte value of previously selected channel)
  Wire.requestFrom(I2C_Address, 1);

  int data = 0;
  if(Wire.available() == 1)
  {
    data = Wire.read();
  }

  // check if data has valid range (should have, only 1 byte requested...)
  if(data > 255 || data < 0){
#if DEBUG
    Serial.print("'readRDAC()' got invalid data from chip.");
#endif
    return 2; // error
  }

  // cast and save data to pointer
  uint8_t data_uint8_t  = (uint8_t)data;
  *value = data_uint8_t;
#if DEBUG
    Serial.print("'readRDAC()' got value from RDAC");
    Serial.print(channel);
    Serial.print(": ");
    Serial.print(uint8ToResistance(data_uint8_t));
    Serial.print(" Ohm (raw=");
    Serial.print(data_uint8_t);
    Serial.println(")");
#endif

  return 0;  // success
}


void setup() {
  // Initialise I2C communication as Master
  Wire.begin();
  // Initialise serial communication, set baud rate to 9600
  Serial.begin(9600);
}

void loop() {
  // test loop that increases potentiometer value in 100 ohm steps
  static float potValue_Ohm = 500.0;
  uint8_t readPotValue = 0;

  writeRDAC(1, resistanceToUint8(potValue_Ohm));
#if DEBUG
  Serial.print("Loop set potentiometer value to: ");
  Serial.println(potValue_Ohm);
#endif

  readRDAC(1, &readPotValue);
#if DEBUG
  Serial.print("Loop got potentiometer value from chip: ");
  Serial.print(readPotValue);
  Serial.print("\n\n");
#endif

  // increase or reset value
  if(potValue_Ohm + 100 < R_AB_OHM){
    potValue_Ohm += 100;
  } else{
    potValue_Ohm = 0;
  }

  delay(2000);
}