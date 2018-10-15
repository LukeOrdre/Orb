bool debugMode = true;
int MegaMotoPWMpin = 11;
float powerMin = 26; // dependent on voltage. 28 gives steady rotation, less stutters
float power = powerMin; //current power

char incomingCharacter;

unsigned long targetClickTime = 100; // number of ms between encoder clicks, so higher = slower
unsigned long targetBuffer = 20;
unsigned long startTime = 0;
unsigned long lastPowerChange;

//unsigned long clickTimes[100] = {0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    
//                                 0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    
//                                 0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,
//                                 0,0,0,0,0,0,0,0,0,0};

int clickCount = 0;
int clickTarget = 0;

unsigned long avgClickTime = 0;

int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long aPinLastChangeTime;
unsigned long bPinLastChangeTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

unsigned long deBounceTime = 3;

 void setup() { 
   pinMode (encoderPinA,INPUT);
   pinMode (encoderPinB,INPUT);
   pinMode(MegaMotoPWMpin, OUTPUT);
   setPwmFrequency(MegaMotoPWMpin, 1);  // change Timer2 divisor to 8 gives 3.9kHz PWM freq
   
   aPinReading = digitalRead(encoderPinA);   
   aPinShort = aPinReading;
   aPinLong = aPinReading;

   bPinReading = digitalRead(encoderPinB);
   bPinShort = bPinReading;
   bPinLong = bPinReading;
   
   Serial.begin (9600);
 } 


void loop()
{
  if(clickTarget == 0)
  {
    //incomingCharacter = Serial.read();
    incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':
         //clickTimes[0] = millis();
         startTime = millis();
         clickCount = 1;
         wipeClicks();
         clickTarget = 80;
         power = powerMin;
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();
         analogWrite(MegaMotoPWMpin, (int)power);
         debugPrint("");
        break;
       case '2':
         clickTarget = 80;
        break;
       case '3':
         clickTarget = 120;
        break;
      }
  }
   
  aPinReading = digitalRead(encoderPinA);   

  if (aPinReading == 0 && aPinLong == 1){
     aPinLastChangeTime = millis();
     aPinShort = 0;
  }

  if (aPinReading == 1 && aPinLong == 0)
  {
     aPinLastChangeTime = millis();
     aPinShort = 1;
  }

  if((millis() - aPinLastChangeTime) > deBounceTime)
  {
     if(aPinLong != aPinShort)
     {           
       recordClick();
       aPinLong = aPinShort;
     }
  }

  bPinReading = digitalRead(encoderPinB);   

  if (bPinReading == 0 && bPinLong == 1){
      bPinLastChangeTime = millis();
      bPinShort = 0;
   }

   if (bPinReading == 1 && bPinLong == 0)
   {
      bPinLastChangeTime = millis();
      bPinShort = 1;
   }

   if((millis() - bPinLastChangeTime) > deBounceTime)
   {
      if(bPinLong != bPinShort)
      {           
        recordClick();
        bPinLong = bPinShort;
      }
   }

  if(clickTarget != 0 && (millis() % 20 == 0))
  {
    updateTiming();
    updatePower();
    debugPrint("");
  }
  
  if(clickCount >= clickTarget) 
  {
    clickCount = 0;
    clickTarget = 0;
    //power = powerDefault;
    //digitalWrite(MegaMotoPWMpin, LOW);
  }
}   

void wipeClicks()
{
//  for(int i=0; i< sizeof (clickTimes); i++)
//  {
//    clickTimes[i]=0;
//  }
}

void recordClick()
{
  //clickTimes[clickCount] = millis();
  clickCount++;
  updateTiming();
}

void updateTiming()
{
  //avgClickTime = (millis() - clickTimes[0]) / clickCount;
  avgClickTime = (millis() - startTime) / clickCount;
}

void updatePower()
{
    if(millis() - lastPowerChange > 3*targetClickTime)
    {
      if(avgClickTime < targetClickTime + targetBuffer)
      {
          //lastPowerChange = millis();
          //power -= 1;
          //if(power < powerMin) power = powerMin;
          //if(power > 254) power = 254;
          //analogWrite(MegaMotoPWMpin, (int)power);
      }
      if(avgClickTime > targetClickTime)
      {
        lastPowerChange = millis();
        power += 1;
        if(power < powerMin) power = powerMin;
        if(power > 254) power = 254;
        analogWrite(MegaMotoPWMpin, (int)power);
      }
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

void debugPrint(char originMessage[])
{
  if(debugMode)
  {
//          Serial.print(millis());
//          Serial.print("     ");
//          Serial.print(clickTimes[0]);
//          Serial.print("     ");
//          Serial.print(clickTimes[1]);
//          Serial.print("     ");
//          Serial.print(clickTimes[2]);
//          Serial.print("     ");
//          Serial.print(clickTimes[3]);
//          Serial.print("     ");
          Serial.print(power);
          Serial.print("     ");
          Serial.print(avgClickTime);
          Serial.print("     ");
          Serial.print((power - 20) * 10);
          Serial.print("     ");
          Serial.print(clickCount);
          Serial.print("     ");
          Serial.print(targetClickTime + targetBuffer);
          Serial.print("     ");
          Serial.print(targetClickTime - targetBuffer);
          Serial.print("     ");
          Serial.print(targetClickTime);
          Serial.print("     ");
          //Serial.println(originMessage);
          Serial.println();
  }          
}

