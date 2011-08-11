# Makefile for simple network driver.
#
# Comment/uncomment the following line to disable/enable debugging
DEBUG = y
 
# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
     DEBFLAGS = -DDEBUG_FUNC -DHAVE_SET_MAC # "-O" is needed to expand inlines
else
     DEBFLAGS = 
endif

EXTRA_CFLAGS += $(DEBFLAGS)
		    

obj-m += myeth_dev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *~	    
