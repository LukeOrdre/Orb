int outPin = 4;
int highdelay = 1;
int lowdelay = 1;

void setup()
{
  pinMode(outPin, OUTPUT);
  Serial.begin(9600); 
}

void loop()
{
/*

  
  if(lowdelay % 100 == 0)
  {
  Serial.print(lowdelay);
  //Serial.print("    ");
  //Serial.print(lowdelay * 20);
  Serial.print("\n");
  }
*/
  
  digitalWrite(outPin, HIGH);
  //delay(1);
  delayMicroseconds(lowdelay);
  digitalWrite(outPin, LOW);
  //delay(10);
  delayMicroseconds(lowdelay);
  
/*  
  lowdelay += 1;
  
  if(lowdelay > 1000) {
      lowdelay=1; 
      highdelay+=1000;
  }
*/  
}
