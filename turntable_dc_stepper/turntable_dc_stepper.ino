int MegaMotoPWMpin = 11;

void setup() {
  // put your setup code here, to run once:
  pinMode(MegaMotoPWMpin, OUTPUT);
  setPwmFrequency(MegaMotoPWMpin, 1);  // change Timer2 divisor to 8 gives 3.9kHz PWM freq
}

void loop() 
{
///slow option 1 
    
//    for(int i=28; i<100; i++)
//    {
//        analogWrite(MegaMotoPWMpin, i);
//        delayMicroseconds(40);
//    }
//    for(int i=100; i>28; i--)
//    {
//        analogWrite(MegaMotoPWMpin, i);
//        delayMicroseconds(40);
//    }
//    delay(25);

    
//    for(int i=15; i<80; i++)
//    {
//        analogWrite(MegaMotoPWMpin, i);
//        delayMicroseconds(40);
//    }
//    for(int i=80; i>15; i--)
//    {
//        analogWrite(MegaMotoPWMpin, i);
//        delayMicroseconds(40);
//    }
//    delay(25);    
//
//slow option 2
//    analogWrite(MegaMotoPWMpin, 150);
//    delay(1);
//    analogWrite(MegaMotoPWMpin, 255);
//    delay(1);
//    analogWrite(MegaMotoPWMpin, 150);
//    delay(1);
//    digitalWrite(MegaMotoPWMpin, LOW);
//    delay(70);

//slow option 2
//    analogWrite(MegaMotoPWMpin, 150);
//    delayMicroseconds(500);
//    analogWrite(MegaMotoPWMpin, 255);
//    delayMicroseconds(500);
//    analogWrite(MegaMotoPWMpin, 150);
//    delayMicroseconds(500);
//    digitalWrite(MegaMotoPWMpin, LOW);
//    delay(30);    

//slow option 4
    digitalWrite(MegaMotoPWMpin, HIGH);
    delay(10);
    digitalWrite(MegaMotoPWMpin, LOW);
    delay(1000);


//    analogWrite(MegaMotoPWMpin, 1);

//    analogWrite(MegaMotoPWMpin, 50);
//    delay(1);
//    analogWrite(MegaMotoPWMpin, 40);
//    delay(1);
//    analogWrite(MegaMotoPWMpin, 30);
//    delay(1);
//    analogWrite(MegaMotoPWMpin, 20);

  //}

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
