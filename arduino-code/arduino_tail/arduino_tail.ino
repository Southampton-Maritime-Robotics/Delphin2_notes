#include <FreqPeriodCounter.h>
#include <Servo.h> 

// define name of ports to connect with terminal on maxon boards
#define set_fin_top 3
#define set_fin_stb 5
#define set_fin_bot 6
#define set_fin_prt 11
#define set_prop 9
Servo myProp;

const byte counterPin = 2;
const byte counterInterrupt = 0; // = 0:pin D2, 1:pin D3

#define posn_fin_top A0
#define posn_fin_stb A1
#define posn_fin_bot A2
#define posn_fin_prt A3

// variables used for the serial communication
unsigned long timeStart; //for finding a header
unsigned long timeElapse; //for finding a header
int timeOut = 50; //milisecond , for finding a header
int flagHeader;
int flagDataID = 0; // use when identifing a data type
int flagCS; // use to check if the data was read completely

// variables used for getting fin positions and motor speed and constructing the message to reply the master
FreqPeriodCounter counter(counterPin, micros, 0);
float sensorValue;
int servoPosition[4] = {-1,-1,-1,-1};
int frq = -1;
char charBuf[60]; // used for constructing the message to be sent to the master
int buffLength = 60;
int buffIndex;
char inData[60]; // Allocate some space for the string
char inChar; // Where to store the character read
char index = 0; // Index into array; where to store the character
String strOut;


// actuator parameters
int fin_neutral[4] = {95,75,75,75};
int prop_neutral = 94;
int maxlimit_fin = 170;
int minlimit_fin = 5;
int maxlimit_prop = 150;
int minlimit_prop = 40;

//// variable used for extracting the demand from the master
int setPoint[5] = {fin_neutral[0],fin_neutral[1],fin_neutral[2],fin_neutral[3],prop_neutral}; // b,c,d,e,prop
int i = 0; // use when parsing a setPoint
int j = 0; // use when parsing a setPoint
int k = 0; // use when parsing a setPoint
String tempData; // use when parsing a setPoint

// loop timing control
unsigned long timeLastCommunication;
int timeLastCommunicationMax = 2000; // millisecond

void setup()
{
  // setup pinMode to utilize a motor control loop
  pinMode(set_fin_top, OUTPUT); // to set a position of top fin
  pinMode(set_fin_stb, OUTPUT); // to set a position of starboard fin
  pinMode(set_fin_bot, OUTPUT); // to set a position of bottom fin
  pinMode(set_fin_prt, OUTPUT); // to set a position of port fin
  myProp.attach(set_prop); // to set propeller speed

  // to set actuators to their neutral position
  analogWrite(set_fin_top,fin_neutral[0]);
  analogWrite(set_fin_stb,fin_neutral[1]);
  analogWrite(set_fin_bot,fin_neutral[2]);
  analogWrite(set_fin_prt,fin_neutral[3]);
  myProp.write(prop_neutral);
  
  pinMode(posn_fin_top, INPUT); // to get a position of top fin
  pinMode(posn_fin_stb, INPUT); // to get a position of starboard fin
  pinMode(posn_fin_bot, INPUT); // to get a position of bottom fin
  pinMode(posn_fin_prt, INPUT); // to get a position of port fin
  
  attachInterrupt(counterInterrupt, counterISR, CHANGE);
  
  // set actuators to neutral/off position by default
  setPoint[0] = fin_neutral[0];
  setPoint[1] = fin_neutral[1];
  setPoint[2] = fin_neutral[2];
  setPoint[3] = fin_neutral[3];
  setPoint[4] = prop_neutral; 
  
  // open serial port
  Serial.begin(9600);
  
}

void loop()
{
  // search for a header '$' from the incoming string.
  flagDataID = 0;
  flagHeader = 0;
  timeStart = millis();
  timeElapse = millis()-timeStart;
  while(flagHeader==0 && timeElapse<timeOut){
    if(Serial.available()>0){
      inChar = Serial.read();
      if(inChar == '$'){
        flagHeader = 1; // the header is found
      }
    }
    timeElapse = millis()-timeStart;
  }
  
  if(flagHeader==0 && timeElapse>=timeOut){
//    Serial.println("time out: no header found");
    Serial.flush();
  }
  
  if(flagHeader == 1){ //if two charactors of the header were found, continue getting data from the coming packet.

    // empty the temporary data string
    inData[0] = (char)0;
    index=0;
  
    flagCS = 0; // flag for the check-sum
    timeStart = millis();
    timeElapse = millis()-timeStart;
    while(flagCS==0 && timeElapse<timeOut){
      if(Serial.available()>0){ // wait until there are atleast two chars in the serial port
        inChar = Serial.read();
        if(inChar == '#'){ // if the check-sum is found
          flagCS=1;
          //identify a packet type
          if(inData[0]=='S'){
            flagDataID = 1;  // setActuatorSetpoint
//            Serial.println("ID S is found");
          }else if(inData[0]=='R'){
            flagDataID = 2; // getActuatorFeedback
//            Serial.println("ID R is found");
          }else{
            flagDataID = -1; // unknown data type
//            Serial.println("wrong dataID");
          }
        }else if(index<buffLength){
          inData[index] = inChar; // Store it
          index++; // update indicator of where to write next
        }
      }
      timeElapse = millis()-timeStart;
    }
    if(flagCS==0 && timeElapse>=timeOut){
//      Serial.println("time out: no check-sum found");
      Serial.flush();
    } 
  }
  
  // watchdog for communication lost
  if (flagDataID!=0){
    timeLastCommunication = millis();
  }
  // if communication lost for longer than a specified limit, set fins to neutral position and turn off prop
  if (millis()-timeLastCommunication>timeLastCommunicationMax){
    // reset actuator setpoint
    setPoint[0] = fin_neutral[0];
    setPoint[1] = fin_neutral[1];
    setPoint[2] = fin_neutral[2];
    setPoint[3] = fin_neutral[3];
    setPoint[4] = prop_neutral; 
    Serial.println("communication lost for over than 2 secs");
  }
  
  switch (flagDataID) {
    
    case 1: // set actuator setpoint
//      Serial.println("received setPoints");
      buffIndex = 1; // indicator for data packet
      tempData = "";
      i = 0; // indicator for actuator index
      k = 0; // indicator for tempData to keep a section of data packet before converting to an integer of setpoint
      while(i<5){ // varying with a number of actuators
        if(inData[buffIndex]!='@'){ //if this char is not a delimiter
          if(k<buffLength){ // if it is not exceed the limit of buffLength
            if(isDigit(inData[buffIndex])){ 
              tempData += inData[buffIndex]; // keep this char into a tempData
              buffIndex++;
              k++;
            }else{ // the input is not a number
//              Serial.println("wrong data format, please check the input");
              break;
            }          
          }
          else{ // if the number of setpoints does not agree with a number of actuators
//            Serial.println("wrong data format, number of setpoints does not agree with a number of actuators");
            break; //break while loop
          }
        }else{ //once the demimiter found, convert an array of char into int
        
          setPoint[i] = tempData.toInt();
          if(i<4){ // apply limit to fin setpoint
            if(setPoint[i]>maxlimit_fin){
              setPoint[i] = maxlimit_fin;
            }else if(setPoint[i]<minlimit_fin){
              setPoint[i] = minlimit_fin;
            }
          }else{ // apply limit to prop setpoint
            if(setPoint[i]>maxlimit_prop){
              setPoint[i] = maxlimit_prop;
            }else if(setPoint[i]<minlimit_prop){
              setPoint[i] = minlimit_prop;
            }
          }
    
          buffIndex++;
          i++;
          tempData = "";
        }
      }

      break; // break case 1
      
    case 2: // measure motor rps and fin angles then send back to the master
      strOut = "";
      strOut +="$";
      for (i=0; i<4; i++){
        sensorValue = analogRead(14+i);
        servoPosition[i] = (int)(sensorValue*-0.3793103448 + 256.2068965517);
        
        charBuf[0] = (char)0; // empty the charBuf
        dtostrf(servoPosition[i], 1, 2, charBuf); // convert the position i_th servo to string
        strOut +=charBuf;
        strOut +="@"; // delimiter
        
        charBuf[0] = (char)0; // empty the charBuf   
        dtostrf(setPoint[i], 1, 2, charBuf); // convert the setpoint of the i_th servo to string
        strOut +=charBuf;
        strOut +="@"; // delimiter
        
      }
      
      charBuf[0] = (char)0; // empty the charBuf
      dtostrf(frq, 1, 2, charBuf); // convert prop speed to string
      strOut +=charBuf;
      strOut +="@"; // delimiter

      charBuf[0] = (char)0; // empty the charBuf
      dtostrf(setPoint[4], 1, 2, charBuf); // convert prop speed to string
      strOut +=charBuf;
      strOut +="@"; // delimiter

      strOut +="#"; // checksum
      
      // reply to the master
      Serial.println(strOut);
      
      break;
      
    case -1:
//      Serial.println("wrong data format, check a message type identifier");
      break;   
  }
  
  // apply setpoints to the servos and motor
  analogWrite(set_fin_top,setPoint[0]);
  analogWrite(set_fin_stb,setPoint[1]);
  analogWrite(set_fin_bot,setPoint[2]);
  analogWrite(set_fin_prt,setPoint[3]);
  myProp.write(setPoint[4]);

  // update frequency measurement
  if(counter.ready()){
    frq = counter.hertz();
  }else{
    frq = 0;
  }
  
}

void counterISR(){ 
  counter.poll();
}
