#include <wiringPi.h>

int pins[8] = {5,6,2,3,4,5,6,7};
int SCL = 24;
int SDA = 25;
void initRelai() {
	if (wiringPiSetup() == -1)
	{  /*error log*/ }
	for (int i=0; i<sizeof(pins); i++ )
	{
  		pinMode(pins[i], OUTPUT);
	}
	pinMode(SCL, OUTPUT);
	pinMode(SDA, OUTPUT); 
	digitalWrite(SCL, 1);
	digitalWrite(SDA, 1);
}

void setRelaiState(int relai, int state)
{
  	digitalWrite(pins[relai],( state + 1) % 2);
}

int getRelaiState(int relai)
{
	return (digitalRead(pins[relai]) + 1) % 2;
}
void myWait(void)
{
	usleep(1);
}

void sendTWI(unsigned long data)
{
	char bit = 0;
	char send = 0;
	// Start
	digitalWrite(SDA, 0);
        myWait();
	digitalWrite(SCL, 0);
        myWait();

	// Send
	for(int i = 63 ; i >=0;i--)
	{
		bit = ((data >> i) % 2);		
		if(send || bit == 1)
		{
			send = 1;
			digitalWrite(SDA, bit);
                	myWait();
			digitalWrite(SCL, 1);
                	myWait();
			digitalWrite(SCL, 0);
                	myWait();
		}
	}

	// Stop
	digitalWrite(SDA, 0);
        myWait();
	digitalWrite(SCL, 1);
        myWait();
	digitalWrite(SDA, 1);

}
