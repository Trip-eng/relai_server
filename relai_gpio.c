#include <wiringPi.h>

int pins[8] = {0,1,2,3,4,5,6,7};

void initRelai() {
	if (wiringPiSetup() == -1)
	{  /*error log*/ }
	for (int i=0; i<sizeof(pins); i++ )
	{
  		pinMode(pins[i], OUTPUT);
	}
}

void setRelaiState(int relai, int state)
{
  	digitalWrite(pins[relai],( state + 1) % 2);
}

int getRelaiState(int relai)
{
	return (digitalRead(pins[relai]) + 1) % 2;
}
