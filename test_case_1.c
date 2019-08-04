#include "myfs.h"

int main(int argc, char **argv)
{
	printf("Create filesystem\n");
	create_myfs(10);
	attach_myfs();
	printf("Copy file 1\n");
	copy_pc2myfs("tp1.txt", "file_1.txt");
	printf("Copy file 2\n");
	copy_pc2myfs("tp1.txt", "file_2.txt");
	printf("Copy file 3\n");
	copy_pc2myfs("tp.txt", "file_3.txt");
	printf("Copy file 4\n");
	copy_pc2myfs("tp.txt", "file_4.txt");
	printf("Copy file 5\n");
	copy_pc2myfs("tp.txt", "file_5.txt");
	printf("Copy file 6\n");
	copy_pc2myfs("tp.txt", "file_6.txt");
	printf("Copy file 7\n");
	copy_pc2myfs("tp.txt", "file_7.txt");
	printf("Copy file 8\n");
	copy_pc2myfs("tp.txt", "file_8.txt");
	printf("Copy file 9\n");
	copy_pc2myfs("tp.txt", "file_9.txt");
	printf("Copy file 10\n");
	copy_pc2myfs("tp.txt", "file_10.txt");
	printf("Copy file 11\n");
	copy_pc2myfs("tp.txt", "file_11.txt");
	printf("Copy file 12\n");
	copy_pc2myfs("tp1.txt", "file_12.txt");
	printf("Listing all files\n");
	ls_myfs();
	printf("Try adding existing file\n");
	printf("Copy file 12\n");
	copy_pc2myfs("tp1.txt", "file_12.txt");
	printf("Listing all files\n");
	ls_myfs();
	printf("removing an existing file 'file_4.txt'\n");
	rm_myfs("file_4.txt");
	printf("Listing all files\n");
	ls_myfs();
	printf("removing a non-existing file 'file_4.txt'\n");
	rm_myfs("file_4.txt");
	printf("Listing all files\n");
	ls_myfs();
	detach_myfs();
	destroy_myfs();
	return 0;
}
