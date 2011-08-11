# Comment/uncomment the following line to disable/enable debugging
DEBUG = y
 
# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
     DEBFLAGS = -DDEBUG_FUNC # "-O" is needed to expand inlines
else
     DEBFLAGS = 
endif

EXTRA_CFLAGS += $(DEBFLAGS)
		    

obj-m += mycdev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *~	    
