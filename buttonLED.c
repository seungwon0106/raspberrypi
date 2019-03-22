#include <wiringPi.h>

#define SW1		1 //wPin 1
#define SW2		3 //wPin 0
#define LED		2 //wpin 2

void pinAssign(void)
{
	pinMode(SW1, INPUT);
	pinMode(LED, OUTPUT);
	pinMode(SW2, INPUT);
}

int main()
{
	wiringPiSetup();
	char val1, val2;

	pinAssign();
	
	while (1)
	{
		val1 = digitalRead(SW1);
		val2 = digitalRead(SW2);
		if (val1 && val2)//check if the button is pressed, if yes, turn on the LED
			digitalWrite(LED, LOW);
		else 
			digitalWrite(LED, HIGH);
	}
}