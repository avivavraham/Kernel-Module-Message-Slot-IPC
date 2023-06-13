//
// Created by aviva on 12/12/2022.
//
/*
 * In this assignment I implemmented the ideas shown in:
 * 1) Linux Device Drivers, Third Edition by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman.
 * link to the book: https://lwn.net/Kernel/LDD3/
 * 2) Linux Kernel Module Programming YouTube playlist by SolidusCode
 * link to the playlist: https://www.youtube.com/playlist?list=PL16941B715F5507C5
*/

#include "message_slot.h"
// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h> /* for memory allocation */
#include <linux/errno.h> /* for errno macro */
MODULE_LICENSE("GPL");

/*
 * Hash table (dynamic) implementation taken from-
 * https://github.com/bryanlimy/dynamic-hash-table
 * Hash function implementation taken from-
 * https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
*/
static int hash_reallocation_flag = 0;

unsigned int hash_function(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

/**
 * Initialize hash table
 * @return pointer to hash table, NULL if failed
 */
hash_table *hash_init(int size) {
    hash_table *table;
    int i;
    if (size <= 0) return NULL;

    // allocate hash table

    table = kmalloc(sizeof(hash_table), GFP_KERNEL);
    if (table == NULL) return NULL;

    // allocate list of entries in hash table
    table->entries = kmalloc(sizeof(hash_entry*) * size, GFP_KERNEL);
    if (table->entries == NULL) return NULL;

    // initialize all entries
    for (i = 0; i < size; i ++) table->entries[i] = NULL;

    table->size = size;
    table->num_entries = 0;

    return table;
}


/**
 * Hash function to calculate index in hash table
 */
int hash(hash_table *table, int key) {
    return hash_function(key) % table->size;
}


/**
 * Insert key and value into hash table
 *
 * @return index of the entry that is being inserted, -1 otherwise
 */
int hash_insert(hash_table *table, int key, struct channel* value) {
    int collisions = 0;
    int i = hash(table, key);
    hash_entry *new_entry,*entry;
    if (table == NULL) return -1;
    entry = table->entries[i];

    while (entry) {
        if (entry->key == key) {
            // entry with the same key value already exist
            kfree(entry->value);
            entry->value = kmalloc(sizeof(channel), GFP_KERNEL);
            if (entry->value == NULL) return -1;
            entry->value = value;
            return i;
        }
        collisions += 1;
        entry = entry->next;
    }

    // allocate entry
    new_entry = kmalloc(sizeof(hash_entry), GFP_KERNEL);
    if (new_entry == NULL) return -1;

    // set entry values
    new_entry->key = key;
    new_entry->value = kmalloc(sizeof(channel), GFP_KERNEL);
    entry = new_entry;
    if (entry->value == NULL) return -1;
    entry->value = value;
    entry->next = table->entries[i];

    // insert entry at the beginning of the linked list
    table->entries[i] = entry;
    table->num_entries += 1;
    if (collisions < 17) { // im not allowing more than 16 collisions
        return i;
    } else{
        if(!hash_reallocation_flag){ // were not in the middle of reallocation
            hash_reallocation_flag = 1;
            extend_array(table);
            hash_reallocation_flag = 0;
            return -2; // stands for reallocation.
        }}
    return -1;
}

/**
 * search the value of a specific key
 *
 * @return the value connected to the key or NULL if it doesnt exist.
 */
struct channel* hash_get_value(hash_table *table, int key){
    int i;
    hash_entry *entry;
    if (table == NULL){
        return NULL;
    }
    i = hash(table, key);
    entry = table->entries[i];

    while (entry) {
        if (entry->key == key) {
            // entry with the same key value already exist
            return entry->value;
        }
        entry = entry->next;
    }
    // didn't find key
    return NULL;
}


/**
 * Extend the table size to 2 * previous size
 *
 * @return 1 on success
 */
int extend_array(hash_table* table){
    hash_table* extended_table = hash_init(2 * table ->size); // doubling the array previous size.
    int i;
    for (i = 0; i < table->size; ++i) {
        hash_entry *entry = table->entries[i];
        while (entry){
            hash_insert(extended_table,entry->key, entry->value);
            entry = entry->next;
        }
    }
    table->size *= 2;
    //printk("extending array to %d",table->size);
    table->entries = extended_table->entries;
    return 1;
}

/**
 * Remove entry with key from hash table
 *
 * @return index of entry that is being removed, -1 if failed
 */
int hash_remove(hash_table *table, int key) {
    int i;
    hash_entry *entry,*prev_entry;
    if (table == NULL) return -1;

    i = hash(table, key);
    entry = table->entries[i];
    prev_entry = table->entries[i];

    // find the entry within the linked list
    while (entry) {
        if (entry->key == key) break;
        prev_entry = entry;
        entry = entry->next;
    }
    if (entry == NULL){
        return -1; // key not found
    }
    // update hash table
    if (table->entries[i] == entry){
        table->entries[i] = entry->next;
    }
    else{
        prev_entry->next = entry->next;
    }
    kfree(entry->value);
    kfree(entry);
    return i;
}


/**
 * Destroy and deallocate hash table
 *
 * @return 0 on success, -1 otherwise
 */
int hash_destroy(hash_table *table) {
    int i;
    if (table == NULL) return -1;
    for (i = 0; i < table->size; i++) {
        hash_entry *entry = table->entries[i];

        while (entry) {
            hash_entry *temp = entry->next;
            kfree(entry->value);
            kfree(entry);

            entry = temp;
        }
    }
    return 0;
}











/**
 * given a pointer associated to a certain file, returns the channel for this file or
 * create one if it not exist.
 * @return channel pointer if valid NULL  if encounters error of any kind
 */
struct channel* file_to_channel(struct file_pointer* pointer){
    if(pointer->channel_number == 0){
        return  NULL;
    }

    if(hash_get_value(message_slots_device_files[pointer->minor].channels,
                      pointer->channel_number) == NULL){ // channel hasn't created yet.
        struct channel* channel_of_file;
        channel_of_file = kmalloc(sizeof(channel), GFP_KERNEL); // create channel
        if(channel_of_file == NULL){
            return NULL;
        }
        channel_of_file->channel_number = pointer->channel_number;
        channel_of_file->memory_use = 0;
        hash_insert(message_slots_device_files[pointer->minor].channels,
                    channel_of_file->channel_number,channel_of_file); // insert channel to data
        return channel_of_file;
    } else{ //channel is valid
        return hash_get_value(message_slots_device_files[pointer->minor].channels, pointer->channel_number);
    }
}



static int return_value = 0; // holds the return values of functions

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    if(file->private_data == NULL)
    { // file hasnt been opened yet
    struct file_pointer* pointer;
    pointer = kmalloc(sizeof(file_pointer), GFP_KERNEL);
    if(pointer == NULL){
        return -1;
    }
    pointer->channel_number = 0;
    pointer->minor = iminor(inode);
    file->private_data = (void*) pointer;
    message_slots_device_files[pointer->minor].minor = pointer->minor;
    if (message_slots_device_files[pointer->minor].initialized == 0){
        message_slots_device_files[pointer->minor].channels = hash_init(HASH_INIT_SIZE);
        message_slots_device_files[pointer->minor].initialized = 1;
        }
    }
    return 0;
}


static int device_release( struct inode* inode,
                           struct file*  file)
{
    file_pointer* pointer = (file_pointer*) file->private_data;
    kfree(pointer);
    return 0;
}

static ssize_t device_read( struct file* file, char __user* buffer,
        size_t length, loff_t* offset ){
    int flag = access_ok(buffer, length);
    channel* channel_file;
    if(file->private_data == NULL || flag == 0){
        return -EINVAL;
    }
    channel_file = file_to_channel((file_pointer*)file->private_data);
    if((channel_file == NULL) || (buffer == NULL)){
        return -EINVAL;
    }
    if(channel_file->memory_use == 0){
        return -EWOULDBLOCK;
    }
    //printk("memory use is %d", channel_file->memory_use)
    if(length < channel_file->memory_use){
        return -ENOSPC;
    }
    flag = copy_to_user(buffer,channel_file->buffer,channel_file->memory_use);
    // https://www.kernel.org/doc/htmldocs/kernel-api/API---copy-to-user.html
    if(flag == 0){
        return channel_file->memory_use;
    }
    else{
        return -1;
        }
    return 0;
}


static ssize_t device_write(struct file* file, const char __user* buffer,
        size_t length,loff_t* offset){
    int flag;
    channel* channel_file;
    flag = access_ok(buffer, length);
    if(file->private_data == NULL || flag == 0){
        return -EINVAL;
    }
    channel_file = file_to_channel((file_pointer*)file->private_data);
    if(channel_file == NULL || buffer == NULL){
        return -EINVAL;
    }
    if(length == 0 || length > BUFFER_LENGTH){
        return -EMSGSIZE;
    }
    flag = copy_from_user(channel_file->buffer, buffer, length);
    if(flag != 0){
        channel_file->memory_use = length - flag;
        return -EINVAL;
    }
    channel_file->memory_use = length - flag;
    return (length - flag);
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    if((ioctl_command_id != MSG_SLOT_CHANNEL) || (ioctl_param ==0)){
        return -EINVAL;
    }
    ((file_pointer*)file->private_data) -> channel_number = ioctl_param;
    return 0;
}




// Initialize the module - Register the character device (skeleton copied from recitation 6).
// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
        .owner	  = THIS_MODULE, // Required for correct count of module usage. This prevents the module from being removed while used.
        .read           = device_read,
        .write          = device_write,
        .open           = device_open,
        .release        = device_release,
        .unlocked_ioctl = device_ioctl,
};



static int __init simple_init(void)
{
    return_value = -1;
    // Register driver capabilities.
    return_value = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &Fops );
    if (return_value < 0){
        printk(KERN_ERR "registration failed for %s , on major number %d .", DEVICE_NAME, MAJOR_NUMBER);
    }

    printk( "Registeration is successful. "
            "The major device number is %d.\n", MAJOR_NUMBER);
    printk( "If you want to talk to the device driver,\n" );
    printk( "you have to create a device file:\n" );
    printk( "mknod /dev/%s c %d 0\n", DEVICE_NAME, MAJOR_NUMBER);
    printk( "You can echo/cat to/from the device file.\n" );
    printk( "Dont forget to rm the device file and "
            "rmmod when you're done\n" );

    return return_value;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    int i;
    printk( "Removing mod\n" );
    for (i = 0; i < 256; ++i) {
        if(message_slots_device_files[i].initialized != 0){ //we allocated memory for hash
            hash_destroy(message_slots_device_files[i].channels);
        }
    }
    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);
