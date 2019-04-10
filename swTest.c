#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>


#define SW1 0	//GPIO17



int main(void)
{
	
	int swFlag = 0;
	int count=0;
	// Init wiringPi
	wiringPiSetup();
	
	//pinAssign
	pinMode(SW1, INPUT);

	while(1)
	{
		//swFlag==0이면 스위치가 눌렸을 때	
		if(digitalRead(SW1)&& !swFlag)
		{
			printf("Switch ON\n");
			swFlag=1;
		}

		else if(! digitalRead(SW1) && swFlag)
		{
			swFlag=0;
		}

		usleep(10000);
		
		

		/*
		if(digitalRead(SW1))
		{
			count ++;
			printf("Switch ON : %d\n", count);
		}
		//usleep(10000);
		*/
	}

}
