//=========================================
//character device driver example -> string(���ڿ�)�� �ְ�����鼭 ��� ����
//				  -> character device!!
//GPIO Driver
//+switch��� �߰� 
//========================================

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
//�� ��������� �ݵ�ñ⺻������  �ʿ��ϴ�
#include <linux/fs.h>
#include <linux/cdev.h> //char����̽� ���� �������
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/gpio.h> //gpio_get_value()��� ����
#include <linux/interrupt.h>
#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpio_swled"
#define GPIO_LED 18
#define GPIO_SW 17
#define BLK_SIZE 100




struct cdev gpio_cdev;//���������� �����߱� ������ ����ü �ʱⰪ�� 0�̴�.
volatile unsigned int *gpio; //32bit ����ü ������ ���� -> gpio �������� ���ٿ� �ʿ��� ������!
static char msg[BLK_SIZE]={0}; //���ڿ� �ʱ�ȭ
static int switch_irq;

static int gpio_open(struct inode *, struct file *);
static int gpio_close(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *buff, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static struct file_operations gpio_fops={
	.owner=THIS_MODULE,//������ �������� ���� ��ü ����
	.read=gpio_read,
	.write=gpio_write,
	.open=gpio_open,
	.release=gpio_close,
}; // ex) open�� gpio open()�Լ� ȣ��, gpio_write, gpio_read�� read, write�� ȣ��� 
   //=> MAJOR, MINOR��ȣ�� ���� vfs�� ���ļ� ��� ����̽��� �´� �Լ��� mapping�ȴ�.
static irqreturn_t isr_func(int irq, void *data) //���ͷ�Ʈ �߻��� handler�Լ� --> ����ġ on/off�� ���ͷ�Ʈ �߻�������!!���⼭��
{
	//IRQ�߻� & LED�� OFF�϶�
	static int count;
	if((irq==switch_irq) && !gpio_get_value(GPIO_LED))
		gpio_set_value(GPIO_LED,1);
	else //IRQ�߻�, &led on�� ��
		gpio_set_value(GPIO_LED,0);
	
	printk(KERN_INFO " Called isr_func():%d\n",count);
	count++;

	return IRQ_HANDLED;
}
		
static int gpio_open(struct inode *inod, struct file *fil)
{
	try_module_get(THIS_MODULE); //���� ��ġ�� ��� ���μ����� ����ϰ� �ִ����� �˷��ִ� �Լ�-> ���μ����� ���������� count���� �����Ѵ�. 
				     //count ���� 0�̵Ǿ�߸� rmmod ��ɾ� ����� �� ���� -> ���� ���� �ʴ´ٸ� reboot�����ؾ��Ѵ�..
	printk(KERN_INFO "GPIO Device opened\n");
	return 0;

}
static int gpio_close(struct inode *inod, struct file *fil)
{
	//����� ��� count�� ���ҽ�Ų��.
	module_put(THIS_MODULE);
	printk(KERN_INFO "GPIO Device closed\n");
	return 0;
}
//���� byte������ return�ϴ� �Լ� -> ssize_t : signed size_t!!
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off) // (����������, ���� �ּҰ�, ���� ������, ���� ������)
										  // applicant�ܿ� ����Ÿ�� user�ܿ� �������־����. ���� Ŀ�����忡���� write����̴�! 
{
	int count;
	if(gpio_get_value(GPIO_LED)) //GPIO�� ���� ���°��� gpio_get_value()�� ���� �� �� �ִ�.
		msg[0]='1';
	else 
		msg[1]='0';
	strcat(msg, " from Kernel");
	//usr�� Ŀ�ο� ������ �� ����. ���� msg�� �������ϱ� ������ Ŀ���� msg�� ������忡 �����ؼ� copy�ϵ��� �Ѵ٤�.
	count=copy_to_user(buff, msg, strlen(msg)+1);

	printk(KERN_INFO "GPIO Device read:%s\n",msg);

	return count;

}
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
	int count;
	memset(msg, 0, BLK_SIZE);

	count = copy_from_user(msg,buff, len);
	gpio_set_value(GPIO_LED, (strcmp(msg, "0")));
	printk(KERN_INFO "GPIO Device write:%s\n", msg);
	return count; 
}
static int __init initModule(void) //insmod
{
	dev_t devno; //int�� 32bitũ��,devno ���� 8bit�� MAJOR�Ҵ��ϰ� ���� 16bit�� MINOR�� �Ҵ��Ѵ�.
		     // ���� 8bit �տ� 4bitũ���� ������ ���� (����)
	int err;
	//tatic void *map; //initModule���� �Լ� ���� ���ο� ������� ������ �׻� �����Ѵ�.
	int count;
	printk("Called initModule()\n");

	//1. ���ڵ���̽� ����̹��� ����Ѵ�.
	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);//MAJOR -> 0~255, MINOR -> 0~65535�� ���ڸ� ������. 
					      //ls -al��ɾ ���� /dev ����Ʈ�� Ȯ���غ��� MAJOR�� MINOR��ȣ�� �� �� �ִ�.
					      //MAJOR ��ȣ�� ���� ����̽��� MINOR��ȣ�� �����Ѵ�. MINOR��ȣ�� ����̽��� ����ٸ� ������ȣ�� ������ �ִ�.
					      //����̽� ���ٽ� MINOR��ȣ�� MAJOR��ȣ�� �ʿ��ϴ�.
	register_chrdev_region(devno,1,GPIO_DEVICE);

	//2. ���� ����̽��� ���� ����ü�� �ʱ�ȭ �Ѵ�.
	cdev_init(&gpio_cdev, &gpio_fops);
	gpio_cdev.owner=THIS_MODULE;
	count = 1;

	//3. ���� ����̽��� �߰� -> kenel��� ���ǿ� �´ٸ� �߰��� ���̴�. ->Ŀ�ο� ����� ������ ��
	err=cdev_add(&gpio_cdev, devno, count);
	if(err<0) //Ŀ�ο� ����̽��� �߰� ���� �ʾ��� ���
	{
		printk(KERN_INFO "ERROR: cdev_add()\n");
		return -1;
	}
	
	printk(KERN_INFO "'sudo mknod /dev/%s c %d 0'\n",GPIO_DEVICE, GPIO_MAJOR); // mknod /dev/gpio ����̽� �̸� c(ĳ���� ����̽�) MAJOR MINOR
									      // FILE SYSTEM�� ��� inode�� �����Ǿ� ������, Ŀ�δܿ��� ���� inode��ȣ���� ���� �������� �ʴ´�.
									      // ����, nod�� �����Ѵٸ� � ����̽��� ���� ���� �Լ��� ���� ������ ��� �Ǵ� ���̴�.
	printk(KERN_INFO "'sudo chmod 666 /dev/%s'\n",GPIO_DEVICE); //����ڿ��� �а� ���� ������ �ش�.
	
	//���� gpio_led���� ��������� Ȯ���ϰ� ����ȯ ȹ��
	err=gpio_request(GPIO_LED, "LED");
	if(err==-EBUSY) //���� ���� ������̸� err => -EBUSY
	{
		printk(KERN_INFO "Error gpio_request LED\n");
		return -1;
	}
	//���� gpio_sw���� ��������� Ȯ���ϰ� ������ ȹ��
	err=gpio_request(GPIO_SW, "SW");
	if(err==-EBUSY) //���� ���� ������̸� err => -EBUSY
	{
		printk(KERN_INFO "Error gpio_request SW\n");
		return -1;
	}

	//4. ���� �޸� ������ ���ڰ��� �����ϸ� ����޸� ������ �����Ѵ�.
	/*map=ioremap(GPIO_BASE, GPIO_SIZE); //map -> void Ÿ�� ������
	if(!map)
	{
		printk(KERN_INFO "Error:mapping GPIO memory\n");
		iounmap(map);
		return -EBUSY;
	}*/


#ifdef DEBUG
	printk(KERN_INFO, "devno=%d",devno);
#endif

	//gpio=(volatile unsigned int*)map; //������ Ÿ�� ����ȯ ->gpio register�� �����ϱ� ���ؼ� �׿� �´� Ÿ������ ����ȯ���־�� ��.
	gpio_direction_output(GPIO_LED, 0);
	//interrupt
	switch_irq=gpio_to_irq(GPIO_SW);
	err=request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch",NULL); 
	if(err)
	{
		printk(KERN_INFO "Error request_irq\n");
		return -1;
	}

	//gpio_direction_input(GPIO_SW);
	//GPIO_OUT(GPIO_LED);
	//GPIO_SET(GPIO_LED); 

	return 0;
}

static void __exit cleanupModule(void) //rmmod
{
	dev_t devno=MKDEV(GPIO_MAJOR, GPIO_MINOR);
	
	//GPIO_CLR(GPIO_LED);
	gpio_direction_output(GPIO_LED,0); // �� ���⼺�̶� �ʱⰪ ���� ���ִ� ��

	gpio_free(GPIO_LED);
	gpio_free(GPIO_SW);	// GPIO request()���� �޾ƿ� �������� �ݳ��Ѵ�.
	
	//request_irq���� �޾ƿ� ��� ������ �ݳ��Ѵ�.
	free_irq(switch_irq,NULL);
	//1. ���� ����̽��� ����� �����Ѵ�.
	unregister_chrdev_region(devno,1);

	//2. ���� ����̽��� ����ü�� �����Ѵ�.
	cdev_del(&gpio_cdev);
	//if(gpio)
	    //iounmap(gpio); //gpio���� �ݳ����� �ʾ��� �� �ݳ��� �ϵ��� ���� 
	
	printk("Good-bye!\n");
	
}




//sudo insmod ȣ��Ǵ� �Լ��� ����
module_init(initModule); 
//sudo rmmod ȣ��Ǵ� �Լ��� ����
module_exit(cleanupModule);
//����� ����
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO Driver Module");
MODULE_AUTHOR("JA Kim");


