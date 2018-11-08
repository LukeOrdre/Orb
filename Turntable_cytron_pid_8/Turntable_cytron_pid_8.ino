#include <PID_v1.h>
#include <TimerOne.h>

bool debugMode = true;

// 4096 encoder clicks, both A & B = 8192.... 14500ms spin, 80 frames at 5.5fps.
// 14.5 seconds is 483 lots of 30ms
// 8192 / 483 = 17 clicks per window

double pidInput, pidOutput, pidSetpoint = 0.75;

int clickWindow = 28; //window measured in ms for counting ticks

double turntableSpeed = 0;

int pidSampleInterval = 1; //ms between samples

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

int subjectLightPin = 1;  //normally ON
int leftDoorLightPin = 2; //normally ON
int rightDoorLightPin = 3; //normally ON
int laserLightPin = 4; //normally ON

int PWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long startTime, lastUpdateTime, now, aPinLastChangeTime, bPinLastChangeTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

void setup()
{
    Timer1.initialize(100);  //100us = 10khz
  
    myPid.SetMode(MANUAL);
    myPid.SetOutputLimits(powerMin, powerMax);
    myPid.SetSampleTime(pidSampleInterval);

    pinMode (encoderPinA,INPUT);
    pinMode (encoderPinB,INPUT);
    pinMode (PWMpin, OUTPUT);
    
    pinMode (subjectLightPin, OUTPUT);
    pinMode (leftDoorLightPin, OUTPUT);
    pinMode (rightDoorLightPin, OUTPUT);
    pinMode (laserLightPin, OUTPUT);

    switchLightsOFF();

    aPinReading = digitalRead(encoderPinA);
    aPinShort = aPinReading;
    aPinLong = aPinReading;

    bPinReading = digitalRead(encoderPinB);
    bPinShort = bPinReading;
    bPinLong = bPinReading;

    Serial.begin (115200);
}


void loop()
{
    now = millis();

    if(clickTarget == 0)
    {
        incomingCharacter = Serial.read();
        //incomingCharacter = '1';

        switch (incomingCharacter) {
        case '1':
            pidSetpoint = 7.5;
            clickWindow = 28;
            myPid.SetTunings(1.5, 20, 0);
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

        }
    }

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

    if(aPinLong != aPinShort)
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

    if(bPinLong != bPinShort)
    {
        recordClick();
        bPinLong = bPinShort;
    }

    myPid.Compute();
    power = (int)pidOutput;
    //power=Tf*(power-oldpower)/Ts+oldpower;
    //oldpower = power;

    Timer1.pwm(PWMpin,power);  

    if(now % clickWindow == 0 && now != lastUpdateTime && clickTarget != 0)
    {
        lastUpdateTime = now;

        turntableSpeed = 10 * ((double)clickCount - (double)prevClickCount) / (double)clickWindow;

        prevClickCount = clickCount;

        pidInput = turntableSpeed;

        if(debugMode)
        {

            Serial.print(lastUpdateTime - startTime);
            Serial.print("\t");
            Serial.print(clickCount);
            Serial.print("\t");
            Serial.print(turntableSpeed);
            Serial.print("\t");
            Serial.print(power);
            Serial.println();
        }
    }

    if(clickCount >= clickTarget)
    {
        clickCount = 0;
        clickTarget = 0;
        digitalWrite(PWMpin, LOW);
        
        switchLightsOFF();
        
        myPid.SetMode(MANUAL);
        delay(2000);
    }
}

void recordClick()
{
    clickCount++;
}

void initializeSpin()
{
    power = powerMin;

    startTime = now;
    lastUpdateTime = now;
    aPinLastChangeTime = now;
    bPinLastChangeTime = now;

    clickCount = 0;
    prevClickCount = 0;
    clickTarget = 16384;

    switchLightsON();

    myPid.SetOutputLimits(powerMin, powerMax);
    myPid.SetMode(AUTOMATIC);

}

void switchLightsOFF()
{
    digitalWrite(subjectLightPin, LOW);
    digitalWrite(leftDoorLightPin, LOW);
    digitalWrite(rightDoorLightPin, LOW);
    digitalWrite(laserLightPin, HIGH);
}

void switchLightsON()
{
    digitalWrite(subjectLightPin, HIGH);
    digitalWrite(leftDoorLightPin, HIGH);
    digitalWrite(rightDoorLightPin, HIGH);
    digitalWrite(laserLightPin, LOW);
}

