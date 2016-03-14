#include <wiringPi.h>

int pins[8] = {29,5,4,15,3,7,9,8};
//int pins[0] ={};
int SCL = 0;
int SDA = 0;
int S_SER=3;
int S_RCLK=4;
int S_SRCLK=5;

unsigned char S_DATA=0xFF;
void initRelai() {
	if (wiringPiSetup() == -1)
	{  /*error log*/ }
	for (int i=0; i<sizeof(pins); i++ )
	{
  		pinMode(pins[i], OUTPUT);
	}
	//shiftIni();
	//shiftSend(S_DATA);
}

void setRelaiState(int relai, int state)
{
	/*
	if (state % 2 == 0)
		S_DATA |= (1<<relai);
	else
		S_DATA &= ~(1<<relai);
	shiftSend(S_DATA); 
  	*/
	digitalWrite(pins[relai],( state + 1) % 2);
}

int getRelaiState(int relai)
{
	//return (((S_DATA >> relai) + 1) % 2);
	return (digitalRead(pins[relai]) + 1) % 2;
}
void myWait(void)
{
	usleep(10);
}
void TWIini()
{
	pinMode(SCL, OUTPUT);
	pinMode(SDA, OUTPUT); 
	digitalWrite(SCL, 1);
	digitalWrite(SDA, 1);
}

void TWIsend(unsigned long data)
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
void shiftIni()
{
	pinMode(S_SER, OUTPUT);
	pinMode(S_SRCLK, OUTPUT);
	pinMode(S_RCLK, OUTPUT);
	digitalWrite(S_SER,0);
	digitalWrite(S_SRCLK,0);
	digitalWrite(S_RCLK,0);
}

void shiftSend(unsigned char data)
{
	printf("Hallo%d",data);
	for(int i=7; i>=0; i--)
	{
		digitalWrite(S_SER,(data >> i)%2);
		myWait();
		digitalWrite(S_SRCLK,1);
		myWait();
		digitalWrite(S_SRCLK,0);
		myWait();
	}
	digitalWrite(S_SER,0);
	digitalWrite(S_RCLK,1);
	myWait();
	digitalWrite(S_RCLK,0);
	myWait();
}
