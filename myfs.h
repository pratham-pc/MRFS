#ifndef MYFS_H
#define MYFS_H
 
 #include <semaphore.h>
 #include <fcntl.h> 
 #include <stdio.h>
 #include <sys/ipc.h>
 #include <sys/shm.h>
 #include <sys/wait.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 
extern int current_inode_num;
extern char *filesystem;
extern int shmid;
extern int shmid_sem;
extern int key;
extern int key_sem;
// extern sem_t *mutex;
struct table_entry {
	short int in;
	int byte_offset;
	char mode;
};
#define TE_INIT {-1, 0, 'r'}
typedef struct table_entry table_entry;

extern table_entry te[10];
// 8276 bytes
struct super_block { 
	unsigned int size;
	unsigned int max_data_blocks;
	unsigned int used_data_blocks;
	unsigned int free_data_block_list[2048]; // stores bitmap info for 2048 * 32 data blocks = 1024 * 32 * 256 bytes = 16 MB of data 
	unsigned int max_inodes;
	unsigned int used_inodes;
	unsigned int free_inode_list[16]; // ensures information of 16 * 32 files = 512 files
};
#define SB_INIT { 0, 0, 0, {0}, 0, 0, {0} }

// 80 bytes
struct inode {
	unsigned int type;
	unsigned int size;
	time_t time_last_read;
	time_t time_last_modified;
	time_t time_inode_modified;
	mode_t access_permissions;
	unsigned int pointers_to_data_block[10];
};
#define INODE_INIT { 0, 0, 0, 0, 0, 0, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1} }

struct file_entry {
	char file_name[30];
	short unsigned int inode_number;
};

typedef struct file_entry file_entry;
typedef struct inode inode;
typedef struct super_block super_block;

/*
 * A memory block of the specified size will be dynamically allocated,
 * and the data structure for the file system will be initialized in
 * that space.
 */
int create_myfs (unsigned int size);

/* Copy a file from the PC file system to the memory resident file system */
int copy_pc2myfs (char *source, char *dest);

/* Copy a file from the memory resident file system to the PC file system */
int copy_myfs2pc (char *source, char *dest);

/* Remove a specified file from the current working directory */
int rm_myfs (char *filename);

/* Display the contents of a (text) file */
int showfile_myfs (char *filename);

/* Display the list of files in the current working directory */
int ls_myfs ();

/* Create a new directory in the current working directory */
int mkdir_myfs (char *dirname);

/* Change working directory */
int chdir_myfs (char *dirname);

/* Remove a specified directory */
int rmdir_myfs (char *dirname);


/* Open a file for reading or writing */
int open_myfs (char *filename, char mode);
/* Close a file */
int close_myfs (int fd);

int attach_myfs();
int detach_myfs();
/* Read a block of bytes from a file */
int read_myfs (int fd, int nbytes, char *buff);

/* Write a block of bytes into a file */
int write_myfs (int fd, int nbytes, char *buff);

/* Check for end of file */
int eof_myfs (int fd);

/* Dump the memory resident file system into a file on the secondary memory */
int dump_myfs (char *dumpfile);

/*
 * Load the memory resident file system from a previously dumped version
 * from secondary memory
 */
int restore_myfs (char *dumpfile);
int destroy_myfs ();

/*
 * Display the status of the file system, like the total size of the file
 * system, how much is occupied, how much is free, how many files are
 * there in total, etc.
 */
int status_myfs ();

/* Change the access mode of a file or directory */
int chmod_myfs (char *name, int mode);
#endif
