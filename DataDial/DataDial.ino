/*
  DataDial was created by Matt Stultz for Make Magazine in November 2013.
  stultzm@gmail.com twitter @MattStultz
  
  Thanks to Kelly Egan @kellyegan and Jonathan Speicher @jon_speicher for their help with the project.

 The web code was based on the sketch:
  Web client
 
 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe, based on work by Adrian McEwen
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "earthquake.usgs.gov";    // name address for USGS Earthquake data (using DNS)

// Set the static IP address to use if the DHCP fails to assign an IP to your arduino.
IPAddress ip(192,168,0,177);

// Initialize the Ethernet client library.
EthernetClient client;

//Memory buffer
#define BUFFER_SIZE 500
char buffer[BUFFER_SIZE];

//Global variables
int dayCount = 0;
int weekCount = 0;
float weekMag = 0.0;
boolean bDay = true;
float maxMag = 0.0;
unsigned long targetTime;

//Adjust these values to ensure each dial is zeroed
//Use the values found in the ServoCalibration sketch.
//Example:
int zeroL=168;
int zeroC=180;
int zeroR=164;
//int zeroL=180;
//int zeroC=180;
//int zeroR=180;

//Servo pins Left, Center, Right
int L = 5;
int C = 6;
int R = 7;

//Servos Left, Center, Right
Servo servoL;
Servo servoC;
Servo servoR;

//Defines the scale for each reading
int dayMult = 3;
float magMult = .1;
int weekMult = 25;

 

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(115200);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  //Connect to each servo.
  servoL.attach(L);
  servoC.attach(C);
  servoR.attach(R);
  
  //Set each servo to the far left position. Our servos are mounted upside down so we will start at 180.
  servoL.write(zeroL);
  servoC.write(zeroC); 
  servoR.write(zeroR);
 
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");

  //Set startup target time that is 24 hours later to reset Weekly magnitude counter.
  //24 hours * 60 minutes * 60 seconds * 1000 miliseconds = 86400000
  targetTime = millis() + 86400000;

}

void loop()
{
    //get Day data
    bDay = true;
    getData();
    //Get Week data
    bDay= false;
    getData();
    
    //check to see if 24 hours has passed since last update and reset the max magnitue.
    
    if(millis() >= targetTime)
    {
      maxMag=0.0;
      targetTime = millis() + 86400000;
    }
    
    //Wait 30 seconds and start looking again.
    delay(30000);
}

void getData(){
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    if(bDay){
      client.println("GET http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_day.geojson HTTP/1.0");
    }else{
      client.println("GET http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_week.geojson HTTP/1.0");
    }
    client.println();    
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  
  //Reset variables.
  int bufIndex = 0;
  boolean bJson = false;
  boolean haveCount = false;
  int outC = 0;
  
  //Loop while you still have a connection
  while(client.connected()){
    //Ensure data has been returned
    if(client.available()) {
        //Read in a character
        char c = client.read();
        //Look for the beginning of Json data.
        if(!bJson){
          if(c=='{'){
            bJson = true;
          }
        }
        //If Json data has been found begin handling it.
        if(bJson){
          //Check to make sure you haven't over filled the buffer.
          if(bufIndex<BUFFER_SIZE){
            //Write the recieved character to the memory buffer.
            buffer[bufIndex] = c;
            bufIndex++;
            //Look for the end of a data group.
            if(c=='}'){
              if(!haveCount){
                if(bDay){
                  //Parse the day count data.
                  dayCount = getCount();
                  Serial.println(dayCount);
                  //Remember you have the day data so you don't look for it again.
                  haveCount = true;
                  //Set the left servo into position based on the count found.
                  int outL = zeroL-(dayCount/dayMult);
                  servoL.write(outL);
                  //We have the data so close the connection.
                  client.stop();
                }else{
                  //Now that we have the day data get the week data.
                  weekCount = getCount();
                  Serial.println(weekCount);
                  haveCount = true;
                  int outR = zeroR-(weekCount/weekMult);
                  servoR.write(outR);
                }
                
                //reset the memory buffer to clean out the data.
                memset(buffer,0,BUFFER_SIZE);
                bufIndex=0;
               }
             }else{
               //We don't need to look for magnitude data for the day so make sure we are in week mode.
               if(!bDay){
                 //Look for the beginning of the data.
                 if(buffer[0]=='{'){
                   //Find the first comma as a stop character.
                   if(c==','){
                     //Parse the magnitude data.
                      weekMag = getMag();
                      //Only update the servo if the magnitude is higher than any of the previous found.
                      if(weekMag>maxMag){
                        maxMag = weekMag;
                        Serial.println(maxMag);
                        outC = zeroC-(maxMag/magMult);
                        servoC.write(outC);
                      }
                   }
                 }else{
                   //Reset the memory buffer.
                   memset(buffer,0,BUFFER_SIZE);
                   bufIndex=0;
                 }
                 
               }
             }
        }else{
            //Reset the memory buffer.
            memset(buffer,0,BUFFER_SIZE);
            bufIndex=0;
        }  
      }    
    }
    
  }
  client.stop();
}


int getCount(){
  //Find the position of the word "count" in the memory buffer.
  char *Start = strstr(buffer, "count");
  //Move past the word count in the buffer
  Start += 7;
  
  //get the characters between the start position and the bracket in the memory buffer.
  char *Data = strtok(Start,"}");
  //Serial.println(Data);
  //Convert the characters to integers and return it.
  int Out = atoi(Data);
  return Out;
  
}

float getMag(){
  //Serial.println(buffer);
  //Find the postion of the word "properties" in the memory buffer.
  char *Start = strstr(buffer, "properties");
  //Move past properties.
  Start += 19;
  
  //Find the trailing comma.
  char *Data = strtok(Start,",");
  
  //Convert the characters to floats and return them.
  //Serial.println(Data);
  float Out = atof(Data);
  return Out; 
}

