int MegaMotoPWMpin = 11;
int powerMin = 28; // dependent on voltage. 
int power = powerMin; //current power
int encoderpinA = 12;  // Connected to CLK on KY-040
int encoderpinB = 13;  // Connected to DT on KY-040 // not needed
int encoderPosition = 0; // current calculated position
int encoderpinAPrev;  // holds prev in loop
int encoderpinACurrent; // holds current pos in loop
int encoderpinBPrev;  // holds prev in loop
int encoderpinBCurrent; // holds current pos in loop
char incomingCharacter;
unsigned long prevClickTime;
unsigned long newClickTime;
unsigned long prevPowerChangeTime;
unsigned long speedtarget = 100; // number of ms between encoder clicks, so higher = slower
unsigned long speedchangebuffer = 20; //ms tolerance on speed target

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  
  pinMode(MegaMotoPWMpin, OUTPUT);
  setPwmFrequency(MegaMotoPWMpin, 8);  // change Timer2 divisor to 8 gives 3.9kHz PWM freq

  encoderpinAPrev = digitalRead(encoderpinA);
}

void loop() {
  
  if(encoderPosition == 0)
  {
    
    incomingCharacter = Serial.read();
  
    switch (incomingCharacter) {
       case '1':
         encoderPosition = 1020;
         //power = powerDefault;
         prevClickTime = millis();
        break;
       case '2':
         encoderPosition = 80;
        break;
       case '3':
         encoderPosition = 120;
        break;
  
      }
  }
  encoderpinACurrent = digitalRead(encoderpinA);
  encoderpinBCurrent = digitalRead(encoderpinB);
  if (encoderpinACurrent != encoderpinAPrev || encoderpinBCurrent != encoderpinBPrev)
  {
     encoderPosition--;
     //Serial.println(encoderPosition);
     newClickTime = millis() - prevClickTime;
     if(newClickTime + speedchangebuffer < speedtarget)
     {
        //if(newClickTime > 3)
        //{
          power -= 1;
          digitalWrite(MegaMotoPWMpin, LOW);
          //delayMicroseconds(10000);
          delay(1500);
          analogWrite(MegaMotoPWMpin, power);
          //Serial.println(power);
        //}
        Serial.println(newClickTime);
     }
     prevClickTime = millis();
  }

  encoderpinAPrev = encoderpinACurrent;
  encoderpinBPrev = encoderpinBCurrent;

  if(encoderPosition > 0)
  {
    if(millis() - prevClickTime > speedtarget)
    {
      if(millis() - prevPowerChangeTime > 5) // prevent power surging, change it slowly
      {
          power +=1;
          prevPowerChangeTime = millis();
          //Serial.println(power);
          analogWrite(MegaMotoPWMpin, power);
      }
      //Serial.println(millis() - prevClickTime);
    }
    if(power < powerMin) power = powerMin;
    if(power > 254) power = 254;
  }
  
  if(encoderPosition <= 0) 
  {
    encoderPosition = 0;
    //power = powerDefault;
    digitalWrite(MegaMotoPWMpin, LOW);
  }
}
 
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) { // Timer0 or Timer1
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) { 
      TCCR0B = TCCR0B & 0b11111000 | mode; // Timer0
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode; // Timer1
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode; // Timer2
  }
}
