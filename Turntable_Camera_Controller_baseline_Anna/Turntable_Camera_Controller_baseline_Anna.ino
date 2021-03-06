#include <PID_v1.h>
#include <TimerOne.h>

int lastSerialWriteClick = 0;
// 4096 encoder clicks, both A & B = 8192.... 14500ms spin, 80 frames at 5.5fps.
// 14.5 seconds is 483 lots of 30ms
// 8192 / 483 = 17 clicks per window
double pidInput, pidOutput, pidSetpoint = 0.75;

int clickWindow; //window measured in ms for counting ticks

double turntableSpeed = 0;
double instantTurntableSpeed = 0;

int pidSampleInterval = 1; //ms between samples

PID myPid(&pidInput, &pidOutput, &pidSetpoint, 1.5, 50, 0, DIRECT); //1.5, 15, 0 seems to work

double powerMin = 105; // dependent on voltage & PWM frequency

double powerMax = 500; // turntable can get stuck if this is too low

double power = 0; //current power

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

unsigned long startTime, lastUpdateTime, now, aPinLastChangeTime, bPinLastChangeTime, thisCaptureStartTime, lastFocusTime, prevCaptureTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

int encoderClicksPerSpin = 8192;
int shutterReleaseCompensation = 0;
int captures, captureTarget, capturePeriod, focusPeriod = 100, captureIntervalClicks;

bool startupMode = true;

void setup()
{
  Timer1.initialize(100);  //100us = 10khz

  myPid.SetMode(MANUAL);
  myPid.SetOutputLimits(powerMin, powerMax);
  myPid.SetSampleTime(pidSampleInterval);

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
    getInstruction();
  }
  else
  {
    countClicks();
  
    myPid.Compute();
    
    power = (int)pidOutput;
    
    Timer1.pwm(MegaMotoPWMpin, power);
  
    if (now % clickWindow == 0 && now != lastUpdateTime && clickTarget != 0)
    {
      lastUpdateTime = now;
      
      if(startupMode)
      {
        turntableSpeed = (double)clickCount - (double)prevClickCount;
      }
      else
      {
        instantTurntableSpeed = 30 * ((double)clickCount - (double)prevClickCount);
        turntableSpeed = (0.9 * turntableSpeed) + (0.1 * instantTurntableSpeed);
      }
      
      //if(turntableSpeed > pidSetpoint) turntableSpeed += 0.01;
      //if(turntableSpeed > pidSetpoint) turntableSpeed -= 0.01;
      
      prevClickCount = clickCount;
      pidInput = turntableSpeed;

    }

    if (now % 100)
    {
      Serial.print(5*captures);
      Serial.print("\t");
      Serial.print(turntableSpeed);
      Serial.print("\t");
      Serial.println(power);
    }

    if(clickCount > 150 && startupMode == true)
    {
       startupMode = false;
       clickWindow = 2;
       myPid.SetTunings(25, 1, 0); //normal running metronome, but isn't a perfectly smooth start
    }

    if(clickCount > 0 && clickTarget > 0)
    {
      if(clickCount % captureIntervalClicks == 0 && thisCaptureStartTime == 0)
      {
          if(captures < captureTarget)
          {
            capture();
          }
      }
      
      if(now >= thisCaptureStartTime + capturePeriod && thisCaptureStartTime != 0)
      {
          stopCapture();
          //focus();
          thisCaptureStartTime = 0;
      }

    }
    if (clickCount > clickTarget && clickCount > 0)
    {
      captures = 0;
      clickCount = 0;
      clickTarget = 0;
      turntableSpeed = 0;
      digitalWrite(MegaMotoPWMpin, LOW);
  
      stopCapture();
      stopFocus();
      pidSetpoint = 0;
      pidInput = 0;
      myPid.SetMode(MANUAL);
      myPid.Compute();
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
  //Serial.println("capture");
  
  thisCaptureStartTime = now;
  digitalWrite(cameraShutterPin, LOW);
  captures++;
  //Serial.println(captures); //we start at zero
  //Serial.print("\t");
  //Serial.print(now - startTime);
  //Serial.print("\t");
  //Serial.print(clickCount);
  //Serial.print("\t");
  //Serial.println(now - prevCaptureTime);

  prevCaptureTime = now;
}

void stopCapture() {
  //Serial.println("stop capture");
  digitalWrite(cameraShutterPin, HIGH);
}

void initializeSpin()
{
  Serial.println("Initialize Spin");
  
  lastSerialWriteClick = 0;

  power = powerMin;

  startTime = now;
  lastUpdateTime = now;
  aPinLastChangeTime = now;
  bPinLastChangeTime = now;

  clickCount = 0;
  prevClickCount = 0;
  
  prevCaptureTime = now;
    
  clickTarget = encoderClicksPerSpin;

  switchLightsON();

  myPid.SetOutputLimits(powerMin, powerMax);
  myPid.SetMode(AUTOMATIC);

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
  digitalWrite(leftDoorLightPin, HIGH);
  digitalWrite(rightDoorLightPin, HIGH);
  digitalWrite(laserLightPin, LOW);
}

void getInstruction()
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
        pidSetpoint = 2; // 2 for metronome
        clickWindow = 25; // 4 for metronome
        myPid.SetTunings(0.1, 10, 0);  //startup mode, smooth start
        //myPid.SetTunings(10, 1, 0);
        //myPid.SetTunings(5, 1000, 0); //normal running metronome, but isn't a perfectly smooth start
        //myPid.SetTunings(0.1, 5, 0);
        //myPid.SetTunings(1.5, 2, 0);
        //myPid.SetTunings(0.1, 10, 0);
        //myPid.SetTunings(3, 20, 0);
        //myPid.SetTunings(5, 2, 0);
        //myPid.SetTunings(80, 10, 0);
        //myPid.SetTunings(10, 1, 0);
        //myPid.SetTunings(2, 1, 0);
        //myPid.SetTunings(, 5, 0);
        startupMode = true;
        captureTarget = 88; //72
        capturePeriod = 100; //300 works well on single shot mode
        //focusPeriod = 10;   //10 works well on single shot mode
        captureIntervalClicks = (encoderClicksPerSpin / captureTarget) + 1;

        //Serial.println(captureIntervalClicks);
        
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

void countClicks()
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

