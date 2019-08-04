#include "myfs.h"

int sort_func(int arr[], int n) {
	int i, mini;
	for (i = 0; i < n; i++) {
		mini = i;
		int j;
		for (j = i + 1; j < n; j++) {
			if (arr[j] < arr[mini]) {
				mini = j;
			}
		}
		if (mini != i) {
			arr[mini] = arr[mini] + arr[i];
			arr[i] = arr[mini] - arr[i];
			arr[mini] = arr[mini] - arr[i];
		}
	}
	return 0;
}


int main(int argc, char **argv)
{
	printf("Restoring a Filesystem\n");
	restore_myfs("mydump-40.backup");
	attach_myfs();
	printf("Listing all files\n");
	ls_myfs();
	int arr[100];
	int i = 0;
	for (i = 0; i < 100; i++) {
		arr[i] = -1;
	}

	int fd_mytest = open_myfs("mytest.txt", 'r');
	char *buff = (char *)malloc(sizeof(char) * 1);
	i = 0;
	while(read_myfs(fd_mytest, 1, buff)) {
		if (buff[0] <= '9' && buff[0] >= '0') {
			if (arr[i] == -1)
				arr[i] = (int)buff[0] - 48;
			else
				arr[i] = ((int)buff[0] - 48)  + arr[i] * 10;
		} else {
			if (i < 100 && arr[i] != -1)
				i++;
		}
	}
	for (i = 0; i < 100; i++) {
		printf("%d \n", arr[i]);
	}
	sort_func(arr, 100);
	fd_mytest = open_myfs("sorted.txt", 'w');
	printf("Listing all files\n");
	ls_myfs();
	for (i = 0; i < 100; i++) {
		char str[100];
		int n = arr[i];
		sprintf(str, "%d", n);
		int j = 0;
		while (n > 0) {
			buff[0] = str[j++];
			write_myfs(fd_mytest, 1, buff);
			n /= 10;
		}
		write_myfs(fd_mytest, 2, ", ");
	}
	close_myfs(fd_mytest);
	printf("Listing all files\n");
	ls_myfs();
	int fd_mytest_new = open_myfs("sorted.txt", 'r');
	while(read_myfs(fd_mytest_new, 1, buff)) {
		printf("%c", buff[0]);
	}
	printf("\n");
	free(buff);
	close_myfs(fd_mytest_new);
	detach_myfs();
	destroy_myfs();
	return 0;
}
