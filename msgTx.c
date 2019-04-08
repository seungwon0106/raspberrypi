
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

//For IPC Function
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


typedef struct
{
	long int msgType; //default 
	char name[20];
	int num;
	int kor;
	int eng;
	int mat;
}MYDATA;


int main(void)
{
	int running = 1;
	MYDATA person[10];
	int msgid;
	int personNum = 0;
	//char buffer[BUFSIZ];

	//step0. ftok();

	//step1. msgget 
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if (msgid == -1)
	{
		fprintf(stderr, "Error:msgget failed : %d\n", errno);
		exit(EXIT_FAILURE);
	}

	while (running)
	{
		printf("Name:");
		fgets(person[personNum].name, sizeof(person[personNum].name), stdin);	//fgets(저장될 주소값, 사이즈, 파일 스트림);

		if (!strncmp(person[personNum].name, "end", 3))
		{
			person[personNum].msgType = 2;
			running = 0;
		}
		else
		{
			printf("Number :");
			scanf("%d", &person[personNum].num);
			while (getchar() != '\n');
			printf("Korean :");
			scanf("%d", &person[personNum].kor);
			while (getchar() != '\n');
			printf("English :");
			scanf("%d", &person[personNum].eng);
			while (getchar() != '\n');
			printf("Math :");
			scanf("%d", &person[personNum].mat);
			person[personNum].msgType = 1;

			//입력 버퍼 비우기
			while (getchar() != '\n');
		}


		//step2. msgsnd
		if (msgsnd(msgid, &person[personNum], sizeof(MYDATA) - sizeof(long), 0) == -1)
		{
			//메시지가 정상적으로 전달되지 않은 경우
			fprintf(stderr, "Error:msgsnd failed : %d\n", errno);
			exit(EXIT_FAILURE);
		}
		personNum++;

	}
	exit(EXIT_SUCCESS);
}