KDIR=/lib/modules/$(shell uname -r)/build 
#커널 디렉토리 경로를 알려줘야함
#모듈은 커널 버전보다 낮거나 같게 만들어주어야 insmod가 동작된다.

obj-m:=gpioioctlModule.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules


clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
