
MODULE_NAME := message_slot

USER_PROGRAMS := message_sender message_reader

KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

CC := gcc

# Kernel module source files
KERNEL_MODULE_SRCS := message_slot.c

# User-space program source files
SENDER_SRC := message_sender.c
READER_SRC := message_reader.c

# Default target
all: $(MODULE_NAME).ko $(USER_PROGRAMS)

# Kernel module target
obj-m := $(MODULE_NAME).o

$(MODULE_NAME).ko: $(KERNEL_MODULE_SRCS)
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

# User-space program targets
message_sender: $(SENDER_SRC)
	$(CC) -o $@ $<

message_reader: $(READER_SRC)
	$(CC) -o $@ $<

# Clean target
clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
	rm -f $(USER_PROGRAMS) $(MODULE_NAME).mod.o $(MODULE_NAME).mod.c $(MODULE_NAME).ko $(MODULE_NAME).o $(MODULE_NAME).order $(MODULE_NAME).symvers

.PHONY: all clean

