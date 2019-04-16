//================================
// Character device driver example 
// GPIO driver
//================================
#include <linux/fs.h>		//open(),read(),write(),close() Ŀ���Լ�
#include <linux/cdev.h>		//register_chrdev_region(), cdev_init()
				//cdev_add(),cdev_del()
#include <linux/module.h>
#include <linux/uaccess.h>	//copy_from_user(), copy_to_user()
#include <linux/gpio.h>		//request_gpio(), gpio_set_value()
				//gpio_get_value(), gpio_free()
#include <linux/interrupt.h>	//gpio_to_irq(), request_irq(), free_irq()
#include <linux/signal.h>	//signal�� ���
#include <linux/sched/signal.h>	//siginfo ����ü�� ����ϱ� ����
#include <linux/hrtimer.h>	//high-resolution timer
#include <linux/ktime.h>

#include "myswitch.h"

#define MS_TO_NS(x) (x * 1E6L)

struct cdev gpio_cdev;
static char msg[BUF_SIZE] = { 0 };

// switch 2�� irq ��ȣ
static int switch_irq1;
static int switch_irq2;

// switch�� ä�͸��� ���� flag
static int flag1 = 0;
static int flag2 = 0;

// USER���α׷��� ��ȣ ����
static struct task_struct *task; 	//�½�ũ�� ���� ����ü

// �ڵ����� ������(mknod)
struct class *class;				//class ����ü 
struct device *dev;

// switch 2���� ä�͸� ������ ���� Ÿ�̸� ����
static struct hrtimer hr_timer;

enum hrtimer_restart myTimer_callback(struct hrtimer *timer)
{
	if (flag1)
		flag1 = 0;
	else if (flag2)
		flag2 = 0;

	printk(KERN_INFO "myTimer_callback\n");

	return HRTIMER_NORESTART;
}

//switch 2���� ���ͷ�Ʈ �ҽ��� ���
static irqreturn_t isr_func(int irq, void *data)
{
	// hrTimer�� ���� ����
	ktime_t ktime;
	//unsigned long delay_in_ms = 50L;	//50ms
	//MS_TO_NS(delay_in_ms)
	unsigned long expireTime = 70000000L; //50ms unit:ns

	static int count = 0;

	static struct siginfo sinfo;

	// sw1�� ������ ��� 
	if (irq == switch_irq1)
	{
		if (!flag1)
		{
			flag1 = 1;

			//kitme_set(������,����������);
			ktime = ktime_set(0, expireTime);
			//hrtimer_init(Ÿ�̸ӱ���ü �ּҰ�, ����� Ÿ�̸Ӱ�,���ð����μ���)
			hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			hr_timer.function = &myTimer_callback;
			printk(KERN_INFO "startTime\n");
			hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

			//����ġ�� ������ �� �������α׷����� SIGUSR1�� �����Ѵ�.
			//sinfo����ü�� 0���� �ʱ�ȭ�Ѵ�.
			memset(&sinfo, 0, sizeof(struct siginfo));
			sinfo.si_signo = SIGUSR1;
			sinfo.si_code = SI_USER;
			if (task != NULL)
			{
				//kill()�� ������ kernel�Լ�
				send_sig_info(SIGUSR1, &sinfo, task);
			}
			else
			{
				printk(KERN_INFO "Error: USER PID\n");
			}
			count++;
			printk(KERN_INFO " Called isr_func():%d\n", count);
		}
	} //sw2�� ������ ���
	else if (irq == switch_irq2)
	{
		if (!flag2)
		{
			flag2 = 1;

			//kitme_set(������,����������);
			ktime = ktime_set(0, expireTime);
			//hrtimer_init(Ÿ�̸ӱ���ü �ּҰ�, ����� Ÿ�̸Ӱ�,���ð����μ���)
			hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			hr_timer.function = &myTimer_callback;
			printk(KERN_INFO "startTime\n");
			hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

			//����ġ�� ������ �� �������α׷����� SIGUSR1�� �����Ѵ�.
			//sinfo����ü�� 0���� �ʱ�ȭ�Ѵ�.
			memset(&sinfo, 0, sizeof(struct siginfo));
			sinfo.si_signo = SIGUSR2;
			sinfo.si_code = SI_USER;
			if (task != NULL)
			{
				//kill()�� ������ kernel�Լ�
				send_sig_info(SIGUSR2, &sinfo, task);
			}
			else
			{
				printk(KERN_INFO "Error: USER PID\n");
			}
			count++;
			printk(KERN_INFO " Called isr_func():%d\n", count);
		}
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


static ssize_t gpio_write(struct file *fil, const char *buff, size_t len, loff_t *off)
{
	int count;
	char *endptr;
	pid_t pid;

	memset(msg, 0, BUF_SIZE);

	count = copy_from_user(msg, buff, len);

	// msg���ڿ��� ���ڷ� ��ȯ
	pid = simple_strtol(msg, &endptr, 10);
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
	return count;
}

static struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.write = gpio_write,
	.open = gpio_open,
	.release = gpio_close,
};


//================================================
// ��� ������ �ʱ�ȭ ����
// 1.���� ����̽� ����̹� ��� 
//================================================
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

	printk(KERN_INFO "'sudo chmod 666 /dev/%s'\n", GPIO_DEVICE);

	// ���� GPIO_SW1�� ��������� Ȯ���ϰ� ������ ȹ��
	err = gpio_request(GPIO_SW1, "SW1");
	if (err == -EBUSY)
	{
		printk(KERN_INFO "Error gpio_request LED\n");
		return -1;
	}

	switch_irq1 = gpio_to_irq(GPIO_SW1);
	err = request_irq(switch_irq1, isr_func, IRQF_TRIGGER_RISING, "switch1", NULL);
	if (err)
	{
		printk(KERN_INFO "Error request_irq\n");
		return -1;
	}

	// ���� GPIO_SW2�� ��������� Ȯ���ϰ� ������ ȹ��
	err = gpio_request(GPIO_SW2, "SW2");
	if (err == -EBUSY)
	{
		printk(KERN_INFO "Error gpio_request SW\n");
		return -1;
	}

	switch_irq2 = gpio_to_irq(GPIO_SW2);
	err = request_irq(switch_irq2, isr_func, IRQF_TRIGGER_RISING, "switch2", NULL);
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


	// 0. �����ߴ� ��带 �����Ѵ�.
	device_destroy(class, devno);
	class_destroy(class);

	// 1.���� ����̽��� ����� �����Ѵ�.
	unregister_chrdev_region(devno, 1);

	// 2.���� ����̽��� ����ü�� �����Ѵ�.
	cdev_del(&gpio_cdev);

	//request_irq���� �޾ƿ� �������� �ݳ��Ѵ�.
	free_irq(switch_irq1, NULL);
	free_irq(switch_irq2, NULL);

	//gpio_request()���� �޾ƿ� �������� �ݳ��Ѵ�.
	gpio_free(GPIO_SW1);
	gpio_free(GPIO_SW2);

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
