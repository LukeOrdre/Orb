int relayPinA = 7;
int relayPinB = 8;
int relayPinC = 9;
int relayPinD = 10;

char incomingCharacter;

void setup() 
{
  pinMode(relayPinA, OUTPUT);
  pinMode(relayPinB, OUTPUT);
  pinMode(relayPinC, OUTPUT);
  pinMode(relayPinD, OUTPUT);
  Serial.begin (9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  incomingCharacter = Serial.read();

switch (incomingCharacter) {
       case '1':
         digitalWrite(relayPinA, HIGH);
         digitalWrite(relayPinB, HIGH);
         digitalWrite(relayPinC, HIGH);
         digitalWrite(relayPinD, HIGH);
         Serial.println("High");
        break;
       case '2':
         digitalWrite(relayPinA, LOW);
         digitalWrite(relayPinB, LOW);
         digitalWrite(relayPinC, LOW);
         digitalWrite(relayPinD, LOW);
         Serial.println("LOW");
        break;
       case '3':
        break;
      }
}
