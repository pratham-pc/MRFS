#include "myfs.h"

int main(int argc, char **argv)
{
	printf("Create filesystem\n");
	create_myfs(10);
	attach_myfs();
	printf("Listing all files\n");
	ls_myfs();
	printf("Open file 'mytest.txt'\n");
	int fd_mytest = open_myfs("mytest.txt", 'w');
	printf("fs = %d\n", fd_mytest);
	printf("Listing all files\n");
	ls_myfs();
	int i;
	srand(time(NULL));
	for (i = 0; i < 100; i++) {
		//printf("Writing random number %d\n", 100);
		int n = rand() % 1000;
		char str[10];
		sprintf(str, "%d", n);
		write_myfs(fd_mytest, (int)strlen(str), str);
		write_myfs(fd_mytest, 2, ", ");
	}
	close_myfs(fd_mytest);
	printf("Listing all files\n");
	ls_myfs();
	printf("Printing mytest.txt\n");
	int fd_mytest_new = open_myfs("mytest.txt", 'r');
	char *buff = (char *)malloc(1 * sizeof(char));
	while(read_myfs(fd_mytest_new, 1, buff)) {
		printf("%c", buff[0]);
	}
	printf("\n");
	free(buff);
	close_myfs(fd_mytest_new);
	printf("Enter the number of copies (0 - 8) to be made:\n");
	int n;
	scanf("%d", &n);
	char fname[12] = "mytest-i.txt";
	for (i = 0; i < n; i++) {
		printf("Creating mytest-%d.txt\n", i+1);
		int fd_mytest_old = open_myfs("mytest.txt", 'r');
		fname[7] = (char)(i + 49);
		int fd_mytest_new = open_myfs(fname, 'w');
		char *buff = (char *)malloc(sizeof(char));
		while(read_myfs(fd_mytest_old, 1, buff)) {
			write_myfs(fd_mytest_new, 1, buff);
		}
		free(buff);
		close_myfs(fd_mytest_old);
		close_myfs(fd_mytest_new);
	}
	// read newly created files
	for (i = 0; i < n; i++) {
		printf("Printing mytest-%d.txt\n", i+1);
		fname[7] = (char)(i + 49);
		int fd_mytest_new = open_myfs(fname, 'r');
		char *buff = (char *)malloc(sizeof(char));
		while(read_myfs(fd_mytest_new, 1, buff)) {
			printf("%c", buff[0]);
		}
		printf("\n");
		free(buff);
		close_myfs(fd_mytest_new);
	}
	
	dump_myfs("mydump-40.backup");
	detach_myfs();
	destroy_myfs();
	return 0;
}
