#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>


//Raspberry Pi3 PHYSICAL I/O Peripherals BaseAddr
#define BCM_IO_BASE 0x3F000000

//GPIO_BASEAddr
#define GPIO_BASE  (BCM_IO_BASE + 0x200000)

//GPIO Function Select Register 0 ~ 5
//                    //addr             value
#define GPIO_IN(g)  (*(gpio+((g)/10)) &=~(7<<(((g)%10)*3)))		//g=pin number
#define GPIO_OUT(g) {(*(gpio + ((g) / 10)) &=~(1 << (((g) % 10) * 3)));\
					 (*(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))); }
//GPIO Pin Output Set 0 /Clr 0
//                      addr      value
#define GPIO_SET(g) (*(gpio+7) = (1<<g))
#define GPIO_CLR(g) (*(gpio+10) = (1<<g))

#define GPIO_GET(g) (*(gpio+13) & (1<<g))
#define GPIO_SIZE 0xB4

volatile unsigned int *gpio;	//cash에 올려 바로바로 사용하려면 써야하고, memory에 올리기 위해서는 사용하면 안된다.  
								//이 변수는 언제든지 값이 바뀔 수 있으니까 항상 메모리에 접근하라고 컴파일러에게 알려주는 것
								//코드가 접근한 영역은 cash사용 cpu가 업데이트할떄마다 cash 값을 알고있다. (가상)
								//ARM은 cash가 한번 값을 읽어오면 업데이트를 하지 않는다. 계속 새로운 값을 읽어와야할떄 사용 
int main(int argc, char **argv)
{
	//GPIO Pin number
	int gno;
	int i, mem_fd;

	void *gpio_map;

	//핀번호 입력이 안됬을때
	if (argc < 2)
	{
		printf("Usage : %s GPIO_NO\n", argv[0]);
		return -1;
	}

	gno = atoi(argv[1]);

	//device open /dev/mem
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)	//O_SYNC : 하드웨어까지 write가 될때까지 아무작동하지 않는다. 
	{
		printf("Error : open() /dev/mem\n");
		return -1;
	}

	//				VirtualAddr                                                   //PhysicalAddr
	gpio_map = mmap(NULL, GPIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
	if(gpio_map == MAP_FAILED)
	{
		printf("Error : mmap() : %d \n", (int)gpio_map);
		return -1;
	}	

	gpio = (volatile unsigned int *)gpio_map;

	GPIO_OUT(gno);

	for (i = 0; i < 5; i++)
	{
		GPIO_SET(gno);
		sleep(1);
		GPIO_CLR(gno);
		sleep(1);
	}
	munmap(gpio_map, GPIO_SIZE);

	return 0;

}

