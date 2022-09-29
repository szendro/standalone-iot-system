/*
 * SmartIOT Node
 * 
 * Version 1.00   15.09.2022    Christopher Collins
 * 
 * 
 * 
 * Hardware:  standalone bare bones ATM328P (int 8MHz XTAL) with Semtech SX1276 LoRa chip 
 *            connected via SPI.
 *            Uses DHT22 temperature/humidity sensor with data line connected to digital 
 *            pin 7 of ATM328P.
 *            A potentiometer is connected to ADC0 on pin 23 of ATM328P, but this could
 *            be any analogue input.
 *            A green LED is connected to digital pin 6 and a red LED to pin 5, and are 
 *            mainly used for debugging purposes as no serial monitor connection is possible.
 *            It would be inadviseable to use these in a battery powered node.
 *            
 * 
 * Operational note:
 *           The node connects to a Gateway using LoRa at 868 MHz for EU band. the Gateway in turn 
 *           connects to the Web UI using HTTP over WiFi.
 *           A transaction over LoRa is ALWAYS initiated by the node in order to minimise power 
 *           consumption by the node. The Gateway is likely to be mains powered.
 *           A transaction with the web may not complete in one loop if a command is issued to the node
 *           because the node loop and the web loop are asynchronous and the node is in sleep mode as
 *           much as possible.
 * 
 * SW functionality:
 *          1. The temp C/temp F/humidity are read from the DHT22 on a propritary serial bus, along with
 *          the reading from the ADC.
 *          2. These sensor values are packed inot a message frame and sent to the LoRa chip via SPI.
 *          3. The node waits for an ACK from the Gateway.
 *          4  The node then sends CMND_WINDOW_OPEN message to the gateway and listens for a command.
 *          5. If an LED_CMND is received the node turns the green LED ON/OFF, then sends an ACK.
 *          6. If a REG_CMND is received, the node reads the requested LoRa register from the SX1276
 *          and sends the value back to the Gateway.
 *          7. The SX1276 is put into sleep mode and the ATM328P enters low power mode for approx 25s.
 *          8. The loop restarts upon wakeup.
 * 
 *
 * Constuction of data packet Node -> Gateway
 *          1      Packet ID (Data)
 *          2      Field length
 *          3-4    ADC value (int)
 *          5      Field length
 *          6-9    Humidity (float)
 *          10     Field length
 *          11-14  Temperature C (float)
 *          15     Field length
 *          16-19  Temperature F (float)
 *          20     Field length
 *          21-24  RSSI (long)
 * 
 * 
 * 
 */


#include <SPI.h>
#include <LoRa.h>
#include <LowPower.h>
#include "DHT.h"
#include "smartiot.h"

#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321
#define DHTPIN 7          // Digital pin connected to the DHT sensor
DHT dht(DHTPIN, DHTTYPE); // Set up and create DHT object


// SPI pins
#define NSS 2
#define RST 3
#define DIO0 4
//#define DISPLAY_DEBUG 

void setup() {
 
 
  // LED's used initially for test purposes
  pinMode(RED_LED, OUTPUT); pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW);
  // Initialise DHT
  dht.begin();
  
  LoRa.setSPIFrequency(1000000);// sets spi clk divisor. 8MHz/1000000 = 8. So SPI clk = 1 MHz.
  LoRa.setPins(NSS, RST, DIO0); // NSS, RST, DIO0
  delay(50);
  if (!LoRa.begin(868E6)) {
    errorLed();
  }
  delay(3000); // wait 3s for DHT22 to settle before reading
}

// Loop runs then node goes into low power mode.
void loop() {
  byte packetType, cmndData, fieldLen, float_buf[4];
  int sensorVal, byteMask=0xff;
  static int resendCount=0; 
  static long lastTime = 0,lastRssi=NO_RSSI;
  static float hum=0.0, tempc=0.0, tempf=0.0; // make static so last value used in case of NaN
  float buf;

  // enter loop to send data until acknowledged by the gateway
  while(true){ 
    
    // Intermittently DHT22 returns NaN - could be because testing with low operating voltage (2.9 V)
    // In this case simply use the last correct value read 
    buf = dht.readHumidity();
    if(!isnan(buf)) hum = buf;
    // Read temperature as Celsius (the default)
    buf = dht.readTemperature();
    if(!isnan(buf)) tempc=buf;
    // Read temperature as Fahrenheit (isFahrenheit = true)
    buf = dht.readTemperature(true);
    if(!isnan(buf)) tempf=buf;

    // Currently just voltage from a pot via ADC
    sensorVal = analogRead(A0);
    
    // Construct packet
    LoRa.beginPacket();
    LoRa.write((byte)DATA);
    // ADC value
    fieldLen=sizeof(int);
    LoRa.write((byte)fieldLen);
    for(int i=0; i<fieldLen; i++){// LSB first
      LoRa.write((byte)((sensorVal>>i*8)&byteMask));
    }

    // Humidity
    fieldLen=sizeof(float);
    LoRa.write((byte)fieldLen);
    memcpy(float_buf, &hum, 4);
    for(int i=0; i<fieldLen; i++){// LSB first
      LoRa.write(float_buf[i]);
    }
    
    // Temperature C
    fieldLen=sizeof(float);
    LoRa.write((byte)fieldLen);
    memcpy(float_buf, &tempc, 4);
    for(int i=0; i<fieldLen; i++){// LSB first
      LoRa.write(float_buf[i]);
    }
   
    // Temperature F
    fieldLen=sizeof(float);
    LoRa.write((byte)fieldLen);
    memcpy(float_buf, &tempf, 4);
    for(int i=0; i<fieldLen; i++){// LSB first
      LoRa.write(float_buf[i]);
    }

    // RSSI
    if(lastRssi!=NO_RSSI){
      fieldLen=sizeof(long);
      LoRa.write((byte)fieldLen);
      for(int i=0; i<fieldLen; i++){// LSB first
        LoRa.write((byte)((lastRssi>>i*8)&byteMask));
      } 
    }
    else
      LoRa.write((byte)0);
      
    // Send packet  
    LoRa.endPacket();

    lastTime = millis();
    while (millis() - lastTime < ACK_TIMEOUT)
    {
      // wait for ack
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
          packetType=(byte)LoRa.read();
          lastRssi=LoRa.packetRssi();
          if(packetType==ACK){
#ifdef DISPLAY_DEBUG            
            BlinkRedLed();     
#endif               
            break;
          }
      }// if packetsize
    }//while ACK

    if(millis() - lastTime >= ACK_TIMEOUT){
#ifdef DISPLAY_DEBUG
          BlinkRedLed();
          BlinkRedLed();
#endif
    }
    break;
  }// while true
  
  // Tell gateway it is free to send commands
  LoRa.beginPacket();
  LoRa.write((byte)CMND_WINDOW_OPEN);
  LoRa.endPacket();
  
  lastTime=millis();
  while(millis()-lastTime<ACK_TIMEOUT){
    int packetSize=LoRa.parsePacket();
    if(packetSize){
      packetType=LoRa.read();
      if(packetType==LED_CMND){
        cmndData=(byte)LoRa.read();
        if(cmndData==GREEN_LED_ON){
          digitalWrite(GREEN_LED, HIGH);  
        }
        else if(cmndData==GREEN_LED_OFF){ 
          digitalWrite(GREEN_LED, LOW);  
        }
        LoRa.beginPacket();
        LoRa.write((byte)ACK);
        LoRa.endPacket();  
        break;      
      }
      if(packetType==REG_CMND){
        cmndData=(byte)LoRa.read();// Reg addr
        byte regVal=getRegister(cmndData);
        regVal=getRegister(cmndData);
        LoRa.beginPacket();
        LoRa.write((byte)REG_VALUE);
        LoRa.write(regVal);
        LoRa.write(cmndData);// so Gateway can check correct response
        LoRa.endPacket();        
        break;
      }
    }
  }

   delay(50); // give serial port enough time to print before sleep
 // Sleep LoRa (reports end draws less current than sleep ?)
  LoRa.end();
  LoRa.sleep();
  // Sleep ATMega
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

// Blink red led at 1s interval for ever.
void errorLed() {
  LoRa.end();
  while (1) {
    digitalWrite(RED_LED, HIGH);
    delay(1000);
    digitalWrite(RED_LED, LOW);
    delay(1000);
  }
}

// Blink green led for 100ms
void BlinkGreenLed()
{
  digitalWrite(GREEN_LED, HIGH);
  delay(50);
  digitalWrite(GREEN_LED, LOW);
  delay(50);
}

void BlinkRedLed() {
  digitalWrite(RED_LED, HIGH);
  delay(50);
  digitalWrite(RED_LED, LOW);
  delay(50);
}

byte getRegister(byte regAddr){
  regAddr &= 0x7f; // Ensure MSB = 0 for read
  digitalWrite(NSS, LOW);
  SPI.beginTransaction(SPISettings (1000000, MSBFIRST, SPI_MODE0));
  // Send the register address
  SPI.transfer(regAddr); 
  // Get register value
  byte regVal=SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(NSS, HIGH);
  return regVal;
}
