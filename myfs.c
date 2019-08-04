#include "myfs.h"
#define MAX_INODES 64

int current_inode_num = -1;
char *filesystem;
int shmid = -1;
int key = 9876;
// int shmid_sem = -1, key_sem = 9875;
table_entry te[10] = { {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'}, {-1, 0, 'r'} };  
sem_t *mutex_1;
sem_t *mutex_2;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// To convert the integer mode to linux file descrription format
void strmode(mode_t mode, char * buf) {
	const char chars[] = "rwxrwxrwx";
	for (size_t i = 0; i < 9; i++) {
		buf[i] = (mode & (1 << (8-i))) ? chars[i] : '-';
	}
	buf[9] = '\0';
}

int create_myfs (unsigned int size)
{
	shmid = shmget(key, size * 1024 * 1024 *sizeof(char), IPC_CREAT | 0666);
	// shmid_sem = shmget(key_sem, sizeof(sem_t), IPC_CREAT | 0666);
	int i;
	if (shmid < 0) {
		printf("shmget");
		return -1;
	}
	if (shmid < 0) {
		printf("shmget - sem");
		return -1;
	}
	char *myfs = shmat(shmid, 0, 0);
	filesystem = myfs;
	if (filesystem == (void *) - 1)
		return -1;
	// mutex = shmat(shmid_sem, 0, 0);
	sem_unlink("mutex_1");
	sem_unlink("mutex_2");
	mutex_1 = sem_open("mutex_1", O_CREAT | O_EXCL,  0644, 1);
	mutex_2 = sem_open("mutex_2", O_CREAT | O_EXCL,  0644, 1);
	super_block sb = SB_INIT;
	sb.size = size;
	sb.max_inodes = MAX_INODES;
	sb.max_data_blocks = (size * 1024 * 1024 - 8276 - (80 * MAX_INODES)) / 256;
	memcpy(filesystem, (super_block *)&sb, 8276);

	for (i = 0; i < MAX_INODES ; i++) {
		inode ind = INODE_INIT;
		if (i == 0) {
			ind.type = 1;
			ind.access_permissions = 0755;
			super_block sb_cpy;
			memcpy(&sb_cpy, myfs, 8276);
			sb_cpy.free_inode_list[0] |= (1 << 0); // mark first inode as occupied
			sb_cpy.used_inodes += 1;
			memcpy(myfs, (super_block *)&sb_cpy, 8276);
		}
		// by default inode 0 is the root directory
		memcpy(myfs + 8276 + (80 * i), (inode *)&ind, 80);
	}
	if (shmdt(filesystem)) {
		printf("Error detaching the filesystem\n");
		return -1;
	}
	return 0;
}

int detach_myfs() {
	if (shmdt(filesystem)) {
		printf("Error detaching the filesystem\n");
		return -1;
	}
	return 0;
}

int attach_myfs() {
	filesystem = shmat(shmid, 0, 0);
	if (filesystem == (void *) - 1)
		return -1;
	// mutex = shmat(shmid_sem, 0, 0);
	return 0;
}

// NEW DATA BLOCK ALLOCATION
int find_empty_data_block()
{
	sem_wait(mutex_1);
	super_block sb;
	memcpy(&sb, filesystem, 8276);
	int i = 0, j = 0;
	int edb = -1;
	while (i < sb.max_data_blocks && edb == -1) {
		j = 0;
		while (j < 32 && edb == -1) {
			if (!(sb.free_data_block_list[i] & (1 << j))) {
				edb = i * 32 + j;
			}
			j++;
		}
		i++;
	}
	sb.used_data_blocks++;
	sb.free_data_block_list[edb / 32] |= (1 << edb % 32);
	memcpy(filesystem, (super_block *)&sb, 8276);
	sem_post(mutex_1);
	return edb;
}

// NEW INODE ALLOCATION
int find_empty_inode()
{
	sem_wait(mutex_2);
	super_block sb;
	memcpy(&sb, filesystem, 8276);
	int i = 0, j = 0;
	int edb = -1;
	while (i < sb.max_inodes && edb == -1) {
		j = 0;
		while (j < 32 && edb == -1) {
			if (!(sb.free_inode_list[i] & (1 << j))) {
				edb = i * 32 + j;
			}
			j++;
		}
		i++;
	}
	sb.used_inodes++;
	sb.free_inode_list[edb / 32] |= (1 << edb % 32);
	memcpy(filesystem, (super_block *)&sb, 8276);
	sem_post(mutex_2);
	return edb;
}

int find_next_directory_entry(inode *dir_inode) {
	int i = 0;
	while (dir_inode->pointers_to_data_block[i] != -1 && i < 8) {
		i++;
	}
	if (i == 0) {
		dir_inode->pointers_to_data_block[i] = find_empty_data_block();
		return 8276 + MAX_INODES* 80 + dir_inode->pointers_to_data_block[i] * 256;
	} else {
		//check space in i - 1
		int j = 0;
		while ((int)filesystem[8276 + 80 * MAX_INODES+ 256 * dir_inode->pointers_to_data_block[i - 1] + 32 * j] != 0) {
			j++;
		}

		if (j == 4) {
			dir_inode->pointers_to_data_block[i] = find_empty_data_block();
			return 8276 + MAX_INODES * 80 + dir_inode->pointers_to_data_block[i] * 256;
		} else {
			return 8276 + MAX_INODES * 80 + dir_inode->pointers_to_data_block[i - 1] * 256 + j * 32;
		}
	}
}

int ls_myfs()
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	inode dir_inode;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	int i;
	for (i = 0; i < 8; i++) {
		if (dir_inode.pointers_to_data_block[i] == -1)
			break;
		int j;
		for (j = 0; j < 256; j+= 32) {
			if ((int)filesystem[dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * sb.max_inodes + j] == 0)
				break;
			file_entry fe;
			memcpy(&fe, filesystem + dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * sb.max_inodes + j, 32);
			printf("%s\t", fe.file_name);
			if (strlen(fe.file_name) < 8)
				printf("\t");

			inode fi;
			memcpy(&fi, filesystem + 8276 + fe.inode_number * 80, 80);
			char buf[10];
			strmode(fi.access_permissions, buf);
			if (fi.type == 0) {
				printf("-");
			} else {
				printf("d");
			}
			printf("%s\t", buf);
			char buff[20];
			strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&fi.time_last_read));
			printf("%s\t", buff);
			strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&fi.time_last_modified));
			printf("%s\t", buff);
			strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&fi.time_inode_modified));
			printf("%s\t", buff);
			printf("%u\n", fi.size);
		}
	}
	return 0;
}

int get_indexed_block(inode *in, int j)
{
	if (j < 8) {
		return in->pointers_to_data_block[j];
	}
	else if (j < 72) {
		int loc;
		memcpy(&loc, filesystem + 8276 + 80 * MAX_INODES + 256 * in->pointers_to_data_block[8] + (j - 8) * 4, 4);
		return loc;
	} else {
		int block = (j - 72) / 64;
		int in_block_index = (j - 72) % 64;
		int block_loc;
		int in_block_loc;
		memcpy(&block_loc, filesystem + 8276 + 80 * MAX_INODES + 256 * in->pointers_to_data_block[9] + block * 4, 4);
		memcpy(&in_block_loc, filesystem + 8276 + 80 * MAX_INODES + 256 * block_loc + in_block_index * 4, 4);
		return in_block_loc;
	}
}

int copy_file_in_new_inode(char *source, inode *in)
{
	// read data from file and add
	FILE *fptr;
	fptr = fopen(source, "r");

	if (!fptr)
		return -1;

	char ch = fgetc(fptr);
	int j = 0;
	int bt = 0;
	while (ch != EOF) {
		int i;
		int next_block = get_indexed_block(in, j);
		for (i = 0; i < 256; i++) {
			if (ch != EOF) {
				filesystem[8276 + 80 * MAX_INODES + 256 * next_block + i] = ch;
			} else {
				break;
			}
			ch = fgetc(fptr);
		}
		j++;
	}
	fclose(fptr);
	return 0;
}

int get_next_free_data_block(int j, inode *in)
{
	if (j < 8) {
		int next_free_block = find_empty_data_block();
		in->pointers_to_data_block[j] = next_free_block;
		return in->pointers_to_data_block[j];
	}
	else if (j < 72) {
		if (in->pointers_to_data_block[8] == -1) {
			int idp_loc = find_empty_data_block();
			in->pointers_to_data_block[8] = idp_loc;
		}
		int loc = find_empty_data_block();
		memcpy(filesystem + 8276 + 80 * MAX_INODES + 256 * in->pointers_to_data_block[8] + (j - 8) * 4, &loc, 4);
		return loc;
	} else {
		// todo
		int block = (j - 72) / 64;
		int in_block_index = (j - 72) % 64;
		int block_loc;
		int in_block_loc;
		memcpy(&block_loc, filesystem + 8276 + 80 * MAX_INODES + 256 * in->pointers_to_data_block[9] + block * 4, 4);
		memcpy(&in_block_loc, filesystem + 8276 + 80 * MAX_INODES + 256 * block_loc + in_block_index * 4, 4);
		return in_block_loc;
	}
	return 0;
}

int get_free_data_blocks(int n, inode *in)
{
	int i;
	for (i = 0; i < 8 && i < n; i++) {
		int next_free_block = find_empty_data_block();
		in->pointers_to_data_block[i] = next_free_block;
	}
	if (i == n)
		return 0;
	// create an indirect pointer data block
	int idp_loc = find_empty_data_block();
	if (idp_loc < 0)
		return -1;
	in->pointers_to_data_block[i] = idp_loc;
	for (i = 8; i < 72 && i < n; i++) {
		int next_free_block = find_empty_data_block();
		if (next_free_block < 0)
			return -1;
		memcpy(filesystem + 8276 + MAX_INODES * 80 + idp_loc * 256 + (i - 8) * 4, (int *)&next_free_block, 4);
	}
	if (i == n)
		return 0;
	int iidp_loc = find_empty_data_block();
	if (iidp_loc < 0)
		return -1;
	in->pointers_to_data_block[9] = iidp_loc;
	int ptrs = n / 64;
	int temp = n % 64 == 0 ? 0 : 1;
	ptrs += temp;
	int cnt = 72;

	for (i = 0; i < ptrs; i++) {
		int next_free_block = find_empty_data_block();
		if (next_free_block < 0)
			return -1;
		memcpy(filesystem + 8276 + MAX_INODES * 80 + iidp_loc * 256 + i * 4, (int *)&next_free_block, 4);
		int j = 0;
		int lim;
		if (i == ptrs - 1 && (n - 8) % 64 != 0) {
			lim = (n - 8) % 64;
		} else {
			lim = 64;
		}
		while (j < lim) {
			int next_next_free_block = find_empty_data_block();
			if (next_next_free_block < 0)
				return -1;
			memcpy(filesystem + 8276 + MAX_INODES * 80 + 256 * next_free_block + j * 4, (int *)&next_next_free_block, 4);
			j++;
		}
	}
	return 0;
}

int copy_to_pc(char *dest, inode *in, super_block *sb)
{
	// read data from file and add
	FILE *fptr;
	fptr = fopen(dest, "w");
	if (!fptr)
		return -1;

	int j = 0;
	int bt = in->size;
	while (bt > 0) {
		int i;
		int next_block = get_indexed_block(in, j);
		for (i = 0; i < 256; i++) {
			if (bt <= 0)
				break;
			fprintf(fptr, "%c", filesystem[8276 + 80 * MAX_INODES + 256 * next_block + i]);
			bt--;
		}
		j++;
	}
	fclose(fptr);
	return 0;
}

void show_file(inode *in, super_block *sb)
{
	// read data from file and add
	int j = 0;
	int bt = in->size;
	while (bt > 0) {
		int i;
		int next_block = get_indexed_block(in, j);
		for (i = 0; i < 256; i++) {
			if (bt <= 0)
				break;
			printf("%c", filesystem[8276 + 80 * sb->max_inodes + 256 * next_block + i]);
			bt--;
		}
		j++;
	}
}

int find_file_entry(char *source)
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	inode dir_inode;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	int i;
	int loc = -1;
	for (i = 0; i < 8; i++) {
		if (dir_inode.pointers_to_data_block[i] == -1) {
			break;
		}
		int j;
		for (j = 0; j < 256; j+= 32) {
			if ((int)filesystem[dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * sb.max_inodes + j] == 0) {
				break;
			}
			file_entry fe;
			memcpy(&fe, filesystem + dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * sb.max_inodes + j, 32);
			if (!strcmp(fe.file_name, source)) {
				loc = dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * sb.max_inodes + j;
				i = 8;
				break;
			}
		}
	}
	return loc;
}

int find_file_inode(char *source)
{
	int loc = find_file_entry(source);
	if (loc < 0)
		return -1;
	file_entry fe;
	memcpy(&fe, filesystem + loc, 32);
	return fe.inode_number;
	
}

int showfile_myfs(char *source) {
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	inode in;
	int in_num = find_file_inode(source);
	memcpy(&in, filesystem + 8276 + 80 * in_num, 80);

	show_file(&in, &sb);
	return 0;
}

int copy_myfs2pc (char *source, char *dest)
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	inode in;
	int in_num = find_file_inode(source);
	if (in_num < 0) {
		printf("File '%s' not found\n", source);
		return -1;
	}
	memcpy(&in, filesystem + 8276 + 80 * in_num, 80);

	if (copy_to_pc(dest, &in, &sb))
		return -1;
	return 0;
}

int copy_pc2myfs (char *source, char *dest)
{
	struct stat file_stat;
	// make appropriate changes to the super_block

	int in_num = find_file_inode(dest);
	if (in_num >= 0) {
		printf("File '%s' already present. Please change the destination name\n", dest);
		return -1;
	}

	int free_inode = -1;

	free_inode = find_empty_inode();

	inode dir_inode;
	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	inode new_inode;
	memcpy(&new_inode, filesystem + 8276 + (free_inode * 80), 80);

	int i = 0;
	file_entry fe;
	strcpy(fe.file_name, dest);
	fe.inode_number = (short unsigned int)free_inode;

	// add the file entry in the dir inode's data block
	int next_dir_entry = find_next_directory_entry(&dir_inode);
	memcpy(filesystem + next_dir_entry, (file_entry *)&fe, 32);

	// update the data block list
	new_inode.type = 0;
	new_inode.time_last_read = time(NULL);
	new_inode.time_last_modified = time(NULL);
	new_inode.time_inode_modified = time(NULL);
	if (stat(source, &file_stat) < 0)
		return -1;
	new_inode.size = file_stat.st_size;
	mode_t bits = file_stat.st_mode;
	new_inode.access_permissions = bits;
	int db_req = file_stat.st_size / 256;
	int temp = file_stat.st_size % 256 == 0 ? 0 : 1;
	db_req += temp;
	if (db_req > 4168)
		error("File to be copied too big");

	// get the db_req number of free data blocks
	if (get_free_data_blocks(db_req, &new_inode) < 0)
		return -1;

	if (copy_file_in_new_inode(source, &new_inode) < 0)
		return -1;

	// copyback super block
	memcpy(filesystem + 8276 + cur_in * 80, (inode *)&dir_inode, 80);
	memcpy(filesystem + 8276 + (free_inode * 80), (inode *)&new_inode, 80);
}

int remove_file_data_blocks(inode *in, super_block *sb)
{
	int ch = in->size;
	int temp = ch % 256 == 0 ? 0 : 1;
	int j = ch / 256 + temp;
	while (j >= 0) {
		int i;
		int next_block = get_indexed_block(in, j);
		for (i = 0; i < 256; i++)
			filesystem[8276 + 80 * sb->max_inodes + 256 * next_block + i] = (char)0;
		j--;
	}
	return 0;
}

int rm_myfs(char *filename)
{
	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	inode dir_inode;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	int i;
	int loc = -1;
	int last_j = -1, last_i = -1, lj = -1, li = -1;
	for (i = 0; i < 8; i++) {
		if (dir_inode.pointers_to_data_block[i] == -1) {
			last_i = i - 1;
			break;
		}
		int j;
		for (j = 0; j < 256; j+= 32) {
			if ((int)filesystem[dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j] == 0) {
				break;
			} else {
				last_j = dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j;
				lj = j;
				li = i;
			}
			if (loc == -1) {
				file_entry fe;
				memcpy(&fe, filesystem + dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j, 32);
				if (!strcmp(fe.file_name, filename)) {
					loc = dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j;
				}
			}
		}
	}
	if (loc == -1) {
		printf("File %s not found!\n", filename);
		return -1;
	}
	// get the last inode
	file_entry last_fe;
	memcpy(&last_fe, filesystem + loc, 32);

	sem_wait(mutex_1);
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	if (loc == last_j) {
		filesystem[loc] = (char)0;
		if (lj == 0) {
			sb.free_data_block_list[((loc - 8276 - sb.max_inodes * 80) / 256) / 32] ^= (1 << (((loc - 8276 - sb.max_inodes * 80) / 256) % 32));
			sb.used_data_blocks--;
			dir_inode.pointers_to_data_block[li] = -1;
		}
	} else {
		memcpy(filesystem + loc, filesystem + last_j, 32);
		filesystem[last_j] = (char)0;
		if (lj == 0) {
			sb.free_data_block_list[((loc - 8276 - sb.max_inodes * 80) / 256) / 32] ^= (1 << (((loc - 8276 - sb.max_inodes * 80) / 256) % 32));
			sb.used_data_blocks--;
			dir_inode.pointers_to_data_block[li] = -1;
		}
	}

	sb.used_inodes--;
	sb.free_inode_list[last_fe.inode_number / 32] ^= (1 << last_fe.inode_number % 32);

	inode file_inode;
	memcpy(&file_inode, filesystem + 8276 + 80 * last_fe.inode_number, 80);

	if (remove_file_data_blocks(&file_inode, &sb) < 0)
		return -1;

	inode file_inode1 = INODE_INIT;
	memcpy(filesystem + 8276 + 80 * last_fe.inode_number, (inode *)&file_inode1, 80);
	memcpy(filesystem, (super_block *)&sb, 8276);
	sem_post(mutex_1);

	return 0;
}

int mkdir_myfs (char *dirname) {
	int free_inode = -1;

	int in_num = find_file_inode(dirname);
	if (in_num != -1) {
		printf("Same named directory already exist\n");
		return -1;
	}
	free_inode = find_empty_inode();

	inode dir_inode;
	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	inode new_inode;
	memcpy(&new_inode, filesystem + 8276 + (free_inode * 80), 80);

	int i = 0;
	file_entry fe;
	strcpy(fe.file_name, dirname);
	fe.inode_number = (short unsigned int)free_inode;

	// add the file entry in the dir inode's data block
	int next_dir_entry = find_next_directory_entry(&dir_inode);
	memcpy(filesystem + next_dir_entry, (file_entry *)&fe, 32);

	// update the data block list
	new_inode.type = 1;
	new_inode.time_last_read = time(NULL);
	new_inode.time_last_modified = time(NULL);
	new_inode.time_inode_modified = time(NULL);
	new_inode.access_permissions = 0755;

	// get a free data block
	int next_free_block = find_empty_data_block();
	new_inode.pointers_to_data_block[0] = next_free_block;

	// create fe regarding . and ..
	file_entry fe_dot;
	strcpy(fe_dot.file_name, ".");
	fe_dot.inode_number = (short unsigned int)free_inode;

	file_entry fe_dot_dot;
	strcpy(fe_dot_dot.file_name, "..");
	fe_dot_dot.inode_number = (short unsigned int)cur_in;

	// add the two in a new data block
	memcpy(filesystem + 8276 + MAX_INODES * 80 + next_free_block * 256, &fe_dot, 32);
	memcpy(filesystem + 8276 + MAX_INODES * 80 + next_free_block * 256 + 32, &fe_dot_dot, 32);

	// copyback super block
	//  **********memcpy(filesystem, (super_block *)&sb, 8276);
	memcpy(filesystem + 8276 + cur_in * 80, (inode *)&dir_inode, 80);
	memcpy(filesystem + 8276 + (free_inode * 80), (inode *)&new_inode, 80);
}

int chdir_myfs(char *dirname)
{
	int t = find_file_inode(dirname);
	if (t < 0) {
		printf("Directory '%s' doesn't exist\n", dirname);
		return -1;
	}
	current_inode_num = t;
	return 0;
}

// ADD HERE ASWELL
int rmdir_myfs(char *dirname)
{
	chdir_myfs(dirname);

	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	inode dir_inode;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	int i;
	int loc = -1;
	for (i = 0; i < 8; i++) {
		if (dir_inode.pointers_to_data_block[i] == -1) {
			break;
		}
		int j;
		for (j = 0; j < 256; j+= 32) {
			if ((int)filesystem[dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j] == 0) {
				break;
			}
			file_entry fe;
			memcpy(&fe, filesystem + dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * MAX_INODES + j, 32);
			if (!strcmp(fe.file_name, ".") || !strcmp(fe.file_name, ".."))
				continue;
			inode in;
			memcpy(&in, filesystem + dir_inode.pointers_to_data_block[i] * 256 + 8276 + 80 * fe.inode_number, 80);
			if (in.type == 0)
				rm_myfs(fe.file_name);
			else
				rmdir_myfs(fe.file_name);
		}
	}
	chdir_myfs("..");
	// remove its entry
	rm_myfs(dirname);
	return 0;
}

int dump_myfs(char *dumpfile)
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);
	int i;
	FILE *fptr;
	fptr = fopen(dumpfile, "w");

	for (i = 0; i < sb.size * 1024 * 1024; i++) {
		fprintf(fptr, "%c", filesystem[i]);
	}
	fclose(fptr);
	return 0;
}

int restore_myfs(char *dumpfile)
{
	int i;
	struct stat file_stat;
	FILE *fptr;
	fptr = fopen(dumpfile, "r");

	if (stat(dumpfile, &file_stat) < 0)
		return -1;
	create_myfs(file_stat.st_size / (1024 * 1024));
	attach_myfs();

	char ch = fgetc(fptr);
	i = 0;
	while (i < file_stat.st_size) {
		//printf("Started %d %c\n", i, ch);
		filesystem[i++] = ch;
		ch = fgetc(fptr);
	}

	fclose(fptr);
	detach_myfs();
	return 0;
}

int chmod_myfs (char *name, int mode)
{
	inode in;
	int in_num = find_file_inode(name);
	memcpy(&in, filesystem + 8276 + 80 * in_num, 80);

	// change its mode
	in.access_permissions = mode;

	// copy it back
	memcpy(filesystem + 8276 + 80 * in_num, &in, 80);
	return 0;
}

int status_myfs()
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);
	printf("**************************************\n");
	printf("Filesystem info:\n");
	printf("Total size: %d bytes\n", 256 * sb.max_data_blocks);
	printf("Occupied size: %d bytes\n", 256 * sb.used_data_blocks);
	printf("Free size: %d bytes\n", 256 * (sb.max_data_blocks - sb.used_data_blocks));
	printf("Total no. of files and directories: %d\n", sb.used_inodes);
	printf("**************************************\n");
	return 0;
}

int create_empty_file(char *filename)
{
	// make appropriate changes to the super_block
	super_block sb;
	memcpy(&sb, filesystem, 8276);

	int free_inode = -1;

	free_inode = find_empty_inode();

	inode dir_inode;
	int cur_in = current_inode_num == -1 ? 0 : current_inode_num;
	memcpy(&dir_inode, filesystem + 8276 + cur_in * 80, 80);

	inode new_inode;
	memcpy(&new_inode, filesystem + 8276 + (free_inode * 80), 80);

	int i = 0;
	file_entry fe;
	strcpy(fe.file_name, filename);
	fe.inode_number = (short unsigned int)free_inode;

	// add the file entry in the dir inode's data block
	int next_dir_entry = find_next_directory_entry(&dir_inode);
	memcpy(filesystem + next_dir_entry, (file_entry *)&fe, 32);

	// update the data block list
	new_inode.type = 0;
	new_inode.time_last_read = time(NULL);
	new_inode.time_last_modified = time(NULL);
	new_inode.time_inode_modified = time(NULL);
	new_inode.size = 0;
	new_inode.access_permissions = 0777;

	// copyback super block
	memcpy(filesystem + 8276 + cur_in * 80, (inode *)&dir_inode, 80);
	memcpy(filesystem + 8276 + (free_inode * 80), (inode *)&new_inode, 80);
	return fe.inode_number;
}

int open_myfs (char *filename, char mode)
{
	int i;
	for (i = 0; i < 10; i++) {
		if (te[i].in == -1) {
			int t = find_file_inode(filename);
			if (t == -1) {
				if (mode == 'r') {
					printf("File '%s' doesn't exists\n", filename);
					return -1;
				}
				// create a blank file and open it in 'w' mode
				 t =  create_empty_file(filename);
			}
			te[i].in = t;
			te[i].mode = mode;
			return te[i].in;
		}
	}
	return -1;
}

int close_myfs (int fd) {
	int i;
	for (i = 0; i < 10; i++) {
		if (te[i].in == fd) {
			te[i].in = -1;
			te[i].byte_offset = 0;
			te[i].mode = 'r';
			return 0;
		}
	}
	return -1;
}

int read_myfs (int fd, int nbytes, char *buff)
{
	super_block sb;
	memcpy(&sb, filesystem, 8276);
	// check if fd is opend and in read mode
	int i;
	int table_index;
	for (i = 0; i < 10; i++) {
		if (fd == te[i].in)
			break;
	}
	table_index = i;

	if (table_index == 10) {
		error("FD not present");
	} else if (te[i].mode != 'r') {
		error("FD not opened in read mode");
	}

	inode in;
	memcpy(&in, filesystem + 8276 + 80 * te[table_index].in, 80);

	// access filesystem and read those bytes
	// starting block = byte_offset / 256
	i = te[table_index].byte_offset / 256;
	if (nbytes + te[table_index].byte_offset > in.size)
		nbytes = in.size - te[table_index].byte_offset;
	int cnt = 0;
	while (cnt < nbytes) {
		int j;
		int next_block = get_indexed_block(&in, i);
		for (j = te[table_index].byte_offset % 256; j < 256; j++) {
			if (cnt >= nbytes)
				break;
			buff[cnt++] = filesystem[8276 + 80 * sb.max_inodes + 256 * next_block + j];
			//printf("%d : %c\n", 8276 + 80 * sb.max_inodes + 256 * next_block + j, filesystem[8276 + 80 * sb.max_inodes + 256 * next_block + j]);
			te[table_index].byte_offset++;
		}
	}
	return nbytes;
}

int write_myfs (int fd, int nbytes, char *buff)
{
	// check if fd is opend and in read mode
	int i;
	int table_index;
	for (i = 0; i < 10; i++) {
		if (fd == te[i].in)
			break;
	}
	table_index = i;

	if (table_index == 10) {
		error("FD not present");
	} else if (te[i].mode != 'w') {
		error("FD not opened in write mode");
	}

	inode in;
	memcpy(&in, filesystem + 8276 + 80 * te[table_index].in, 80);

	// write in fd till its full and then append rest stuff

	i = te[table_index].byte_offset / 256;
	int cnt = 0;
	while (te[table_index].byte_offset < in.size && cnt < nbytes) {
		int j;
		int next_block = get_indexed_block(&in, i);
		i++;
		for (j = 0; j < 256; j++) {
			if (cnt >= nbytes)
				break;
			filesystem[8276 + 80 * MAX_INODES + 256 * next_block + j] = buff[cnt++];
			te[table_index].byte_offset++;
		}
	}
	i = te[table_index].byte_offset / 256;
	if (cnt < nbytes && te[table_index].byte_offset % 256 != 0) {
		int last_block = get_indexed_block(&in, i);
		int j;
		for (j = te[table_index].byte_offset % 256; j < 256; j++) {
			if (cnt >= nbytes)
				break;
			filesystem[8276 + 80 * MAX_INODES + 256 * last_block + j] = buff[cnt++];
			in.size++;
			//printf("Appending %d : %c\n", 8276 + 80 * sb.max_inodes + 256 * last_block + j, filesystem[8276 + 80 * sb.max_inodes + 256 * last_block + j]);
			te[table_index].byte_offset++;
		}
	}
	while (cnt < nbytes) {
       // create new block and paste this
		int i;
		int next_block = get_next_free_data_block(te[table_index].byte_offset / 256, &in);
		for (i = 0; i < 256; i++) {
			if (cnt < nbytes) {
				filesystem[8276 + 80 * MAX_INODES + 256 * next_block + i] = buff[cnt++];
				//printf("New Block %d : %c\n", 8276 + 80 * sb.max_inodes + 256 * next_block + i, filesystem[8276 + 80 * sb.max_inodes + 256 * next_block + i]);
				te[table_index].byte_offset++;
				in.size++;
			} else {
				break;
			}
		}
	}
	memcpy(filesystem + 8276 + 80 * te[table_index].in,(inode *)&in, 80);
}

int eof_myfs(int fd)
{
	int i;
	int table_index;
	for (i = 0; i < 10; i++) {
		if (fd == te[i].in)
			break;
	}
	table_index = i;

	if (table_index == 10) {
		return -1;
	}

	inode in;
	memcpy(&in, filesystem + 8276 + 80 * te[table_index].in, 80);
	if (te[table_index].byte_offset == in.size) {
		return 1;
	}
	return 0;
}

int destroy_myfs()
{
	shmctl(shmid, 0,  NULL);
	return 0;
}
