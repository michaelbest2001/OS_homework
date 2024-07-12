#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE
/*The goal of this assignment is to gain experience with kernel programming and, particularly, 
a better understanding on the design and implementation of inter-process communication (IPC), kernel modules, and drivers.
In this assignment, you will implement a kernel module that provides a new IPC mechanism, called a message slot.
 A message slot is a character device file through which processes communicate. 
 A message slot device has multiple message channels active concurrently, which can be used by multiple processes.
  After opening a message slot device file, a process uses ioctl() to specify the id of the message channel it wants to use. 
  It subsequently uses read()/write() to receive/send messages on the channel. In contrast to pipes,
   a message channel preserves a message until it is overwritten, so the same message can be read multiple times.
*/

// make sure to include the necessary header files
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/errno.h>

#include "message_slot.h"

#define MESSAGE_SLOT
MODULE_LICENSE("GPL");


/*You’ll need a data structure to describe individual message slots (device files with different minor numbers).
 In device_open(), the module can check if it has already created a data structure for the file being opened, and create one if not. 
 You can get the opened file’s minor number using the iminor() kernel function (applied to the struct inode* argument of device_open()).
*/

// The message slot device file will be named /dev/message_slot.<minor>, where <minor> is the minor number of the device file.

//==================== DATA STRUCTURES =============================//

// struct channel: A data structure to hold a message and its length.

typedef struct channel {
    unsigned int id;
    char message[BUF_LEN];
    int msg_len;
    struct channel* next;
    
} channel;

// struct slot: A data structure to hold the minor number and the channels of a message slot.
typedef struct {
    int num_of_files;
    int minor;
    channel* channels;
    channel* cur_channel;

} slot;
// slots: An array of pointers to slot data structures, indexed by the minor number of the message slot device file.
static slot* slots[256];


// Function prototypes
static int device_open(struct inode *inode, struct file *file);
static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param);
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset);
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset);
static int device_release(struct inode *inode, struct file *file);

//==================== OPEN =============================//
static int device_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    // check if the slot has already been created and create one if not
    if (slots[minor] == NULL) {
        slots[minor] = kmalloc(sizeof(slot), GFP_KERNEL);
        if (!slots[minor]) {
            return -ENOMEM;
        }
        slots[minor]->minor = minor;
        slots[minor]->channels = NULL;
        slots[minor]->cur_channel = NULL;
    }
    // set the current channel to NULL and the private data of the file to the slot
    slots[minor]->cur_channel = NULL;
    file->private_data = slots[minor];
    // increment the number of open files
    slots[minor]->num_of_files++; 
    printk(KERN_INFO "Device opened(%d)\n", minor);
    return SUCCESS;
}
//==================== RELEASE =============================//
/*When a message slot file is closed, any memory that was allocated specifically for that file object
should be freed. For this, you can use the release method (of struct file_operations), which
the kernel invokes when a device’s open file gets closed*/
static int device_release(struct inode *inode, struct file *file)
{
    // decrement the number of open files 
    int minor = iminor(inode);
    if (slots[minor]->num_of_files == 0) {
        printk(KERN_ERR "Error: device already closed(%d)\n", minor);
        return -EINVAL;
    }
    slots[minor]->num_of_files--; 
    // free the slot if no files are open
    if (slots[minor] != NULL && (slots[minor]->num_of_files == 0)) {
        kfree(slots[minor]);
        slots[minor] = NULL;
    }
    file->private_data = NULL;
    
    printk(KERN_INFO "Device closed(%d)\n", minor);
    return SUCCESS;
}
//==================== READ =============================//
/*• If no channel has been set on the file descriptor, returns -1 and errno is set to EINVAL.
• If no message exists on the channel, returns -1 and errno is set to EWOULDBLOCK.
• If the provided buffer length is too small to hold the last message written on the channel, returns -1 and errno is set to ENOSPC.
• In any other error case (for example, failing to allocate memory), returns -1 and errno is set appropriately (you are free to choose the exact value).
2*/
static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset){
    channel* ch;
    slot* slot;
    slot =  file->private_data;
    ch = slot->cur_channel;

    if (ch == NULL) {
        return -EINVAL;
    }
    if (ch->msg_len == 0) {
        return -EWOULDBLOCK;
    }
    if (length < ch->msg_len) {
        printk(KERN_ERR "Buffer length is too small\n");
        return -ENOSPC;
    }
    if (copy_to_user(buffer, ch->message, ch->msg_len) != 0) {
        printk(KERN_ERR "Failed to copy message to user\n");
        return -EFAULT;
    }
    // return the number of bytes read from the device
    printk(KERN_INFO "Message read from channel %d\n: %s\n", ch->id, ch->message);
    return ch->msg_len;
}
//==================== WRITE =============================//
/*• If no channel has been set on the file descriptor, returns -1 and errno is set to EINVAL.
• If the passed message length is 0 or more than 128, returns -1 and errno is set to EMSGSIZE.
• In any other error case (for example, failing to allocate memory), returns -1 and errno is set appropriately (you are free to choose the exact value).*/
static ssize_t device_write(struct file *file, const char __user * buffer, size_t length, loff_t * offset){
    channel* ch;
    slot* slot;
    slot = file->private_data;
    ch = slot->cur_channel;

    if (ch == NULL) {
        return -EINVAL;
    }
    if (length == 0 || length > BUF_LEN) {
        printk(KERN_ERR "Message length is invalid\n");
        return -EMSGSIZE;
    }
    if (copy_from_user(ch->message, buffer, length) != 0) {
        printk(KERN_ERR "Failed to copy message from user\n");
        return -EFAULT;
    }
    ch->msg_len = length;
    // return the number of bytes written to the device
    printk(KERN_INFO "Message written to channel %d\n: %s\n", ch->id, ch->message);
    return length;
}
//==================== IOCTL =============================//
/*A message slot supports a single ioctl command, named MSG_SLOT_CHANNEL. 
This command takes a single unsigned int parameter that specifies a non-zero channel id. 
Invoking the ioctl() sets the file descriptor’s channel id. 
Subsequent reads/writes on this file descriptor will receive/send messages on the specified channel.

Error cases:
• If the passed command is not MSG_SLOT_CHANNEL, the ioctl() returns -1 and errno is set to EINVAL.
• If the passed channel id is 0, the ioctl() returns -1 and errno is set to EINVAL.*/

static long device_ioctl(struct file *file, unsigned int ioctl_command, unsigned long ioctl_param){
    slot* slot; 
    channel* ch;
    unsigned int channel_id;

    slot = file->private_data;
    channel_id = ioctl_param;

    if (ioctl_command != MSG_SLOT_CHANNEL || channel_id == 0) {
        return -EINVAL;
    }

    ch = slot->channels;
    while (ch != NULL) {
        if (ch->id == channel_id) {
            slot->cur_channel = ch;
            printk(KERN_INFO "Channel set to %d\n", channel_id);
            return SUCCESS;
        }
        ch = ch->next;
    }
    ch = kmalloc(sizeof(channel), GFP_KERNEL);
    if (!ch) {
        return -ENOMEM;
    }
    ch->id = channel_id;
    ch->msg_len = 0;
    ch->next = slot->channels;
    slot->channels = ch;
    slot->cur_channel = ch;
    return SUCCESS;
}
//==================== DEVICE SETUP =============================//
struct file_operations Fops = {
    .owner          = THIS_MODULE,
    .read           = device_read,
    .write          = device_write,
    .unlocked_ioctl = device_ioctl,
    .open           = device_open,
    .release        = device_release
    
};
//==================== INIT MODULE =============================//
static int __init simple_init(void)
{
    int ret_val;
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
    if (ret_val < 0) {
        printk(KERN_ERR "Registering the device failed with %d\n", ret_val);
        return ret_val;
    }
    printk(KERN_INFO "Registering the device succeeded with %d\n", MAJOR_NUM);
    return 0;
}
//==================== CLEANUP MODULE =============================//
static void __exit simple_cleanup(void)
{
    // free the memory allocated for the slots and channels
    int i;
    for (i = 0; i < 256; i++) {
        if (slots[i] != NULL) {
            channel* ch = slots[i]->channels;
            while (ch != NULL) {
                channel* temp = ch;
                ch = ch->next;
                kfree(temp);
            }
            kfree(slots[i]);
        }
    }

    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk(KERN_INFO "Unregistered the device\n");
}
module_init(simple_init);
module_exit(simple_cleanup);

/*1. message_slot: A kernel module implementing the message slot IPC mechanism.
 2. message_sender: A user space program to send a message.
3. message_reader: A user space program to read a message.
*/

//If module initialization fails, print an error message using printk(KERN_ERR ...).

