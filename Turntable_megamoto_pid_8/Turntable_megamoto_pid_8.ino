#include <PID_v1.h>
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

double powerMax = 150; // turntable can get stuck if this is too low

double power = 0; //current power

char incomingCharacter;

int clickCount = 0;
int prevClickCount = 0;
int clickTarget = 0;

int subjectLightPin = 1;  //normally ON
int leftDoorLightPin = 2; //normally ON
int rightDoorLightPin = 3; //normally ON
int laserLightPin = 4; //normally ON


int MegaMotoPWMpin = 11;
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
    myPid.SetMode(MANUAL);
    myPid.SetOutputLimits(powerMin, powerMax);
    myPid.SetSampleTime(pidSampleInterval);

    pinMode (encoderPinA,INPUT);
    pinMode (encoderPinB,INPUT);
    pinMode (MegaMotoPWMpin, OUTPUT);
    
    pinMode (subjectLightPin, OUTPUT);
    pinMode (leftDoorLightPin, OUTPUT);
    pinMode (rightDoorLightPin, OUTPUT);
    pinMode (laserLightPin, OUTPUT);

    switchLightsOFF();

    setPwmFrequency(MegaMotoPWMpin, pwmDivisor);

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

    if(clickTarget == 0)
    {
        incomingCharacter = Serial.read();
        //incomingCharacter = '1';

        switch (incomingCharacter) {
        case '1':
            pidSetpoint = 7.4;
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
        case 'L':
            switchLightsON();
            break;
        case 'D':
            switchLightsOFF();
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
    analogWrite(MegaMotoPWMpin, power);

    if(now % clickWindow == 0 && now != lastUpdateTime && clickTarget != 0)
    {
        lastUpdateTime = now;

        turntableSpeed = 10 * ((double)clickCount - (double)prevClickCount) / (double)clickWindow;

        prevClickCount = clickCount;

        pidInput = turntableSpeed;

    }

    int currPrintPos = clickCount / 20;
    if((clickCount % 20 == 0 && currPrintPos > lastSerialWriteClick) || currPrintPos > lastSerialWriteClick){
        lastSerialWriteClick = clickCount / 20;
        Serial.print(lastUpdateTime - startTime);
        Serial.print("\t");
        Serial.print(clickCount);
        Serial.print("\t");
        Serial.print(turntableSpeed);
        Serial.print("\t");
        Serial.print(power);
        Serial.println();
    }

    if(clickCount >= clickTarget)
    {
        //only switch off lights at end of turntable rotation
        if(clickTarget != 0){
          switchLightsOFF();
        }
      
        clickCount = 0;
        clickTarget = 0;
        digitalWrite(MegaMotoPWMpin, LOW);
        
        myPid.SetMode(MANUAL);
        delay(1000);
    }
}

void recordClick()
{
    clickCount++;
}

void initializeSpin()
{
    lastSerialWriteClick = 0;
  
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

void setPwmFrequency(int pin, int divisor) {
    byte mode;
    if(pin == 5 || pin == 6 || pin == 9 || pin == 10) { // Timer0 or Timer1
        switch(divisor) {
        case 1:
            mode = 0x01;
            break;
        case 8:
            mode = 0x02;
            break;
        case 64:
            mode = 0x03;
            break;
        case 256:
            mode = 0x04;
            break;
        case 1024:
            mode = 0x05;
            break;
        default:
            return;
        }
        if(pin == 5 || pin == 6) {
            TCCR0B = TCCR0B & 0b11111000 | mode; // Timer0
        } else {
            TCCR1B = TCCR1B & 0b11111000 | mode; // Timer1
        }
    } else if(pin == 3 || pin == 11) {
        switch(divisor) {
        case 1:
            mode = 0x01;
            break;
        case 8:
            mode = 0x02;
            break;
        case 32:
            mode = 0x03;
            break;
        case 64:
            mode = 0x04;
            break;
        case 128:
            mode = 0x05;
            break;
        case 256:
            mode = 0x06;
            break;
        case 1024:
            mode = 0x7;
            break;
        default:
            return;
        }
        TCCR2B = TCCR2B & 0b11111000 | mode; // Timer2
    }
}


