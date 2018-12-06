#include <PID_v1.h>
#include <TimerOne.h>

int lastSerialWriteClick = 0;
// 4096 encoder clicks, both A & B = 8192.... 14500ms spin, 80 frames at 5.5fps.
// 14.5 seconds is 483 lots of 30ms
// 8192 / 483 = 17 clicks per window
double pidInput, pidOutput, pidSetpoint = 0.75;

int clickWindow = 28; //window measured in ms for counting ticks

double turntableSpeed = 0;

int pidSampleInterval = 1; //ms between samples

int pwmDivisor = 1; // 1 for 31.2khz (inaudible) or 8 (audible) for 3.9khz Timer 2 PWM frequency

PID myPid(&pidInput, &pidOutput, &pidSetpoint, 1.5, 50, 0, DIRECT); //1.5, 15, 0 seems to work

double powerMin = 40; // dependent on voltage & PWM frequency. At 24V with divisor 1:
// 27 turns in 11 seconds with the lid off
// 30 is enough to overcome the lowest friction with the lid on, and crawl around.
// at 12v with divisor 1:  40 to 90 seems like a good range

double powerMax = 500; // turntable can get stuck if this is too low

double power = 0; //current power

char incomingCharacter;

int clickCount = 0;
int prevClickCount = 0;
int clickTarget = 0;

int subjectLightPin = 4;  //normally ON
int leftDoorLightPin = 6; //normally ON
int rightDoorLightPin = 7; //normally ON
int laserLightPin = 5; //normally ON

int cameraFocusPin = 2;
int cameraShutterPin = 3;

int MegaMotoPWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long startTime, lastUpdateTime, now, aPinLastChangeTime, bPinLastChangeTime, lastCaptureTime, lastFocusTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

//360/73=4.93
//8192/360=22.76
//1 img spacing = 112
//90deg 444
//448

int encoderClicksPerSpin = 8192;
int shutterReleaseCompensation = 0;
int captureTarget = 72;
int captures = 0;
int capturePeriod = 50;
int focusPeriod = 150;

int captureIntervalClicks = (encoderClicksPerSpin / captureTarget) + 1;

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
        pidSetpoint = 3;
        clickWindow = 20;
        myPid.SetTunings(3, 20, 0);
        initializeSpin();
        break;
      case '2':
        pidSetpoint = 8.8;
        myPid.SetTunings(1.5, 15, 0);
        clickWindow = 25;
        initializeSpin();
        break;
      case '3':
        pidSetpoint = 11.25;
        myPid.SetTunings(1.5, 10, 0);
        clickWindow = 24;
        initializeSpin();
        break;
      case '4':
        pidSetpoint = 15;
        myPid.SetTunings(1.5, 5, 0);
        clickWindow = 24;
        initializeSpin();
        break;
      case '5':
        pidSetpoint = 22.5;
        myPid.SetTunings(1, 3, 0);
        clickWindow = 16;
        initializeSpin();
        break;
      case '6':
        pidSetpoint = 9.5;
        clickWindow = 28;
        myPid.SetTunings(1.5, 20, 0);
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
      recordClick();
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
      recordClick();
      bPinLong = bPinShort;
    }
  
    myPid.Compute();
    power = (int)pidOutput;
    //power=Tf*(power-oldpower)/Ts+oldpower;
    //oldpower = power;
    Timer1.pwm(MegaMotoPWMpin, power);
  
    if (now % clickWindow == 0 && now != lastUpdateTime && clickTarget != 0)
    {
      lastUpdateTime = now;
  
      turntableSpeed = 10 * ((double)clickCount - (double)prevClickCount) / (double)clickWindow;
  
      prevClickCount = clickCount;
  
      pidInput = turntableSpeed;
  
    }
  
    int currPrintPos = clickCount / 20;
    if ((clickCount % 20 == 0 && currPrintPos > lastSerialWriteClick) || currPrintPos > lastSerialWriteClick) {
      lastSerialWriteClick = currPrintPos;
    }
  
    if(clickCount > 0 && clickTarget > 0)
    {
      if(clickCount % captureIntervalClicks == 0 && lastCaptureTime == 0)
      {
          if(captures < captureTarget)
          {
            capture();
          }
      }
      
      if(now >= lastCaptureTime + capturePeriod)
      {
          stopCapture();
          focus();
          lastCaptureTime = 0;
      }

      if(now >= lastFocusTime + focusPeriod)
      {
          stopFocus();
          lastFocusTime = 0;
      }
    }
    if (clickCount > clickTarget && clickCount > 0)
    {
      //only switch off lights at end of turntable rotation
      if (clickTarget != 0) {
        switchLightsOFF();
      }
      
      captures = 0;
      clickCount = 0;
      clickTarget = 0;
      digitalWrite(MegaMotoPWMpin, LOW);
  
      stopCapture();
      stopFocus();
      
      myPid.SetMode(MANUAL);
      delay(1000);
      Serial.println("Capture Finished");
    }
  }
}

void recordClick()
{
  clickCount++;
}

void focus() {
  digitalWrite(cameraFocusPin, LOW);
  lastFocusTime = now;
}

void stopFocus() {
  digitalWrite(cameraFocusPin, HIGH);
}

void capture() {
  lastCaptureTime = now;
  
  //digitalWrite(cameraFocusPin, LOW);
  digitalWrite(cameraShutterPin, LOW);

  Serial.print(captures++); //we start at zero
  Serial.print("\t");
  Serial.print(now - startTime);
  Serial.print("\t");
  Serial.println(clickCount);
  
}

void stopCapture() {
  digitalWrite(cameraFocusPin, HIGH);
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
  clickTarget = encoderClicksPerSpin;

  switchLightsON();

  myPid.SetOutputLimits(powerMin, powerMax);
  myPid.SetMode(AUTOMATIC);

  focus();
  delay(500);
  capture();
  delay(capturePeriod);
  stopCapture();
  delay(500);
  focus();

  
}

void switchLightsOFF()
{
  digitalWrite(subjectLightPin, HIGH);
  digitalWrite(leftDoorLightPin, LOW);
  digitalWrite(rightDoorLightPin, LOW);
  digitalWrite(laserLightPin, HIGH);

////  digitalWrite(0, HIGH);
////  digitalWrite(1, LOW);
//  digitalWrite(2, HIGH);
//  digitalWrite(3, HIGH);
//  digitalWrite(4, HIGH);
//  digitalWrite(5, HIGH);
//  digitalWrite(6, HIGH);
//  digitalWrite(7, HIGH);
}

void switchLightsON()
{
  digitalWrite(subjectLightPin, LOW);
  digitalWrite(leftDoorLightPin, HIGH);
  digitalWrite(rightDoorLightPin, HIGH);
  digitalWrite(laserLightPin, LOW);
}
