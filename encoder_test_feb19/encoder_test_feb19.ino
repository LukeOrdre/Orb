#include <TimerOne.h>
#include <PID_v1.h>

double pidInput, pidOutput, pidSetpoint = 0.75;

PID myPid(&pidInput, &pidOutput, &pidSetpoint, 1.5, 50, 0, DIRECT); //1.5, 15, 0 seems to work

int MegaMotoPWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

int clickCount;
int lastClickCount;
int prevClickCount;
int clickTarget;

int encoderClicksPerSpin = 8192;

int captureTarget = 88; //72
int capturePeriod = 300; //300 works well on single shot mode
int focusPeriod = 10;   //10 works well on single shot mode
int captureIntervalClicks = (encoderClicksPerSpin / captureTarget) + 1;



int pidSampleInterval = 1; //ms between samples

double powerMin = 0; // dependent on voltage & PWM frequency

double powerMax = 500; // turntable can get stuck if this is too low

int power = 0;
int frictionFactor = 40;
int basePower = 40;

unsigned long startTime, lastUpdateTime, now, lastClickTime = 0, lastPowerChangeTime = 0;

unsigned long clickWindow = 10, turntableStopTime;

double turntableSpeed;

bool stoppingTurntable, forceStart;

void setup() {
  // put your setup code here, to run once:

  Timer1.initialize(100);  //100us = 10khz

  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (MegaMotoPWMpin, OUTPUT);

  
  aPinReading = digitalRead(encoderPinA);
  aPinShort = aPinReading;
  aPinLong = aPinReading;

  bPinReading = digitalRead(encoderPinB);
  bPinShort = bPinReading;
  bPinLong = bPinReading;


  myPid.SetMode(AUTOMATIC);
  myPid.SetOutputLimits(powerMin, powerMax);
  myPid.SetSampleTime(pidSampleInterval);

  myPid.SetTunings(0.1, 30, 0);  //startup mode, smooth start

  forceStart = true;
  
  Serial.begin (230400);
}

void loop() {
  // put your main code here, to run repeatedly:
  now = millis();
  aPinReading = digitalRead(encoderPinA);
  
    if (aPinReading == 0 && aPinLong == 1)
    {
      lastClickTime = now;
      aPinShort = 0;
    }
  
    if (aPinReading == 1 && aPinLong == 0)
    {
      lastClickTime = now;
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
      lastClickTime = now;
      bPinShort = 0;
    }
  
    if (bPinReading == 1 && bPinLong == 0)
    {
      lastClickTime = now;
      bPinShort = 1;
    }
  
    if (bPinLong != bPinShort)
    {
      clickCount++;
      bPinLong = bPinShort;
    }

    if(clickCount != lastClickCount)
    {
      lastClickCount = clickCount;
    }

    if (now % clickWindow == 0 && now != lastUpdateTime)
    {
      lastUpdateTime = now;
  
      turntableSpeed = 10 * ((double)clickCount - (double)prevClickCount);
  
      prevClickCount = clickCount;
  
      pidInput = turntableSpeed;
      //Serial.print(10+(5*turntableSpeed));
      //Serial.print("\t");
      //Serial.println(pidOutput);
  
    }
    
    if(clickCount > 0 && clickCount % captureIntervalClicks == 0 && stoppingTurntable == false)
    {
        stoppingTurntable = true;
        turntableStopTime = now;
        pidSetpoint = 0;
    }

    if(now > turntableStopTime + 350)
    { 
        stoppingTurntable = false;
    }
    
    if(stoppingTurntable == false)
    {
        pidSetpoint = 5 + (2 * (1 - cos(clickCount * 0.0184)));
    }
    
    myPid.Compute();
    power = (int)pidOutput;

    Timer1.pwm(MegaMotoPWMpin, power);

    if(now % 50 == 0)
    {
         Serial.print(pidInput);
         Serial.print("\t");
         Serial.print(pidSetpoint);
         Serial.print("\t");
         Serial.print(power);
         Serial.print("\t");
         Serial.print(clickCount);
         Serial.print("\t");
         Serial.print(clickCount % captureIntervalClicks);
         Serial.print("\t");
         Serial.println(now - lastClickTime);
    }
}
