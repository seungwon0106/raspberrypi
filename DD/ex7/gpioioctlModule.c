//================================
// Character device driver example 
// GPIO driver
//================================
#include <linux/fs.h>		//open(),read(),write(),close() Ŀ���Լ�
#include <linux/cdev.h>		//register_chrdev_region(), cdev_init()
				//cdev_add(),cdev_del()
#include <linux/module.h>
//#include <linux/io.h>		//ioremap(), iounmap()
#include <linux/uaccess.h>	//copy_from_user(), copy_to_user()
#include <linux/gpio.h>		//request_gpio(), gpio_set_value()
				//gpio_get_value(), gpio_free()
#include <linux/interrupt.h>	//gpio_to_irq(), request_irq(), free_irq()
#include <linux/timer.h>	//init_timer(), add_timer(), del_timer()
#include <linux/signal.h>	//signal�� ���
#include <linux/sched/signal.h>	//siginfo ����ü�� ����ϱ� ����
#include "myioctl.h"

#define GPIO_MAJOR	200
#define GPIO_MINOR	0
#define GPIO_DEVICE	"gpioled"
#define GPIO_LED	18
#define GPIO_SW		17
#define BLK_SIZE	100

//#define DEBUG

struct cdev gpio_cdev;
struct class *class;
struct device *dev;
volatile unsigned int *gpio;
static char msg[BLK_SIZE] = { 0 };
static int switch_irq;
static struct timer_list timer; 	//Ÿ�̸�ó���� ���� ����ü
static struct task_struct *task; 	//�½�ũ�� ���� ����ü
pid_t pid;
char pid_valid;

static int gpio_open(struct inode *, struct file *);
static int gpio_close(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *buff, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
long  gpio_ioctl(struct file *, unsigned int, unsigned long);


long  gpio_ioctl_wr(struct file *fil, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{

	case CMD1:
		printk(KERN_INFO "CMD1:%ld\n", arg);
		break;

	case CMD2:
		printk(KERN_INFO "CMD2:%ld\n", arg);
		break;

	case CMD3:
		printk(KERN_INFO "CMD3:%ld\n", arg);
		break;

	default:
		printk(KERN_INFO "default:%ld\n", arg);
		break;
	}
	return 0;
}

static struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.read = gpio_read,
	.write = gpio_write,
	.unlocked_ioctl = gpio_ioctl_wr,
	.open = gpio_open,
	.release = gpio_close,
};


static void timer_func(unsigned long data)
{
	gpio_set_value(GPIO_LED, data);
	if (data)
		timer.data = 0L;
	else
		timer.data = 1L;
	timer.expires = jiffies + (1 * HZ);
	add_timer(&timer);
}

static irqreturn_t isr_func(int irq, void *data)
{
	//IRQ�߻� & LED�� OFF�϶� 
	static int count;
	static int flag = 0;

	static struct siginfo sinfo;

	if (!flag)
	{
		flag = 1;
		if ((irq == switch_irq) && !gpio_get_value(GPIO_LED))
		{
			gpio_set_value(GPIO_LED, 1);

			//����ġ�� ������ �� �������α׷����� SIGIO�� �����Ѵ�.
			//sinfo����ü�� 0���� �ʱ�ȭ�Ѵ�.
			memset(&sinfo, 0, sizeof(struct siginfo));
			sinfo.si_signo = SIGIO;
			sinfo.si_code = SI_USER;
			if (task != NULL)
			{
				//kill()�� ������ kernel�Լ�
				send_sig_info(SIGIO, &sinfo, task);
			}
			else
			{
				printk(KERN_INFO "Error: USER PID\n");
			}

		}
		else //IRQ�߻� & LED ON�϶�
			gpio_set_value(GPIO_LED, 0);

		printk(KERN_INFO " Called isr_func():%d\n", count);
		count++;
	}
	else
	{
		flag = 0;
	}
	return IRQ_HANDLED;
}


static int gpio_open(struct inode *inod, struct file *fil)
{
	//����� ��� ī��Ʈ�� ���� ��Ų��.
	try_module_get(THIS_MODULE);
	printk(KERN_INFO "GPIO Device opened\n");
	return 0;
}


static int gpio_close(struct inode *inod, struct file *fil)
{
	//����� ��� ī��Ʈ�� ���� ��Ų��.
	module_put(THIS_MODULE);
	printk(KERN_INFO " GPIO Device closed\n");
	return 0;
}

static ssize_t gpio_read(struct file *fil, char *buff, size_t len, loff_t *off)
{
	int count;
	// <linux/gpio.h>���Ͽ� �ִ� gpio_get_value()�� ����
	// gpio�� ���°��� �о�´�. 
	if (gpio_get_value(GPIO_LED))
		msg[0] = '1';
	else
		msg[1] = '0';

	// �� �����Ͱ� Ŀ�ο��� �� ���������� ǥ���Ѵ�.
	strcat(msg, " from kernel");

	//Ŀ�ο����� �ִ� msg���ڿ��� ���������� buff�ּҷ� ���� 
	count = copy_to_user(buff, msg, strlen(msg) + 1);

	printk(KERN_INFO "GPIO Device read:%s\n", msg);

	return count;
}


static ssize_t gpio_write(struct file *fil, const char *buff, size_t len, loff_t *off)
{
	int count;
	char *cmd, *str;
	char *sep = ":";
	char *endptr, *pidstr;

	memset(msg, 0, BLK_SIZE);

	count = copy_from_user(msg, buff, len);
	str = kstrdup(msg, GFP_KERNEL);
	cmd = strsep(&str, sep);
	pidstr = strsep(&str, sep);
	//cmd�� ���ڿ��� �νĽ�Ű�� ���ؼ�
	cmd[1] = '0';
	printk(KERN_INFO "cmd:%s, pid:%s\n", cmd, pidstr);

	if (!strcmp(cmd, "0"))
	{
		del_timer_sync(&timer);
	}
	else
	{
		init_timer(&timer);
		timer.function = timer_func;
		timer.data = 1L;	//timer_func���� �����ϴ� ���ڰ�
		timer.expires = jiffies + (1 * HZ); //1�� �ڿ� Ÿ�̸� ����
		add_timer(&timer);
	}
	if (!strcmp(cmd, "end"))
	{
		pid_valid = 0;
	}
	else
	{
		pid_valid = 1;
	}

	//pidstr���ڿ��� ���ڷ� ��ȯ
	pid = simple_strtol(pidstr, &endptr, 10);
	printk(KERN_INFO "pid=%d\n", pid);

	if (endptr != NULL)
	{
		// pid���� ���� task_struct����ü�� �ּҰ��� Ȯ�� 
		task = pid_task(find_vpid(pid), PIDTYPE_PID);
		if (task == NULL)
		{
			printk(KERN_INFO "Error:Can't find PID from user APP\n");
			return 0;
		}

	}

	gpio_set_value(GPIO_LED, (strcmp(msg, "0")));
	printk(KERN_INFO "GPIO Device write:%s\n", msg);
	return count;
}


static int __init initModule(void)
{
	dev_t devno;
	int err;
	int count;

	printk("Called initModule()\n");

	// 1. ���ڵ���̽� ����̹��� ����Ѵ�.
	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	register_chrdev_region(devno, 1, GPIO_DEVICE);

	// 2. ���� ����̽��� ���� ����ü�� �ʱ�ȭ �Ѵ�.
	cdev_init(&gpio_cdev, &gpio_fops);
	gpio_cdev.owner = THIS_MODULE;
	count = 1;

	// 3. ���ڵ���̽��� �߰�
	err = cdev_add(&gpio_cdev, devno, count);
	if (err < 0)
	{
		printk(KERN_INFO "Error: cdev_add()\n");
		return -1;
	}

	//class�� �����Ѵ�.
	class = class_create(THIS_MODULE, GPIO_DEVICE);
	if (IS_ERR(class))
	{
		err = PTR_ERR(class);
		printk(KERN_INFO "class_create error %d\n", err);

		cdev_del(&gpio_cdev);
		unregister_chrdev_region(devno, 1);
		return err;
	}

	//��带 �ڵ����� ������ش�.
	dev = device_create(class, NULL, devno, NULL, GPIO_DEVICE);
	if (IS_ERR(dev))
	{
		err = PTR_ERR(dev);
		printk(KERN_INFO "device create error %d\n", err);
		class_destroy(class);
		cdev_del(&gpio_cdev);
		unregister_chrdev_region(devno, 1);
		return err;
	}



	//printk(KERN_INFO "'sudo mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
	//printk(KERN_INFO "'sudo chmod 666 /dev/%s'\n", GPIO_DEVICE);

	// ���� GPIO_LED���� ��������� Ȯ���ϰ� ������ ȹ��
	err = gpio_request(GPIO_LED, "LED");
	if (err == -EBUSY)
	{
		printk(KERN_INFO "Error gpio_request LED\n");
		return -1;
	}

	// ���� GPIO_SW���� ��������� Ȯ���ϰ� ������ ȹ��
	err = gpio_request(GPIO_SW, "SW");
	if (err == -EBUSY)
	{
		printk(KERN_INFO "Error gpio_request SW\n");
		return -1;
	}

	gpio_direction_output(GPIO_LED, 0);
	switch_irq = gpio_to_irq(GPIO_SW);
	err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch", NULL);
	if (err)
	{
		printk(KERN_INFO "Error request_irq\n");
		return -1;
	}

	return 0;
}

static void __exit cleanupModule(void)
{
	dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	del_timer_sync(&timer);

	// 0. �����ߴ� ��带 �����Ѵ�.
	device_destroy(class, devno);
	class_destroy(class);

	// 1.���� ����̽��� ����� �����Ѵ�.
	unregister_chrdev_region(devno, 1);

	// 2.���� ����̽��� ����ü�� �����Ѵ�.
	cdev_del(&gpio_cdev);

	gpio_direction_output(GPIO_LED, 0);

	//request_irq���� �޾ƿ� �������� �ݳ��Ѵ�.
	free_irq(switch_irq, NULL);

	//gpio_request()���� �޾ƿ� �������� �ݳ��Ѵ�.
	gpio_free(GPIO_LED);
	gpio_free(GPIO_SW);


	printk("Good-bye!\n");
}


//sudo insmod ȣ��Ǵ� �Լ��� ����
module_init(initModule);

//sudo rmmod ȣ��Ǵ� �Լ��� ����
module_exit(cleanupModule);

//����� ����
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO Driver Module");
MODULE_AUTHOR("Heejin Park");