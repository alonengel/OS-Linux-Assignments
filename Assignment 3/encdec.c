#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

int encdec_open(struct inode *inode, struct file *filp);

int encdec_release(struct inode *inode, struct file *filp);

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size,
"i");

int major = 0;

// Data Buffer pointers to act as a block for each method
char *bufferCaesar;
char *bufferXor;

struct file_operations fops_caesar = {
        .open     =    encdec_open,
        .release =    encdec_release,
        .read     =    encdec_read_caesar,
        .write     =    encdec_write_caesar,
        .llseek  =    NULL,
        .ioctl     =    encdec_ioctl,
        .owner     =    THIS_MODULE
};

struct file_operations fops_xor = {
        .open     =    encdec_open,
        .release =    encdec_release,
        .read     =    encdec_read_xor,
        .write     =    encdec_write_xor,
        .llseek  =    NULL,
        .ioctl     =    encdec_ioctl,
        .owner     =    THIS_MODULE
};

// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct {
    unsigned char key;
    int read_state;
} encdec_private_date;


int init_module(void) {
    major = register_chrdev(major, MODULE_NAME, &fops_caesar);
    if (major < 0) {
        return major;
    }

    //Allocating the memory for the blocks.
    bufferCaesar = kmalloc(memory_size, GFP_KERNEL);
    bufferXor = kmalloc(memory_size, GFP_KERNEL);

    //One of the memory allocation failed. (the kmalloc's)
    if (!bufferCaesar || !bufferXor) {
        unregister_chrdev(major, MODULE_NAME);

        //Checking which one we were actually able to create if so, and then we free it using kfree.
        if (bufferCaesar) {
            kfree(bufferCaesar);
        }
        if (bufferXor) {
            kfree(bufferXor);
        }
        //Out of memory error (-12)
        return -ENOMEM;
    }

    return 0;
}

void cleanup_module(void) {
    //Using the same code from before (from init_module).
    unregister_chrdev(major, MODULE_NAME);

    //Even tho wwe know both were created if we got here, its always good measure to check.
    if (bufferCaesar) {
        kfree(bufferCaesar);
    }
    if (bufferXor) {
        kfree(bufferXor);
    }

}

int encdec_open(struct inode *inode, struct file *filp) {
    int minor = MINOR(inode->i_rdev);


    if (minor == 0 || minor == 1) {
        if (minor == 0) { //Caesar Cipher (minor is 0).
            filp->f_op = &fops_caesar;
        } else { //XOR Cipher (minor is 1).
            filp->f_op = &fops_xor;
        }
    } else {
        return -EINVAL; //Invalid argument (-22) - neither of our minors.
    }


    //now we allocate the private data using kmalloc
    encdec_private_date *privateData = kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
    if (!privateData) { //Failed kmalloc memory allocation.
        //Out of memory error (-12)
        return -ENOMEM;
    }

    //We succeeded in creating the private data
    //Setting the default values
    privateData->key = 0;
//    privateData->read_state = ENCDEC_READ_STATE_RAW;
    privateData->read_state = ENCDEC_READ_STATE_DECRYPT; //Default value - (1) same as we saw in the video, constant from the file encdec.h.
    filp->private_data = privateData;

    return 0;
}

int encdec_release(struct inode *inode, struct file *filp) {
    //Retrieving the private_data
    encdec_private_date *privateData = filp->private_data;
    //freeing it.
    kfree(privateData);

    return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

    encdec_private_date *privateData = filp->private_data;

    // We got 3 options for the CMD:
    // ENCDEC_CMD_CHANGE_KEY (0) - new encryption key for our private data.
    // ENCDEC_CMD_SET_READ_STATE (1) - new read state for our private data.
    // ENCDEC_CMD_ZERO (2) - reset the data block buffer.

    if (cmd == ENCDEC_CMD_CHANGE_KEY) {
        //New encryption key
        privateData->key = (unsigned char) arg;

    } else if (cmd == ENCDEC_CMD_SET_READ_STATE) {
        //New read state
        privateData->read_state = (int) arg;
    } else if (cmd == ENCDEC_CMD_ZERO) {
        //We need to check which Cipher we are using, to "reset" its data buffer block .
        if (filp->f_op == &fops_caesar) {
            //Caesar Cipher
            memset(bufferCaesar, 0, memory_size);
        } else {
            //XOR Cipher
            memset(bufferXor, 0, memory_size);
        }
    } else {
        return -ENOTTY;
    }

    return 0;
}

// Adding implementations for:
// ------------------------
// 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
// 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);


ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    if (*f_pos >= memory_size) {
        return -EINVAL;
    }

    encdec_private_date *privateData = filp->private_data;
    int result = copy_to_user(buf, bufferCaesar + (*f_pos), count);

    if (privateData->read_state == ENCDEC_READ_STATE_DECRYPT) {
        //Decrypt mode - decrypt using the example loop.
        size_t i;
        for (i = 0; i < count; i++) {
            buf[i] = ((buf[i] - privateData->key) + 128) % 128;
        }
    }

    //Update the new position to be the next "data buffer" minus the not successful read blocks.
    *f_pos = *f_pos + count - result; //*f_pos += count - result;
    return count - result; //the amount of bytes we were able to read.
}


ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    if (*f_pos >= memory_size) {
        return -ENOSPC;
    }

    encdec_private_date *privateData = filp->private_data;

    //Copy from kernel mode to user mode (to buf), in case some blocks are not successful, we get value different from 0.
    int result = copy_from_user(bufferCaesar + (*f_pos), buf, count);

//    if (result != 0) {
//        return -ENOSPC;
//    }
    size_t i;
    for (i = 0; i < count - result; i++) {
        bufferCaesar[i + (*f_pos)] = (bufferCaesar[i + (*f_pos)] + privateData->key) % 128;
    }

    //Update the new position to be the next "data buffer" minus the not successful read blocks.
    *f_pos = *f_pos + count - result;
    return (count - result); //the amount of bytes we were able to read.


}

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    if (*f_pos >= memory_size) {
        return -EINVAL;
    }

    encdec_private_date *privateData = filp->private_data;
    int result = copy_to_user(buf, bufferXor + (*f_pos), count);

    if (privateData->read_state == ENCDEC_READ_STATE_DECRYPT) {
        //Decrypt mode - decrypt using the example loop.
        size_t i;
        for ( i= 0; i < count; i++) {
            buf[i] = (buf[i] ^ privateData->key);
        }
    }

    //Update the new position to be the next "data buffer" minus the not successful read blocks.
    *f_pos = *f_pos + count - result; //*f_pos += count - result;

    return count - result; //the amount of bytes we were able to read.
}


ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    if (*f_pos >= memory_size) {
        return -ENOSPC;
    }

    encdec_private_date *privateData = filp->private_data;

    //Copy from kernel mode to user mode (to buf), in case some blocks are not successful, we get value different from 0.
    int result = copy_from_user(bufferXor + (*f_pos), buf, count);

//    if (result != 0) {
//        return -ENOSPC;
//    }
    size_t i;
    for (i = 0; i < count - result; i++) {
        bufferXor[i + (*f_pos)] = (bufferXor[i + (*f_pos)] ^ privateData->key);
    }

    //Update the new position to be the next "data buffer" minus the not successful read blocks.
    *f_pos = *f_pos + count - result;
    return (count - result); //the amount of bytes we were able to read.
}

