/*
 * SmartIOT Gateway
 * 
 * Version 1.00   15.09.2022    Christopher Collins
 * 
 * 
 * 
 * Hardware: ESP32 Dev Module connected to Semtech SX1276 via SPI 
 * 
 * Operational note:
 * The Gateway connects to the end node using LoRa at 868 MHz for EU band and connects to 
 * the Web UI using HTTP over WiFi.
 * A transaction over LoRa is ALWAYS initiated by the node in order to minimise power 
 * consumption by the node. The Gateway is likely to be mains powered.
 * A transaction with the web may not complete in one loop if a command is issued to the node
 * because the node loop and the web loop are asynchronous and the node is in sleep mode as
 * much as possible.
 * 
 * SW functionality:
 * 1. Loop waiting for the node to wake up and send data.
 * 2. Unpack the data and assign to sensor variables.
 * 3. Send ACK for data to node.
 * 4. Wait for message from node indicating it has opened command window.
 *    If no message within ACK_TIMEOUT or message corrupt skip to next loop waiting for data.
 * 5. Send LED_CMND to instruct node to turn ON/OFF LED.   
 * 6. Wait for ACK from node. if not received within ACK_TIMEOUT skip to next command.
 * 7. Send REG_CMND to instruct node to read a register from the SX1276 LoRa chip.
 * 8. Wait for ACK from node. if not received within ACK_TIMEOUT skip to next stage.
 * 9. Transact with UI on web server using HTTP request/response:
 *    Check requested state of LED.
 *    Check requested LoRa register.
 *    Send sensor data + LoRa register value.
 * 10.Return to waiting for data from node.   
 * 
 */

#include <SPI.h>
#include <LoRa.h>
#include <HTTPClient.h>
#include <WiFi.h>   
#include "smartiot.h"

#define DISPLAY_MESSAGES
#define DISPLAY_DEBUG




//Add WIFI data
const char* ssid = "mySSID";                    //Add your WIFI network name 
const char* password =  "myPASSWORD";           //Add WIFI password



long lastTime=0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Set up LoRa
  Serial.println("LoRa Receiver waiting for data");
  LoRa.setPins(13, 12, 14); // NSS, RST, DIO0

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Set up WiFi
  WiFi.begin(ssid, password); 
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected, my IP: ");
  Serial.println(WiFi.localIP());  

  lastTime=millis();
}

void loop() {
  byte packetType, fieldLen,ledCmnd, regCmnd, float_buf[4];
  static int adcVal=0, rssiVal=NO_RSSI; 
  static float humVal=0.0, tempcVal=0.0, tempfVal=0.0;
  int byteBuf=0, packetSize;
  static byte packet[MAX_PACKET_SIZE];
  static bool ledOn=true, dataReady=false, regvalueValid=false;
  static byte lastledCmnd=0xff, regValue, requestedRegAddr=0xff, readRegAddr=0xff;
  String httpData="";


  
 /* 
  *  Since we wait for data from the remote node, the duration of the loop is determined
  *  by the POWER_DOWN timer of the remote node
  *  
  */

    //
    // Communicate with remote DB first to get/set data ready for remote node
    //
    
    if(WiFi.status()==WL_CONNECTED){
      HTTPClient http; // http connection is closed each transaction

      // *** CHECK STATE OF LED SWITCH FROM WEBSITE ***
      // Open http connection to website, and prepare header for transactions
      http.begin("https://smartlora.000webhostapp.com/webcomms.php");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
      // Read the LED status from the remote DB
      httpData="check_LED_status=" + String(GREEN_LED);
      int responseCode=http.POST(httpData);
      if(responseCode>0){// respondeCode>0 means server sent response
        //Serial.println("Server response to check LED: " + String(responseCode));
        if(responseCode==200){ // 200 means response is good and echo data can be read
          String responseBody=http.getString();
          if(responseBody=="LED_is_on")
            ledCmnd=GREEN_LED_ON;
          else if(responseBody=="LED_is_off")
            ledCmnd=GREEN_LED_OFF;
#ifdef DISPLAY_MESSAGES            
          else
            Serial.println("ERROR - unrecognised LED status from web server: " + responseBody);    
#endif            
        }         
      }
      http.end(); // new http connection required for each transaction

      // *** CHECK LORA REGISTER SELECTED FROM WEBSITE ***
      // Open http connection to website, and prepare header for transactions
      http.begin("https://smartlora.000webhostapp.com/webcomms.php");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
      // Read the address of the selected LoRa register from the remote DB
      httpData="check_lora_reg";
      responseCode=http.POST(httpData);
      if(responseCode>0){// respondeCode>0 means server sent response
        if(responseCode==200){ // 200 means response is good and echo data can be read
          String responseBody = http.getString();
          requestedRegAddr = getregaddr(responseBody);
       }         
      }
      http.end(); // new http connection required for each transaction

      /* 
       *  Note: The read value will be one step behind the requested value when selected by user.
       *  This test prevents the value belonging to previous register being sent with the requested register.
       *  
       */
      if(readRegAddr==requestedRegAddr){
       // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Format response string for php   
        httpData="Lora_reg_value=" + String(regValue) + "&Lora_reg_addr=" + String(requestedRegAddr);
        responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed data: " + http.getString());
#endif            
          }         
        }
        http.end();   
      }  
            
      // only send sensor data when already received from node
      if(dataReady==true){
        // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
           
        httpData="Adc_Data=" + String(adcVal);
        responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed data: " + http.getString());
#endif            
          }         
        }
        http.end();   

        // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
           
        httpData="Hum_Data=" + String(humVal);
        responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed data: " + http.getString());
#endif            
          }         
        }
        http.end();   

        // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
           
        httpData="Tempc_Data=" + String(tempcVal);
        responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed data: " + http.getString());
#endif            
          }         
        }
        http.end();   

        // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
           
        httpData="Tempf_Data=" + String(tempfVal);
        responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed data: " + http.getString());
#endif            
          }         
        }
        http.end();   
      }// Data ready
      
      // send RSSI if attached
      if(rssiVal!=NO_RSSI){
        // Open http connection to website, and prepare header for transactions
        http.begin("https://smartlora.000webhostapp.com/webcomms.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
           
        httpData="Rssi_Data=" + String(rssiVal);
        int responseCode=http.POST(httpData);
        if(responseCode>0){// respondeCode>0 means server sent response
          if(responseCode==200){ // 200 means response is good and echo data can be read
#ifdef DISPLAY_MESSAGES            
            Serial.println("Server ehoed RSSI: " + http.getString());
#endif            
          }         
        }
        http.end();      
      }

      
    }// http transaction is now complete. Now attend to LoRa connection...

  // Loop waiting for the node to send data
   while(true){  
     // try to parse packet
     packetSize = LoRa.parsePacket();
     // Code execution is blocked until a packet is received from remote node 
     if(packetSize) {
        if(packetSize>MAX_PACKET_SIZE){
#ifdef DISPLAY_DEBUG          
          Serial.print("Packet skipped. Size too big: ");
          Serial.println(packetSize);
#endif          
          break;
        }
        
        packetType=(byte)LoRa.read();
        //Data from node is sync start
        if(packetType!=DATA)
          continue;

        // Read ADC value
        adcVal=0;
        fieldLen=(byte)LoRa.read();
        if(fieldLen>MAX_FIELD_LEN){
#ifdef DISPLAY_DEBUG          
          Serial.println("ERROR - fieldLen to large in ADC field");
#endif          
          break;
        }
        for(int i=0; i<fieldLen; i++){// LSB first
          byteBuf=(byte)LoRa.read();
          adcVal=adcVal|(byteBuf<<i*8);
        }
        // Read humiidity
        fieldLen=(byte)LoRa.read();
        if(fieldLen>MAX_FIELD_LEN){
#ifdef DISPLAY_DEBUG          
          Serial.println("ERROR - fieldLen to large in HUM field");
#endif          
          break;
        }
        for(int i=0; i<fieldLen; i++){// LSB first
          float_buf[i]=(byte)LoRa.read();
        }
        humVal= *((float *)float_buf);
        
        // Read temp C
        fieldLen=(byte)LoRa.read();
        if(fieldLen>MAX_FIELD_LEN){
#ifdef DISPLAY_DEBUG          
          Serial.println("ERROR - fieldLen to large in TEMPC field");
#endif          
          break;
        }
        for(int i=0; i<fieldLen; i++){// LSB first
          float_buf[i]=(byte)LoRa.read();
        }
        tempcVal= *((float *)float_buf);
        
        // Read temp F
        fieldLen=(byte)LoRa.read();
        if(fieldLen>MAX_FIELD_LEN){
#ifdef DISPLAY_DEBUG          
          Serial.println("ERROR - fieldLen to large in TEMPF field");
#endif          
          break;
        }
        for(int i=0; i<fieldLen; i++){// LSB first
          float_buf[i]=(byte)LoRa.read();
        }
        tempfVal= *((float *)float_buf);
        
        // Check if RSSI attached
        rssiVal=0;
        fieldLen=(byte)LoRa.read();          
        if(fieldLen>MAX_FIELD_LEN){
#ifdef DISPLAY_DEBUG          
          Serial.println("ERROR - fieldLen to large in ADC field");
#endif          
          break;
        }
        if(fieldLen!=0){
          for(int i=0; i<fieldLen; i++){// LSB first
            byteBuf=(byte)LoRa.read();
            rssiVal=rssiVal|(byteBuf<<i*8);
          }
        }
        else
          rssiVal=NO_RSSI;
          
        dataReady=true;
        break;
     }   
   } // while waiting data    
        
  // Send ACK to data. 
  LoRa.beginPacket();
  LoRa.write((byte)ACK);
  LoRa.endPacket();
  
  // Wait for node to open command window     
  lastTime=millis();
 // while(millis()-lastTime<ACK_TIMEOUT){
 while(true){
     packetSize = LoRa.parsePacket();
     if(packetSize) {
          if(packetSize>MAX_PACKET_SIZE){
#ifdef DISPLAY_DEBUG            
            Serial.print("Packet size too big: ");
            Serial.println(packetSize);
#endif            
            break;
          }
          
          packetType=(byte)LoRa.read();
          if(packetType==CMND_WINDOW_OPEN){
            break;
          }  
          else{
#ifdef DISPLAY_DEBUG            
            Serial.println("Unexpected packet waiting for command window - " + String(packetType));  
#endif            
          }
     }
  }         
  
  if(millis()-lastTime>=ACK_TIMEOUT){
#ifdef DISPLAY_DEBUG    
    Serial.println("ERROR - command window open not received from node");
#endif    
    // Cannot send commands so wait next data        
  }
  else{
    // Send LED command if has been updated
    if(ledCmnd!=lastledCmnd){
      LoRa.beginPacket();
      LoRa.write((byte)LED_CMND);
      LoRa.write((byte)ledCmnd);
      LoRa.endPacket();     
      lastledCmnd=ledCmnd;
      
      // Wait for ACK
      lastTime=millis();
      while(millis()-lastTime<ACK_TIMEOUT){
        packetSize = LoRa.parsePacket();
        if(packetSize){
          packetType=(byte)LoRa.read();
          if(packetType!=ACK){
#ifdef DISPLAY_DEBUG            
            Serial.print("ERROR - unexpected message waiting for ACK to LED_CMND: ");
            Serial.println(packetType);
#endif            
          } 
          break;      
        }
     }
     if(millis()-lastTime>ACK_TIMEOUT){
#ifdef DISPLAY_DEBUG      
        Serial.print("ERROR - timeout waiting for ACK to LED_CMND");       
#endif
     }
    }//if            
   
    // Send register command if has been updated
    if(requestedRegAddr != 0xff){
      LoRa.beginPacket();
      LoRa.write((byte)REG_CMND);
      LoRa.write((byte)requestedRegAddr);
      LoRa.endPacket();     

      // Wait for REG_VALUE
      lastTime=millis();
      while(millis()-lastTime<ACK_TIMEOUT){
        packetSize = LoRa.parsePacket();
        if(packetSize){
          packetType=(byte)LoRa.read();
          if(packetType==REG_VALUE){
            regValue = (byte)LoRa.read();
            readRegAddr = (byte)LoRa.read();
          }
          else{
#ifdef DISPLAY_DEBUG            
            Serial.print("ERROR unexpected message waiting for REG_VALUE: ");
            Serial.println(packetType);
#endif            
          } 
          break;      
        }
      }// while
      if(millis()-lastTime>ACK_TIMEOUT){
#ifdef DISPLAY_DEBUG        
        Serial.print("ERROR - timeout waiting for ACK to REG_CMND");       
#endif           
      }
    }// if
  }// else  
}// loop

// Extract the register address as an integer from the HTTP message string
byte getregaddr(String responseBody)
{
    // This procedure is required because 'String' is a class in C++ not a char array
    // and we need to be able to access the chars which are hidden within
    char addr[20], command[20]; 
    char *spntr, *dpntr;
    
    spntr = (char*)responseBody.c_str();
    dpntr = command;         
    while(*spntr != '='){
      *dpntr=*spntr;
      dpntr++;
      spntr++;
     }
    *dpntr='\0';
    
    spntr++; // skip '='
    dpntr = addr;
    while(*spntr != '\0'){
      *dpntr=*spntr;
      dpntr++;
      spntr++;
    }       
    *dpntr='\0';   
    // Similarly 'toInt()' only works on String class, not an actual string
    return byte(((String)addr).toInt());
}  
