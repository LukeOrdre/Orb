int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

int encoderPosition = 0; 

unsigned long aPinLastChangeTime;
unsigned long aLastClickTime;
unsigned long bLastClickTime;
unsigned long lastFourClicks[4] = {0,0,0,0};
unsigned long avgSpeed;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

unsigned long bPinLastChangeTime;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

unsigned long deBounceTime = 3;

 void setup() { 
   pinMode (encoderPinA,INPUT);
   pinMode (encoderPinB,INPUT);

   aPinReading = digitalRead(encoderPinA);   
   aPinShort = aPinReading;
   aPinLong = aPinReading;

   bPinReading = digitalRead(encoderPinB);
   bPinShort = bPinReading;
   bPinLong = bPinReading;
   
   Serial.begin (9600);
 } 

 void loop() {
   
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
        encoderPosition++;
        //Serial.println(encoderPosition);
        //Serial.println(millis() - aLastClickTime);

        lastFourClicks[3] = lastFourClicks[2];
        lastFourClicks[2] = lastFourClicks[1];
        lastFourClicks[1] = lastFourClicks[0];
        lastFourClicks[0] = millis() - aLastClickTime;

        avgSpeed = (lastFourClicks[0] + lastFourClicks[1] + lastFourClicks[2] + lastFourClicks[3]) / 4;
        //Serial.println(avgSpeed);
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
        encoderPosition++;

        lastFourClicks[3] = lastFourClicks[2];
        lastFourClicks[2] = lastFourClicks[1];
        lastFourClicks[1] = lastFourClicks[0];
        lastFourClicks[0] = millis() - bLastClickTime;

        //Serial.println((lastFourClicks[0] + lastFourClicks[1] + lastFourClicks[2] + lastFourClicks[3] / 4));
        
        bLastClickTime = millis();
        bPinLong = bPinShort;
      }
   }
 }   
   

