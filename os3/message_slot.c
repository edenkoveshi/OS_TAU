/*
 * message_slot.c
 *
 *  Created on: 21 בדצמ× 2017
 *      Author: Eden
 */

#include "message_slot.h"
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>


MODULE_LICENSE("GPL");

// -------- DEFINING DS ---------
// -------- CHANNEL LIST ---------

typedef struct channel_list_node{
	struct channel_list_node* next;
	int channel_id;
	char message[MAX_LENGTH];
} channel_list_node;

//ADT: Find,Add,Remove

channel_list_node* find_channel(channel_list_node* begin,int lookup_id)
{
	if(!begin) return 0;
	printk("FIND CHANNEL 1\n");
	while(begin!=NULL)
	{
		if(begin->channel_id==lookup_id)
			return begin;//found
		begin=begin->next;
	}
	printk("FIND CHANNEL 2\n");
	return 0;//not found
}

channel_list_node* add_channel(channel_list_node* begin,int channel_to_add_id)
{
	channel_list_node* channel_to_add;
	channel_to_add=kmalloc(sizeof(channel_list_node),GFP_KERNEL);
	if(!channel_to_add) return 0; //kmalloc failure
	channel_to_add->channel_id=channel_to_add_id;
	channel_to_add->next=NULL;
	if(!begin)
	{
		begin=channel_to_add; //create a new channel list
		return channel_to_add;
	}
	while(begin->next!=NULL)
	{
		begin=begin->next;
	}
	begin->next=channel_to_add;
	return channel_to_add;
}

//There's no need in removing a single channel
int remove_all_channels(channel_list_node* begin)
{
	channel_list_node* tmp;
	if(!begin) return SUCCESS;
	while(begin!=NULL)
	{
		tmp=begin->next;
		kfree(begin);
		begin=tmp;
	}
	return SUCCESS;
}



// ---------- DEVICE LIST -------
typedef struct device_list_node{
	struct device_list_node* next;
	int dev_id;
	channel_list_node* channels;
} device_list_node;

device_list_node* find_device(device_list_node* begin,int lookup_id)
{
	if(!begin) return 0;
	printk("HELLO 1\n");
	while(begin!=NULL)
	{
		if(begin->dev_id==lookup_id)
			return begin;//found
		begin=begin->next;
	}
	printk("HELLO 2\n");
	return 0; //not found
}

device_list_node* add_device(device_list_node* begin,int device_to_add_id)
{
	device_list_node* device_to_add;
	device_to_add=kmalloc(sizeof(device_list_node),GFP_KERNEL);
	if(!device_to_add) return 0; //kmalloc failure
	device_to_add->dev_id=device_to_add_id;
	device_to_add->next=NULL;
	device_to_add->channels=NULL;
	if(!begin) return 0;
	while(begin->next)
	{
		begin=begin->next;
	}
	begin->next=device_to_add;
	return device_to_add;
}

int remove_all_devices(device_list_node* begin)
{
	device_list_node* tmp;
	if(!begin) return SUCCESS;
	while(begin)
	{
		tmp=begin->next;
		if(begin->channels!=NULL)
			remove_all_channels(begin->channels); //free all connected channels
		kfree(begin);
		begin=tmp;
	}
	return SUCCESS;
}

// -------- GLOBAL VARIABLES ---------
device_list_node* head; //head of device list

// ------- FILE I/O OPERATIONS -------
static int device_open(struct inode* inode,struct file* file)
{
	int dev_id=iminor(inode);
	device_list_node* current_device;
	current_device=find_device(head,dev_id);
	if(current_device!=0) return SUCCESS; //file is already opened
	printk("find_device success\n");
 	if((current_device=add_device(head,dev_id))==0) return -ENOMEM; //this fails only on kmalloc failure
	printk("add_device success\n");
	printk("device_open success\n");
 	return SUCCESS;
}

static int device_release(struct inode* inode,struct file* file)
{
	printk("device_release success\n");
	return SUCCESS;
}

static long device_ioctl(struct file *file,unsigned int ioctl_num,unsigned long ioctl_param)
{
	unsigned long chan_id=(unsigned int) ioctl_param;
	device_list_node* current_device;
	printk("HERE 1\n");
	if(ioctl_num!=MSG_SLOT_CHANNEL) return -EINVAL;//invalid command
	printk("HERE 2\n");
	if((current_device=find_device(head,iminor(file->f_inode)))==0) return -EINVAL; //find current device
	printk("ioctl current device id:%d\n",current_device->dev_id);
	printk("HERE 3\n");
	printk("find_device success\n");
	if(current_device->channels==NULL)
		current_device->channels=kmalloc(sizeof(channel_list_node),GFP_KERNEL);
	if(add_channel(current_device->channels,chan_id)==0) return -ENOMEM; //add channel on current device
	printk("add_chanel success\n");
	printk("channel id:%d\n",current_device->channels->channel_id);
	file->private_data=(void*) chan_id;
	printk("ioctl success\n");
	return SUCCESS;
}

static int device_read(struct file* file,char __user* buffer,size_t length,loff_t* offset)
{
	device_list_node* current_device;
	channel_list_node* current_channel;
	int message_length=0;
	int i;
	//find current device & channel
	if((current_device=find_device(head,iminor(file->f_inode)))==0) return -EINVAL;
	printk("find_device success\n");
	if((current_channel=find_channel(current_device->channels,(int) file->private_data))==0) return -EINVAL;
	printk("find_channel success\n");
	if(!current_channel->message) return -EWOULDBLOCK; //no message on channel
	while(current_channel->message[message_length]!='\0') //find message length
	{
		message_length++;
	}
	if(length<message_length) return -ENOSPC; //provided buffer is too small
	//everything is now ready for message reading
	for(i=0;i<length;i++)
	{
		if(put_user(current_channel->message[i],&buffer[i])!=0) //read. abort and return empty string
			//if reading failed
		{
			i=0;
			while(i<length)
			{
				put_user(0,&buffer[i]);
				i++;
			}
			return -EFAULT;
		}
	}
	printk("device_read success\n");
	return message_length;
}

static int device_write(struct file* file,const char __user* buffer,size_t length,loff_t* offset)
{
	device_list_node* current_device;
	channel_list_node* current_channel;
	int i;
	char* msg;
	//find current device & channel
	if((current_device=find_device(head,iminor(file->f_inode)))==0) return -EINVAL;
	printk("find_device sucess\n");
	printk("current device id:%d\n",current_device->dev_id);
	if((current_channel=find_channel(current_device->channels,(int) file->private_data))==0) return -EINVAL;
	printk("find_channel success\n");
	if(length>MAX_LENGTH) return -EINVAL; //invalid message length
	msg=current_channel->message;
	for(i=0;i<length;i++)
	{
		if(get_user(current_channel->message[i],&buffer[i])!=0) //write. abort and re-write previous string
			//if writing failed
		{
			strcpy(current_channel->message,msg);
			return -EFAULT;
		}
	}
	printk("device_write success\n");
	return length;
}


// -------- MODULE CONFIGURATION - INIT AND EXIT -----------
struct file_operations Fops =
{
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .release        = device_release,
  .unlocked_ioctl=	device_ioctl,
};

static int __init device_init(void){
	int major;
	head=kmalloc(sizeof(device_list_node),GFP_KERNEL);
	if(!head) return -ENOMEM;
	head->next=NULL;
	head->dev_id=0;
	head->channels=NULL;
	major=register_chrdev(MAJOR_NUM,DEVICE_RANGE_NAME,&Fops);
	if(major<0) //register failed
	{
		printk(KERN_ALERT "%s registraion failed for  %d\n",DEVICE_RANGE_NAME,MAJOR_NUM);
		return major;
	}
	printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
	return SUCCESS;
}

static void __exit device_exit(void){
	remove_all_devices(head);
	//all resources free'd
	unregister_chrdev(MAJOR_NUM,DEVICE_RANGE_NAME);
}

module_init(device_init);
module_exit(device_exit);

