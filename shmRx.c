#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//For IPC
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHMSIZE 100	//int * 100

int main(void)
{
	void *shared_Mem = (void*)0; //데이터 타입 x , 주소가 중요, 
								//NULL값을 넣음(초기값을 안주면 쓰레기값이 들어가기때문에 명시적으로 메모리번지 할당되지 않앗다고 넣어줌 ->잘못된 포인터 접근 방지).
	int shmid;
	int *shmaddr;
	int rdData;
	int i;
	
	//step0. ftok()

	//step1. shmget
	shmid = shmget((key_t)1234, sizeof(int)*SHMSIZE, 0666 | IPC_CREAT);	//sizeof 할당 받고자하는 공유메모리 사이즈
	if (shmid == -1)
	{
		fprintf(stderr, "shmget failed");
		exit(EXIT_FAILURE);
	}

	//step2. shmat
	shared_Mem = shmat(shmid, (void*)0, 0);	//(void*)0 : 커널에서 할당하는 메모리로 사용. 요청하는 번지수 x
											//malloc이지만 공유할수 있는 메모리. 
	if (shared_Mem == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}

	printf("Memory attached at %X\n", (int)shared_Mem);
	shmaddr = (int*)shared_Mem;

	//step3. memory access
	for (i = 0; i < SHMSIZE ; i++)
	{
		rdData = *(shmaddr + i);
		printf("shaddr : %X, data : %d \n", shmaddr + i, rdData);
	}
	//sleep(4);

	//step4. shmdt
	if(shmdt(shared_Mem)==-1)
	{
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

	//step5. shmctl	: IPC_RMID
	if(shmctl(shmid, IPC_RMID, 0) == -1)
	{
		fprintf(stderr, "shmctl (IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}