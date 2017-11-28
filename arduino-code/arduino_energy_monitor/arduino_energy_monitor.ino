//#include <FreqPeriodCounter.h>
// measure battery voltage and current to actuators
// battery voltage is returned in Volt
// current is returned in Milliampere
// most currents are measured in the line from the battery voltage to the actuator driver
// this battery voltage is measured in this node as well
// exception to this are the control surfaces, which are supplied with 12 V
// they are measured in the supply line to the tail section arduino and all servos

// define name of ports to connect with terminal on maxon boards
#define pinVolBattery 14
#define pinAmpTH_0 15
#define pinAmpTH_1 16
#define pinAmpTH_2 17
#define pinAmpTH_3 18
#define pinAmpFins 19 // supply with 12vDC
#define pinAmpProp 20 // supply with 24vDC

// variables used for the serial communication
unsigned long timeStart; //for finding a header
unsigned long timeElapse; //for finding a header
int timeOut = 100; //milisecond , for finding a header
int flagHeader;
int flagDataID = 0; // use when identifing a data type
int flagCS; // use to check if the data was read completely

// variables used for the voltage measurement and current measurement
int sensorValue;
float sensorValueVol;
float voltage;
float mAmp;
int mVperAmp = 100; // use 185 for 5A Module, 100 for 20A Module, and 66 for 30A Module
int ACSoffset = 2500;  // 0 A gives 2.5 V measurement for this sensor configuration

int i = 0;
int buffLength = 60;
int buffIndex;
char inData[60]; // Allocate some space for the string
char inChar; // Where to store the character read
char index = 0; // Index into array; where to store the character
String strOut;

// variables used for the output
float vol; // store the voltage measurement
char charBuf[6]; // store the temporary value of voltage and current before send back to mother board

void setup()
{
  // setup pinMode to be utilized in a loop
  pinMode(pinVolBattery, INPUT);
  pinMode(pinAmpProp, INPUT);
  pinMode(pinAmpFins, INPUT);
  pinMode(pinAmpTH_0, INPUT);
  pinMode(pinAmpTH_1, INPUT);
  pinMode(pinAmpTH_2, INPUT);
  pinMode(pinAmpTH_3, INPUT);

  // open serial port
  Serial.begin(9600);

}

void loop()
{
  // search for a header 'fa' from the incoming string.
  
  flagDataID = 0;
  if(Serial.available()){
    flagHeader = 0;
    inChar = Serial.read();
    if(inChar == 'f'){
      flagHeader = 1;
      timeStart = millis();
      timeElapse = millis()-timeStart;
      while(flagHeader<2 && timeElapse<timeOut){

        if(Serial.available()>0){ // if there are at least one charactor to be read
          inChar = Serial.read();
          if(flagHeader == 0){
            if(inChar == 'f'){
              flagHeader = 1; // found first character of the header
            }
          }
          else if (flagHeader == 1){
            if(inChar == 'a'){
              flagHeader = 2; // found second character of the header
              break; // break while loop as found the header
            }
            else if (inChar == 'f'){
              flagHeader = 1;
            }
            else{
              flagHeader = 0;
            }
          }
        }
        timeElapse = millis()-timeStart;
      }
      if(flagHeader<2 && timeElapse<=timeOut){
        //        Serial.println("time out: no header found");
        Serial.flush();
      }       
    }
  }

  if(flagHeader == 2){ //if two charactors of the header were found, continue getting data from the coming packet.

    // empty the temporary string
    for (i=0;i<buffLength;i++) {
      inData[i]=0;
    }
    index=0;

    flagCS = 0; // flag for the check-sum
    timeStart = millis();
    timeElapse = millis()-timeStart;
    while(flagCS<2 && timeElapse<timeOut){
      if(Serial.available()>1){ // wait until there are atleast two chars in the serial port
        inChar = Serial.read();
        if(inChar == 'c'){ // if the first footer is found
          flagCS=1;
          inChar = Serial.read();
          if(inChar == 's'){ // if the second footer is found
            flagCS=2; // succesfully got the check-sum

            //identify a data type
            if(inData[0]=='V'){
              flagDataID = 1; // getVoltage
              break;
            }
            else if(inData[0] == 'I'){
              flagDataID = 2; // getCurr
              break;
            }
            else{
              flagDataID = -1; // unknown data type
              break;
            }

            break; // break while loop as succeeded getting the data
          }    
        }
        else if(index<buffLength){
          inData[index] = inChar; // Store it
          index++; // update indicator of where to write next
        }
      }

      timeElapse = millis()-timeStart;
    }
    if(flagCS<2 && timeElapse>=timeOut){
      //      Serial.println("time out: cannot find check-sum stop waitting for data");
      Serial.flush();
    }
    flagHeader = 0; // reset flagHeader

  }

  switch (flagDataID) {

    case 1: // measure the voltage of battery and write it to serial port
      // this can also be used to check if the arduino is able to send and receive the data
      // redeclare to empty the variable
      charBuf[0] = (char)0; // empty the charBuf
  
      sensorValue = analogRead(pinVolBattery);
      // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
      vol = sensorValue * (80 / 1023.0); // read measurement from Voltage Sensor Module
  
      dtostrf(vol, 1, 2, charBuf);
      strOut = "";
      strOut +="#";
      strOut +=charBuf;
      strOut +="V";
      Serial.println(strOut);
      break;
  
    case 2:
      // measure the current through the 6 sensors
      // A1/pin 15: Th0/Vertical front
      // A2/pin 16: Th1/Vertical aft
      // A3/pin 17: Th2/Horizontal front
      // A4/pin 18: Th3/Horizontal aft
      // A5/pin 19: Arduino and control surface servos
      // A6/pin 20: Propeller motor
      strOut = "";
      strOut +="#";
      for(i=0;i<6;i++){
        sensorValue = analogRead(i+15);
        sensorValueVol = (sensorValue / 1023.0) * 5000; // Convert analog read to mV: 10 bit==1024 values between 0V and 5V
        mAmp = 1000*((sensorValueVol - ACSoffset) / mVperAmp);

        charBuf[0] = (char)0; // empty the charBuf
        dtostrf(mAmp, 1, 2, charBuf); // convert ith rpm from float to char and store it in charBuf
        strOut +=charBuf;
        strOut +="@";
      }
      
      strOut +="A";
      Serial.println(strOut);
      break;
 
    case -1:
      //      Serial.println("wrong data format, check a message type identifier");
      break;
  }

}
