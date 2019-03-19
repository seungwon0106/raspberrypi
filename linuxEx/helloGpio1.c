#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <unistd.h>

#define loopCount 10
#define delayMax 1024	//500ms



// ./helloGpio [PinNumber]
int main (int argc, char ** argv)
{

	int gpioNo;
	int i;
	int delayTime;
	//STEP 1. WiringPi Init
	wiringPiSetup();

	// pin number Error
	if(argc<2)
	{
		printf("Usage : %s gpioNo\n", argv[0]);
		return -1;
	}	
	

	gpioNo = atoi(argv[1]);
	//STEP 2. Pin direction setup
	pinMode(gpioNo, OUTPUT);

	for(i=0; i<loopCount; i++)
	{
		for(delayTime=0;delayTime<delayMax; delayTime++)
		{			
			//SETP 3. Pin Write
			digitalWrite(gpioNo, HIGH);
			usleep(delayTime);

			digitalWrite(gpioNo, LOW);
			usleep(delayMax-delayTime);
		}

		for(delayTime=0;delayTime<delayMax; delayTime++)
		{			
			//SETP 3. Pin Write
			digitalWrite(gpioNo, LOW);
			usleep(delayTime);

			digitalWrite(gpioNo, HIGH);
			usleep(delayMax-delayTime);
		}

	}

	return 0;
	
}	
