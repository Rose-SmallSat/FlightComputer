// Arduino master sketch

#include <Wire.h>
/* Pins */
int HEATER_PIN=8;
int ADDR_PIN=13; //address pin (ADD0)
/* Addresses */
#define SENSOR_ADDR 0x4F // I2C address of temp sensor
#define CONFIG_REG_ADDR 0x01 //address of the internal configuration register
byte TEMP_REG_ADDR=0b00000000; //address of the internal temp register
byte TLOW_REG_ADDR=0b0000010; //address of the internal TLOW register
byte THIGH_REG_ADDR=0b0000011; //address of the internal THIGH register
/* Config Register Options */
const byte SHUTDOWN_MODE=0b00000001;
//
const byte THERMOSTAT_MODE=0b00000010;
//
const byte POLARITY=0b00000100;
// number of faults
const byte ONE_FAULT=0b00000000;
const byte TWO_FAULT=0b00001000;
const byte FOUR_FAULT=0b00010000;
const byte SIX_FAULT=0b00011000;
// resolution
const byte NINE_BIT_RES=0b00000000;
const byte TEN_BIT_RES=0b00100000;
const byte ELEVEN_BIT_RES=0b01000000;
const byte TWELVE_BIT_RES=0b01100000;
//
const byte ONE_SHOT=0b10000000;
/* Global constants */
#define CONFIG_VALUE 0x60
/* Global variables */
byte i2c_rcv;               // data received from I2C bus
unsigned long time_start;   // start time in milliseconds
float temperature_celsius;
unsigned int data[2];

void setup()
{
  Wire.begin(); // join I2C bus as the master
  Serial.begin(9600);

  temperature_celsius=20;
  i2c_rcv = 255;
  time_start = millis();
  
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(ADDR_PIN,OUTPUT);
  
  Wire.beginTransmission(0x4F); // informs the bus that we will be sending data to device 8 (0x08)
  Wire.write(0x01);
  Wire.write(0x60);
  Wire.endTransmission();
  delay(300);
}// end setup

void loop()
{
  Wire.beginTransmission(0x4F);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(0x4F,2);
  Serial.println("bruh");
  if(Wire.available()==2)
  {  
    data[0]=Wire.read();
    data[1]=Wire.read();
    temperature_celsius = (((data[0] * 256) + (data[1] & 0xF0)) / 16) * 0.0625;
    Serial.println("Wire Available!");
    Serial.println("Temperature: "+String(temperature_celsius)+" C"); 
  }
  delay(500);
}// end loop
