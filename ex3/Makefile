# Kernel module name
MODULE_NAME := message_slot

# User-space program names
USER_PROGRAMS := message_sender message_reader

# Kernel source directory
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

# Compiler
CC := gcc

# Kernel module source files
KERNEL_MODULE_SRCS := message_slot.c

# User-space program source files
SENDER_SRC := message_sender.c
READER_SRC := message_reader.c

# Targets
all: $(MODULE_NAME).ko $(USER_PROGRAMS)

# Kernel module target
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
	rm -f $(USER_PROGRAMS) $(MODULE_NAME).ko

.PHONY: all clean
