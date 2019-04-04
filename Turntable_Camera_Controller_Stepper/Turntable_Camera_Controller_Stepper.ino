//#include <PID_v1.h>
#include <TimerOne.h>

int lastSerialWriteClick = 0;

double power = 90; //current power

char incomingCharacter;

int clickCount = 0;
int prevClickCount = 0;
int clickTarget = 0;

int cameraFocusPin = 2;
int cameraShutterPin = 3;

int subjectLightPin = 4;  //normally ON
int leftDoorLightPin = 6; //normally ON
int rightDoorLightPin = 7; //normally ON
int laserLightPin = 5; //normally ON

int MegaMotoPWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long startTime, lastUpdateTime, now, aPinLastChangeTime, bPinLastChangeTime, thisCaptureStartTime, lastFocusTime, prevCaptureTime, lastCapture;

unsigned long lastPowerChangeTime, captureStartTime, waitForTurntableStop = 350;

bool capturing = false, turntableStopping = false, shutterOpen = false;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

int encoderClicksPerSpin = 8192;
int shutterReleaseCompensation = 0;
int captures, captureTarget, capturePeriod, focusPeriod, captureIntervalClicks;

void setup()
{
  Timer1.initialize(100);  //100us = 10khz

  //myPid.SetMode(MANUAL);
  //myPid.SetOutputLimits(powerMin, powerMax);
  //myPid.SetSampleTime(pidSampleInterval);

  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (MegaMotoPWMpin, OUTPUT);

  pinMode (subjectLightPin, OUTPUT);
  pinMode (leftDoorLightPin, OUTPUT);
  pinMode (rightDoorLightPin, OUTPUT);
  pinMode (laserLightPin, OUTPUT);

  pinMode(cameraFocusPin, OUTPUT);
  pinMode(cameraShutterPin, OUTPUT);

  stopCapture();
  stopFocus();

  switchLightsOFF();

  aPinReading = digitalRead(encoderPinA);
  aPinShort = aPinReading;
  aPinLong = aPinReading;

  bPinReading = digitalRead(encoderPinB);
  bPinShort = bPinReading;
  bPinLong = bPinReading;

  Serial.begin (230400);
}


void loop()
{
  now = millis();

  if (clickTarget == 0)
  {
    incomingCharacter = Serial.read();
    //incomingCharacter = '1';

    switch (incomingCharacter) {
      case 'F':
        focus();
        delay(500);
        stopFocus();
        break;
      case 'C':
        capture();
        break;
      case 'S':
        stopCapture();
        break;
      case '1':
        
        captureTarget = 88; //72
        capturePeriod = 300; //300 works well on single shot mode
        focusPeriod = 10;   //10 works well on single shot mode
        captureIntervalClicks = (encoderClicksPerSpin / captureTarget) + 1;

        Serial.println(captureIntervalClicks);
        
        initializeSpin();
        break;
      case 'L':
        switchLightsON();
        break;
      case 'D':
        switchLightsOFF();
        break;
    }
  }
  else
  {
    if(power > 0)
    {
              aPinReading = digitalRead(encoderPinA);
            
              if (aPinReading == 0 && aPinLong == 1)
              {
                aPinLastChangeTime = now;
                aPinShort = 0;
              }
            
              if (aPinReading == 1 && aPinLong == 0)
              {
                aPinLastChangeTime = now;
                aPinShort = 1;
              }
            
              if (aPinLong != aPinShort)
              {
                clickCount++;
                aPinLong = aPinShort;
              }
            
              bPinReading = digitalRead(encoderPinB);
            
              if (bPinReading == 0 && bPinLong == 1)
              {
                bPinLastChangeTime = now;
                bPinShort = 0;
              }
            
              if (bPinReading == 1 && bPinLong == 0)
              {
                bPinLastChangeTime = now;
                bPinShort = 1;
              }
            
              if (bPinLong != bPinShort)
              {
                clickCount++;
                bPinLong = bPinShort;
              }
    }
    if(capturing == false)
    {
        if(clickCount % captureIntervalClicks == 0 && clickCount > lastCapture)
        {
          capturing = true;
          turntableStopping = true;
          captureStartTime = now;
          power = 0;
        }
    }
    if(capturing == false)
    {
      if(now > lastPowerChangeTime + 5)
      {
        if(now > bPinLastChangeTime + 5 && now > aPinLastChangeTime + 5)
        {
            power++;
            lastPowerChangeTime = now;
        }
        else
        {
            //power=80;
            //lastPowerChangeTime = now;
        }
      }
    }
    
    Timer1.pwm(MegaMotoPWMpin, power);
    
    if(capturing == true)
    {
       if(now > captureStartTime + waitForTurntableStop)
       {
            turntableStopping = false;
       }
        if(turntableStopping == false)
        {
             if(shutterOpen == false)
             {
                  shutterOpen = true;
                  capture();
             }
             if(shutterOpen == true && now > captureStartTime + waitForTurntableStop + capturePeriod)
             {
                    stopCapture();
                    shutterOpen = false;
                    capturing = false;
                    turntableStopping = false;
                    lastCapture = clickCount;
                    power = 80;
             }
        }
    }

    
    if (clickCount > clickTarget && clickCount > 0)
    {
      captures = 0;
      clickCount = 0;
      clickTarget = 0;
      digitalWrite(MegaMotoPWMpin, LOW);
  
      stopCapture();
      stopFocus();

      delay(1000);
      
      //only switch off lights at end of turntable rotation
      //if (clickTarget != 0) {
        switchLightsOFF();
      //}
      Serial.println("Capture Finished");
    }
  }
}

void focus() {
  Serial.println("focus");
  
  digitalWrite(cameraFocusPin, LOW);
  
  lastFocusTime = now;
}

void stopFocus() {
  Serial.println("stop focus");
  
  digitalWrite(cameraFocusPin, HIGH);
}

void capture() {
  Serial.println("capture");
  
  thisCaptureStartTime = now;
  digitalWrite(cameraShutterPin, LOW);
  captures++;
  Serial.print(captures); //we start at zero
  Serial.print("\t");
  Serial.print(now - startTime);
  Serial.print("\t");
  Serial.print(clickCount);
  Serial.print("\t");
  Serial.println(now - prevCaptureTime);

  prevCaptureTime = now;
}

void stopCapture() {
  Serial.println("stop capture");
  digitalWrite(cameraShutterPin, HIGH);
}

void initializeSpin()
{
  Serial.println("Initialize Spin");
  
  lastSerialWriteClick = 0;

  startTime = now;
  lastUpdateTime = now;
  aPinLastChangeTime = now;
  bPinLastChangeTime = now;

  clickCount = 0;
  prevClickCount = 0;
  
  prevCaptureTime = now;
    
  clickTarget = encoderClicksPerSpin;

  switchLightsON();

  focus();
  delay(500);
  stopFocus();
  delay(500);
  stopCapture();
  delay(500);
  capture();
  delay(capturePeriod);
  stopCapture();
  //delay(500);
  lastCapture = 0;  
}

void switchLightsOFF()
{
  digitalWrite(subjectLightPin, HIGH);
  digitalWrite(leftDoorLightPin, LOW);
  digitalWrite(rightDoorLightPin, LOW);
  digitalWrite(laserLightPin, HIGH);

}

void switchLightsON()
{
  digitalWrite(subjectLightPin, LOW);
  //digitalWrite(leftDoorLightPin, HIGH);
  //digitalWrite(rightDoorLightPin, HIGH);
  digitalWrite(laserLightPin, LOW);
}
