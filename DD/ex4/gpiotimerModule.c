//=========================================
//character device driver example -> string(문자열)을 주고받으면서 모듈 동작
//				  -> character device!!
//GPIO Driver
//+switch기능
//+ timer기능 
//========================================

#include <linux/module.h>
//위 헤더파일은 반드시기본적으로  필요하다
#include <linux/fs.h> // open(), read(), write(), close() 커널함수
#include <linux/cdev.h> //register_chrdev_region(), cdev_inig(), cdev_add(), cdev_del()
//#include <linux/io.h> //ioremap(), iounmap()
#include <linux/uaccess.h> //copy_from_user(), copy_to_user()
#include <linux/gpio.h> //gpio_get_value(), request_gpio(), gpio_set_value(),gpio_free()
#include <linux/interrupt.h>//gpio_to_irq(), request_irq(), free_irq()
#include <linux/timer.h>//init_timer(), add_timer(), del_timer()

#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpioled"
#define GPIO_LED 18
#define GPIO_SW 17
#define BLK_SIZE 100




struct cdev gpio_cdev;//전역변수로 지정했기 때문에 구조체 초기값은 0이다.
volatile unsigned int *gpio; //32bit 구조체 포인터 설정 -> gpio 레지스터 접근에 필요한 포인터!
static char msg[BLK_SIZE]={0}; //문자열 초기화
static int switch_irq;
static struct timer_list timer; //타이머 처리를 위한 구조체, 타이머를 여러개 사용할 수 있으므로 list구조체를 선언

static int gpio_open(struct inode *, struct file *);
static int gpio_close(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *buff, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static struct file_operations gpio_fops={
	.owner=THIS_MODULE,//파일의 소유권을 가진 주체 지정
	.read=gpio_read,
	.write=gpio_write,
	.open=gpio_open,
	.release=gpio_close,
};

// ex) open시 gpio open()함수 호출, gpio_write, gpio_read도 read, write시 호출됨 
//=> MAJOR, MINOR번호를 보고 vfs를 거쳐서 헤당 디바이스에 맞는 함수에 mapping된다.

static void timer_func(unsigned long data)
{
	gpio_set_value(GPIO_LED, data);
	if(data)//timer가 1초마다 동작될때마다 led를 깜빡이게 동작시키고 싶음. 따라서 먼저 timer 1초가 지났을 때 led불이 켜졌을 것이다. -> 그러면 timer.data의 값을 0으로 바꿔주고, 만약 0이 들어오면 1로 바꿔준다.
		timer.data=0L;
	else
		timer.data=1L;
	timer.expires=jiffies+(1*HZ);
	add_timer(&timer);

}
static irqreturn_t isr_func(int irq, void *data) //인터럽트 발생시 handler함수 --> 스위치 on/off가 인터럽트 발생조건임!!여기서는
{
	//IRQ발생 & LED가 OFF일때
	static int count;
	if((irq==switch_irq) && !gpio_get_value(GPIO_LED))
		gpio_set_value(GPIO_LED,1);
	else //IRQ발생, &led on일 때
		gpio_set_value(GPIO_LED,0);
	
	printk(KERN_INFO " Called isr_func():%d\n",count);
	count++;

	return IRQ_HANDLED;
}
		
static int gpio_open(struct inode *inod, struct file *fil)
{
	try_module_get(THIS_MODULE); //현재 장치를 몇개의 프로세스가 사용하고 있는지를 알려주는 함수-> 프로세스가 많아질수록 count수가 증가한다. 
				     //count 값이 0이되어야만 rmmod 명령어 사용할 수 있음 -> 만약 되지 않는다면 reboot실행해야한다..
	printk(KERN_INFO "GPIO Device opened\n");
	return 0;

}
static int gpio_close(struct inode *inod, struct file *fil)
{
	//모듈의 사용 count를 감소시킨다.
	module_put(THIS_MODULE);
	printk(KERN_INFO "GPIO Device closed\n");
	return 0;
}
//읽은 byte개수를 return하는 함수 -> ssize_t : signed size_t!!
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off) // (파일포인터, 버퍼 주소값, 읽을 사이즈, 버퍼 오프셋)
										  // applicant단에 데이타를 user단에 전달해주어야함. 따라서 커널입장에서는 write모드이다! 
{
	int count;
	if(gpio_get_value(GPIO_LED)) //GPIO의 현재 상태값을 gpio_get_value()를 통해 알 수 있다.
		msg[0]='1';
	else 
		msg[1]='0';
	strcat(msg, " from Kernel");
	//usr는 커널에 진입할 수 없다. 따라서 msg를 못접근하기 때문에 커널이 msg를 유저모드에 진입해서 copy하도록 한다ㅏ.
	count=copy_to_user(buff, msg, strlen(msg)+1);

	printk(KERN_INFO "GPIO Device read:%s\n",msg);

	return count;

}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
	int count;
	memset(msg, 0, BLK_SIZE);

	count = copy_from_user(msg,buff, len);

	if(!strcmp(msg,"0"))
	{
		del_timer_sync(&timer); //동기화 방식으로 timer설치 -> 비동기식일 때 커널 뻗는 문제 발생할 수도 있음, 
					// 중간에 ./gpio 0이 되었을 때 rmmod를 하면 디바이스 내에서 timer는아직 계속 동작되고 있을 것이다. 
					// 따라서 timer_func함수의 데이터값들이 미아가 된다...
					// 커널 byebye~~
	}
	else 
	{
		init_timer(&timer);//타이머 setting
		timer.function=timer_func; //타이머가 시간이 만료되면  timer_func안으로 jump함 -> timer_func에서 해당되는 동작을 수행한다. 
		timer.data=1L; //timer_func로 인자를 넘겨줄 수 있음, long type의 1값을 넘겨준다.
		timer.expires = jiffies + (1*HZ); //jiffies : 커널의 현재 시간 (1*HZ) = 1초 , 1초뒤 타이머 만료
		add_timer(&timer);
	}
	gpio_set_value(GPIO_LED, (strcmp(msg, "0")));
	printk(KERN_INFO "GPIO Device write:%s\n", msg);
	return count; 
}

static int __init initModule(void) //insmod
{
	dev_t devno; //int형 32bit크기,devno 상위 8bit에 MAJOR할당하고 하위 16bit에 MINOR를 할당한다.
		     // 상위 8bit 앞에 4bit크기의 공간이 있음 (주의)
	int err;
	//tatic void *map; //initModule에서 함수 종료 여부와 관계없이 무조건 항상 존재한다.
	int count;
	printk("Called initModule()\n");

	//1. 문자디바이스 드라이버를 등록한다.
	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);//MAJOR -> 0~255, MINOR -> 0~65535의 숫자를 가진다. 
					      //ls -al명령어를 통해 /dev 리스트를 확인해보면 MAJOR와 MINOR번호를 알 수 있다.
					      //MAJOR 번호가 같은 디바이스는 MINOR번호로 구별한다. MINOR번호는 디바이스가 각기다른 고유번호를 가지고 있다.
					      //디바이스 접근시 MINOR번호와 MAJOR번호가 필요하다.
	register_chrdev_region(devno,1,GPIO_DEVICE);

	//2. 문자 디바이스를 위한 구조체를 초기화 한다.
	cdev_init(&gpio_cdev, &gpio_fops);
	gpio_cdev.owner=THIS_MODULE;
	count = 1;

	//3. 문자 디바이스를 추가 -> kenel허용 조건에 맞다면 추가될 것이다. ->커널에 모듈이 물리게 됨
	err=cdev_add(&gpio_cdev, devno, count);
	if(err<0) //커널에 디바이스가 추가 되지 않았을 경우
	{
		printk(KERN_INFO "ERROR: cdev_add()\n");
		return -1;
	}
	
	printk(KERN_INFO "'sudo mknod /dev/%s c %d 0'\n",GPIO_DEVICE, GPIO_MAJOR); // mknod /dev/gpio 디바이스 이름 c(캐릭터 디바이스) MAJOR MINOR
									      // FILE SYSTEM의 경우 inode가 생성되어 있지만, 커널단에서 고유 inode번호같은 것이 존재하지 않는다.
									      // 따라서, nod를 생성한다면 어떤 디바이스에 대한 관련 함수에 대한 정보를 담게 되는 것이다.
	printk(KERN_INFO "'sudo chmod 666 /dev/%s'\n",GPIO_DEVICE); //사용자에게 읽고 쓰는 권한을 준다.
	
	//현재 gpio_led핀이 사용중인지 확인하고 사용권환 획득
	err=gpio_request(GPIO_LED, "LED");
	if(err==-EBUSY) //지금 핀이 사용중이면 err => -EBUSY
	{
		printk(KERN_INFO "Error gpio_request LED\n");
		return -1;
	}
	//현재 gpio_sw핀이 사용중인지 확인하고 사용권한 획득
	err=gpio_request(GPIO_SW, "SW");
	if(err==-EBUSY) //지금 핀이 사용중이면 err => -EBUSY
	{
		printk(KERN_INFO "Error gpio_request SW\n");
		return -1;
	}

	//4. 물리 메모리 번지로 인자값을 전달하면 가상메모리 번지를 리턴한다.


#ifdef DEBUG
	printk(KERN_INFO, "devno=%d",devno);
#endif


	gpio_direction_output(GPIO_LED, 0);
	//interrupt
	switch_irq=gpio_to_irq(GPIO_SW);
	err=request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch",NULL); 
	if(err)
	{
		printk(KERN_INFO "Error request_irq\n");
		return -1;
	}





	return 0;
}

static void __exit cleanupModule(void) //rmmod
{
	dev_t devno=MKDEV(GPIO_MAJOR, GPIO_MINOR);
	del_timer_sync(&timer); // 디바이스에서 아직 동작되고 있는 timer가 있다면 종료해준다!!
	
	//GPIO_CLR(GPIO_LED);
	gpio_direction_output(GPIO_LED,0); // 핀 방향성이랑 초기값 설정 해주는 것

	gpio_free(GPIO_LED);
	gpio_free(GPIO_SW);	// GPIO request()에서 받아온 사용권한을 반납한다.
	
	//request_irq에서 받아온 사용 권한을 반납한다.
	free_irq(switch_irq,NULL);
	//1. 문자 디바이스의 등록을 해제한다.
	unregister_chrdev_region(devno,1);

	//2. 문자 디바이스의 구조체를 삭제한다.
	cdev_del(&gpio_cdev);


	
	printk("Good-bye!\n");
	
}




//sudo insmod 호출되는 함수명 정의
module_init(initModule); 
//sudo rmmod 호출되는 함수명 정의
module_exit(cleanupModule);
//모듈의 정보
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO Driver Module");
MODULE_AUTHOR("JA Kim");


