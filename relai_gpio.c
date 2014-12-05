#include <wiringPi.h>

int pins[10] = {0,1,2,3,4,5,6,7,8,9};

void init() {
  if (wiringPiSetup() == -1)
  {  /*error log*/ }
  for ( i=0; i<sizeof(s0); i++ )
  	pinMode(pins[i], OUTPUT);
}

void setPinState(int pin, int state)
{
  digitalWrite(pins[pin], state);
}

int getPinState(int pin)
{
  return digitalRead(pins[pin]);
}
