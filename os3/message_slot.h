/*
 * message_slot.h
 *
 *  Created on: 15 בדצמ× 2017
 *      Author: Eden
 */

#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#define MAX_LENGTH 128
#define SUCCESS 0
#define MAJOR_NUM 244
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "char_dev"

#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>

struct channel_list_node;
struct device_list_node;
struct channel_list_node* find_channel(struct channel_list_node* begin,int lookup_id);
int add_channel(struct channel_list_node* begin,int channel_to_add_id);
int remove_all_channels(struct channel_list_node* begin);
struct device_list_node* find_device(struct device_list_node* begin,int lookup_id);
int add_device(struct device_list_node* begin,int device_to_add_id);
int remove_all_devices(struct device_list_node* begin);

static int device_open(struct inode* inode,struct file* file);
static int device_release(struct inode* inode,struct file* file);
static long device_ioctl(struct file *file,unsigned int ioctl_num,unsigned long ioctl_param);
static int device_read(struct file* file,char __user* buffer,size_t length,loff_t* offset);
static int device_write(struct file* file,const char __user* buffer,size_t length,loff_t* offset);
static int __init device_init(void);
static void __exit device_exit(void);



#endif /* MESSAGE_SLOT_H_ */
