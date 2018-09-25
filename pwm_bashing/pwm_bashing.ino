void setup ()
{
  pinMode (4, OUTPUT) ;
  TCCR2A = 0x23 ;
  TCCR2B = 0x09 ; // mode 7, clock prescale by 1
  OCR2A = 160-1 ;  // 160 clock periods = 10us per cycle
  OCR2B =0 ;
  TCNT2 =0 ;
}

void loop ()
{
  // here you can set the duty cycle by writing values between 0 and 160 to 
  OCR2B = 160;
}
