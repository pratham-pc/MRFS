# Memory Resident File-System

Following is the list of functions available to the user for creating the filesystem:

create_myfs(size):
A memory block of the specified size will be dynamically allocated,
and the data structure for the file system will be initialized in
that space.

copy_pc2myfs(source, dest):
Copy a file from the PC file system to the memory resident file system

copy_myfs2pc(source, dest):
Copy a file from the memory resident file system to the PC file system

rm_myfs(filename):
Remove a specified file from the current working directory

showfile_myfs(filename):
Display the contents of a (text) file

ls_myfs():
Display the list of files in the current working directory

mkdir_myfs (dirname);
Create a new directory in the current working directory

chdir_myfs(dirname):
Change working directory

rmdir_myfs(dirname):
Remove a specified directory

open_myfs(filename, mode):
Open a file for reading or writing

close_myfs(fd):
Close a file

read_myfs(fd, nbytes, buff):
Read a block of bytes from a file

write_myfs(fd, nbytes, buff):
Write a block of bytes into a file

eof_myfs(fd):
Check for end of file

dump_myfs(dumpfile):
Dump the memory resident file system into a file on the secondary memory

attach_myfs():
Just like the shared memory needs to attached to a process before accessing it, same has
to be done for myfs, after its creation.

detach_myfs():
Just like the shared memory needs to detached to a process before accessing it, same has
to be done for myfs, after its creation.

destroy_myfs():
This facilitates the user to clear-up the shared filesystem created using the given API
function. This actually deletes the shared memory created by shmget() in the create_myfs()
function.

MACRO introduced: MAX_INODES : value = 64, and denotes the max inodes the created
filesystem has.
