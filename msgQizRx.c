#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// For IPC function
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


struct myData
{
	long int msgType; //default 
	char name[20];
	int num;
	int kor;
	int eng;
	int mat;
};


int main(void)
{
	int running = 1;
	struct myData person[10];
	struct msqid_ds msqstat;  //���� �޽���ť�� �޽��� ������ Ȯ���ϱ� ����
	int msgid;
	int personNum = 0;
	int total = 0;
	float ave = 0;

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
		//step2. msgrcv
		if (msgrcv(msgid, &person[personNum], sizeof(person) - sizeof(long), 0, 0) == -1)
		{
			fprintf(stderr, "Error:msgrcv failed : %d\n", errno);
			exit(EXIT_FAILURE);
		}

		if (person[personNum].msgType == 2)
			running = 0;
		else
		{
			//printf("Receive Data Person[%d]\n", personNum);
			total = person[personNum].kor + person[personNum].eng + person[personNum].mat;
			ave = (total / 3);
			printf("Name :%s, total : %d, ave : %f\n", person[personNum].name, total, ave);
		}

		if (msgctl(msgid, IPC_STAT, &msqstat) == -1)
		{
			perror("Fail:msgctl()");
			exit(1);
		}
		printf("remain message count:%d\n", msqstat.msg_qnum);
		if (msqstat.msg_qnum > 0)
			running = 1;
		personNum++;
	}

	//step3. msgctl IPC_RMID
	if (msgctl(msgid, IPC_RMID, 0) == -1)
	{
		fprintf(stderr, "Error:msgctl IPC_RMID failed : %d\n", errno);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}