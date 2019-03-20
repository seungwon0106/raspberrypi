#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>

//File TO Key
//key _t ftok(const char *pathname, int proj_id)
int main(int argc, char **argv)
{
	key_t msgKey;
	
	msgKey = ftok("/home/pi/raspberrypiEx",'A');	//여러개 키값을 받을때 8비트 문자로 구별
	printf("ftok_key = %d\n", msgKey);


	return 0;


}	
