#include "myfs.h"

int main(int argc, char **argv)
{
	create_myfs(10);
	attach_myfs();
	mkdir_myfs("mycode");
	mkdir_myfs("mydocs");
	printf("Listing all files in root dir\n");
	ls_myfs();
	detach_myfs();
	int x = fork();
	if (x == 0) {
		int y = fork();
		if (y == 0) {
			int i;
			attach_myfs();
			chdir_myfs("mydocs");
			char str[100] = "A ";
			int fd = open_myfs("mytest.txt", 'w');
			for (i = 0; i < 26; i++) {
				// printf("%s\n", str);
				write_myfs(fd, 2, str);
				str[0] += 1;
			}
			close_myfs(fd);
			detach_myfs();
		} else {
			attach_myfs();
			chdir_myfs("mycode");
			copy_pc2myfs("tp.txt", "dest.txt");
			detach_myfs();
		}
	} else {
		wait(NULL);
		attach_myfs();
		printf("Listing all files in root dir\n");
		ls_myfs();
		chdir_myfs("mycode");
		printf("Listing all files in mycode dir\n");
		ls_myfs();
		chdir_myfs("..");
		printf("Listing all files in root dir\n");
		ls_myfs();
		chdir_myfs("mydocs");
		printf("Listing all files in mydocs dir\n");
		ls_myfs();
		detach_myfs();
		destroy_myfs();
	}
	return 0;
}
