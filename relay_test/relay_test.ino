int relayPinA = 7;
char incomingCharacter;
void setup() 
{
  pinMode(relayPinA, OUTPUT);
  Serial.begin (9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  incomingCharacter = Serial.read();

switch (incomingCharacter) {
       case '1':
         digitalWrite(relayPinA, HIGH);
         Serial.println("High");
        break;
       case '2':
         digitalWrite(relayPinA, LOW);
         Serial.println("LOW");
        break;
       case '3':
        break;
      }
}
