#include <TimerOne.h>
int MegaMotoPWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

void setup() {
  Timer1.initialize(100);
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (MegaMotoPWMpin, OUTPUT);
  Timer1.pwm(MegaMotoPWMpin, 95);
}

void loop() {
  
  
  // put your main code here, to run repeatedly:
  
}
