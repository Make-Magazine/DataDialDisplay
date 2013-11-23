#include <Servo.h>

Servo sL;
Servo sC;
Servo sR;

//Adjust these values to ensure your dials are on zero.
int zeroL=168;
int zeroC=180;
int zeroR=164;

void setup() {
  //Attaches to all servos and then moves them to each end of the dial.
  //This will help us position the hands correctly on the servo to match the gauge.
  
  sL.attach(5);
  sC.attach(6);
  sR.attach(7);
  
  sL.write(80);
  delay(1000);
  sC.write(80);
  delay(1000);
  sR.write(80);
  delay(1000);
  sL.write(zeroL);
  delay(1000);
  sC.write(zeroC);
  delay(1000);
  sR.write(zeroR);
  
}

void loop() {
  // put your main code here, to run repeatedly: 
  
}
