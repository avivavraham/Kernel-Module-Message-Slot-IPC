#define DEVICE_NAME "msg_slot"
#define MAJOR_NUMBER 235
#define SUCCESS 0
#define BUFFER_LENGTH 128
#include <linux/ioctl.h>
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int)
//#define MSG_SLOT_CHANNEL 3
//https://www.kernel.org/doc/html/v5.4/ioctl/ioctl-number.html
#define HASH_INIT_SIZE 4 // the hash table initial size



/**
 * Hash table entry structure
 */
typedef struct hash_entry {
    unsigned int key;
    struct channel* value;
    struct hash_entry *next;
} hash_entry;

/**
 * Hash table structure
 */
typedef struct hash_table {
    int size;
    int num_entries;
    hash_entry **entries;
} hash_table;

int extend_array(hash_table *table);

hash_table *hash_init(int size);

int hash_hash(hash_table *table, int key);

int hash_insert(hash_table *table, int key, struct channel* value);

int hash_remove(hash_table *table, int key);

int hash_destroy(hash_table *table);

struct channel* hash_get_value(hash_table *table, int key);

/**
 * single channel structure
 */
typedef struct channel {
    int channel_number;
    char buffer[BUFFER_LENGTH];
    int memory_use;
} channel;

/**
 * file_pointer structure
 * can be found inside private_data field of files using message slot
 */
typedef struct file_pointer {
    unsigned int channel_number;
    int minor;
} file_pointer;

/**
 * message_slot_file structure
 * minor number == his index in the message_slots_device_files array
 * channels hash table holds all the channels opened for the specific minor
 */
struct message_slot_file{
    int minor;
    int initialized;
    struct hash_table* channels;

} message_slots_device_files[256]; //an array of the different device files indexes by their minor number.
