bool debugMode = true;
int MegaMotoPWMpin = 11;
int powerMin = 28; // dependent on voltage. 
int power = powerMin; //current power

char incomingCharacter;

unsigned long targetClickTime = 100; // number of ms between encoder clicks, so higher = slower
unsigned long targetBuffer = 5;
unsigned long speedChangeBuffer = 20; //ms tolerance on speed target
unsigned long prevPowerChangeTime;
unsigned long lastFourClicks[5] = {0,0,0,0,0};
unsigned long avgClickTime = 0;

int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

int encoderPosition = 0; 

unsigned long aPinLastChangeTime;
unsigned long bPinLastChangeTime;

unsigned long aLastClickTime;
unsigned long bLastClickTime;


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


void loop() {

  if(encoderPosition == 0)
  {
    
    //incomingCharacter = Serial.read();
    incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':
         encoderPosition = 80;
         power = powerMin;
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();
         aLastClickTime = millis();
         bLastClickTime = millis();
         lastFourClicks[0] = millis();
         lastFourClicks[1] = millis();
         lastFourClicks[2] = millis();
         lastFourClicks[3] = millis();
         lastFourClicks[4] = millis();
         analogWrite(MegaMotoPWMpin, power);
        break;
       case '2':
         encoderPosition = 80;
        break;
       case '3':
         encoderPosition = 120;
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
        encoderPosition--;
        updateTiming();
        updatePower();
        aLastClickTime = millis();
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
        encoderPosition--;
        updateTiming();
        updatePower();
        bLastClickTime = millis();
        //debugPrint("b pin enc--");
        bPinLong = bPinShort;
      }
   }

  if(encoderPosition > 0)
  {
    if((millis() - lastFourClicks[0]) > (targetClickTime + targetBuffer))
    {
      if((millis() - prevPowerChangeTime) > speedChangeBuffer) // prevent power surging, change it slowly
      {
          power +=1;
          prevPowerChangeTime = millis();
          if(power < powerMin) power = powerMin;
    if(power > 254) power = 254;
          analogWrite(MegaMotoPWMpin, power);
      }
    }
    
  }
 
  
  if(encoderPosition <= 0) 
  {
    encoderPosition = 0;
    //power = powerDefault;
    //digitalWrite(MegaMotoPWMpin, LOW);
  }

  //if((avgClickTime < targetClickTime - targetBuffer) || (avgClickTime > targetClickTime + targetBuffer))
  if(millis() % 50 == 0)
  {
            debugPrint("");
  }
 }   

void updateTiming()
{
        //lastFourClicks[4] = lastFourClicks[3];
        lastFourClicks[3] = lastFourClicks[2];
        lastFourClicks[2] = lastFourClicks[1];
        lastFourClicks[1] = lastFourClicks[0];
        lastFourClicks[0] = millis();

        avgClickTime = (millis() - lastFourClicks[3]) / 4;
}

void updatePower()
{
      //if(millis() - prevPowerChangeTime > speedChangeBuffer) // prevent power surging, change it slowly
      //{
          if(avgClickTime < targetClickTime + targetBuffer)
          {
            power -= 1;
            if(power < powerMin) power = powerMin;
            if(power > 254) power = 254;
            analogWrite(MegaMotoPWMpin, power);
            prevPowerChangeTime = millis();
          }
          if(avgClickTime > targetClickTime - targetBuffer)
          {
            power += 2;
            if(power < powerMin) power = powerMin;
            if(power > 254) power = 254;
            analogWrite(MegaMotoPWMpin, power);
            prevPowerChangeTime = millis();
          }
      //}
      //debugPrint("");
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
//          Serial.print(lastFourClicks[0]);
//          Serial.print("     ");
//          Serial.print(lastFourClicks[1]);
//          Serial.print("     ");
//          Serial.print(lastFourClicks[2]);
//          Serial.print("     ");
//          Serial.print(lastFourClicks[3]);
//          Serial.print("     ");
          Serial.print(avgClickTime);
          Serial.print("     ");
          Serial.print((power - 20) * 10);
          Serial.print("     ");
          Serial.print(encoderPosition);
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

