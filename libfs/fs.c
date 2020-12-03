#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "disk.h"
#include "fs.h"
#define FAT_EOC 0xFFFF
#define AVAILABLE 0

struct __attribute__((packed)) super_block
{
	/* ECS150FS */
	uint8_t signature[8];
	uint16_t total_num_blocks;
	uint16_t root_dir_index;
	uint16_t data_start_index;
	uint16_t num_data_blocks;
	uint8_t num_FAT_blocks;
	uint8_t unused[4079];
};

struct __attribute__((packed)) entry
{
	/* Description of the file, "test1", 22000, 2 */
	uint8_t filename[FS_FILENAME_LEN];
	uint32_t file_size;
	uint16_t first_data_index;
	uint8_t unused[10];
};

struct __attribute__((packed)) root_directory
{
	/* 32bytes per file, 128 max files  */
	struct entry entries_root[FS_FILE_MAX_COUNT];
};

struct __attribute__((packed)) FAT
{
	/* Linked list of data blocks composing a file, 2bytes per data block, 2048 max blocks */
	uint16_t *entries_fat;
};

struct __attribute__((packed)) file
{
	uint8_t filename[FS_FILENAME_LEN];
	size_t file_offset;
};

struct __attribute__((packed)) file_descriptor_table
{
	size_t num_open_file;
	struct file file_t[FS_OPEN_MAX_COUNT];
};

struct super_block super_t;
struct root_directory root_t;
struct FAT fat_t;
struct file_descriptor_table file_des_table;

/* Open the virtual disk, read the metadata  */
int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	if (block_disk_open(diskname) == -1)
	{
		return -1;
	}

	/* Read the first block of the disk into the superblock */
	if (block_read(0, &super_t) == -1)
	{
		return -1;
	}

	/* Error Checking */
	if (memcmp("ECS150FS", super_t.signature, sizeof(super_t.signature)))
	{
		return -1;
	}

	if (block_disk_count() != super_t.total_num_blocks)
	{
		return -1;
	}

	if (super_t.num_data_blocks + super_t.num_FAT_blocks + 2 != super_t.total_num_blocks)
	{
		return -1;
	}
	
	/* Each data block takes 2bytes of entries in FAT */
	uint16_t extra = 0;
	if (((super_t.num_data_blocks * 2) % BLOCK_SIZE) != 0)
	{
		extra = 1;
	}

	if (super_t.num_FAT_blocks != (((super_t.num_data_blocks * 2) / BLOCK_SIZE) + extra))
	{
		return -1;
	}

	if (super_t.num_FAT_blocks + 1 != super_t.root_dir_index)
	{
		return -1;
	}

	if (super_t.num_FAT_blocks + 2 != super_t.data_start_index)
	{
		return -1;
	}

	if (super_t.root_dir_index + 1 != super_t.data_start_index)
	{
		return -1;
	}


	/* Read FAT entries, Big array of 16bit entries (linked list of data blocks) */
	fat_t.entries_fat = malloc(super_t.num_FAT_blocks * BLOCK_SIZE);
	/* Read block into each FAT block of 4096 */
	for (int i = 1; i <= super_t.num_FAT_blocks; i++)
	{
		if (block_read(i, (void *)fat_t.entries_fat + (i - 1) * BLOCK_SIZE) == -1)
		{
			return -1;
		}
	}
	
	/* Error Checking */
	if (fat_t.entries_fat[0] != FAT_EOC)
	{
		return -1;
	}

	/* Read root directory entries */
	if (block_read(super_t.root_dir_index, &root_t) == -1)
	{
		return -1;
	}

	/* Error Checking */
	size_t root_check = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (root_t.entries_root[i].filename[0] == '\0')
		{
			root_check = 1;
		}

		if (root_check && root_t.entries_root[i].file_size != 0)
		{
			return -1;
		}
		root_check = 0;
	}

	return 0;
}

/* Close virtual disk - Make sure that Virtual disk is up to date */
int fs_umount(void)
{
	/* TODO: Phase 1 */
	/* write blocks to disk */
	if (block_write(0, &super_t) == -1)
	{
		return -1;
	}

	for (int i = 1; i < super_t.root_dir_index; i++)
	{
		if (block_write(i, fat_t.entries_fat + (i - 1) * BLOCK_SIZE) == -1) {

			return -1;
		}
	}

	if (block_write(super_t.root_dir_index, &root_t) == -1)
	{
		return -1;
	}

	
	/* clean and reset everything */ 
	free(fat_t.entries_fat);
	struct entry empty_entry = {.filename = "", .file_size = 0, .first_data_index = 0};
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		root_t.entries_root[i] = empty_entry;
	}
	char *tempstr = "";
	strcpy((char *)super_t.signature, tempstr);
	super_t.total_num_blocks = 0;
	super_t.root_dir_index = 0;
	super_t.data_start_index = 0;
	super_t.num_data_blocks = 0;
	super_t.num_FAT_blocks = 0;

	if (file_des_table.num_open_file != 0)
	{
		return -1;
	}

	/* close virtual disk */
	if (block_disk_close() == -1)
	{
		return -1;
	}

	return 0;
}

/* Show information about volume */
int fs_info(void)
{
	/* TODO: Phase 1 */
	/* 
		FS Info:
		total_blk_count=8198
		fat_blk_count=4
		rdir_blk=5
		data_blk=6
		data_blk_count=8192
		fat_free_ratio=8191/8192
		rdir_free_ratio=128/128
	*/

	/* Error Checking: No underlying virtual disk was opened */
	if (super_t.signature[0] == 0)
	{
		return -1;
	}

	/* Find numbers of free FAT, Root directory */
	int fat_free = 0;
	for (int i = 0; i < super_t.num_data_blocks; i++)
	{
		if (fat_t.entries_fat[i] == AVAILABLE)
		{
			fat_free++;
		}
	}

	int rdir_free = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (root_t.entries_root[i].filename[0] == AVAILABLE)
		{
			rdir_free++;
		}
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", super_t.total_num_blocks);
	printf("fat_blk_count=%d\n", super_t.num_FAT_blocks);
	printf("rdir_blk=%d\n", super_t.root_dir_index);
	printf("data_blk=%d\n", super_t.data_start_index);
	printf("data_blk_count=%d\n", super_t.num_data_blocks);
	printf("fat_free_ratio=%d/%d\n", fat_free, super_t.num_data_blocks);
	printf("rdir_free_ratio=%d/%d\n", rdir_free, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	if (!filename)
	{
		return -1;
	}

	if (strlen(filename) > FS_FILENAME_LEN)
	{
		return -1;
	}

	int count = 0;
	int i;
	for (i = 0; i < FS_FILE_MAX_COUNT; i++){
		/* Duplicate filename */
		if (strcmp((char *)root_t.entries_root[i].filename, filename) == 0)
		{
			return -1;
		}
		/* Root directory already contains MAX files, Count numbers of file */
		if (root_t.entries_root[i].filename[0] != '\0')
		{
			count++;
		}
	}

	if (count >= FS_FILE_MAX_COUNT)
	{
		return -1;
	}
	
	/* Create a new empty file with default properties */
	i = 0;
	for (i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp((char *)root_t.entries_root[i].filename, "") == 0)
		{
			memcpy((char *)root_t.entries_root[i].filename, filename, FS_FILENAME_LEN);
			root_t.entries_root[i].file_size = 0;
			root_t.entries_root[i].first_data_index = FAT_EOC;
			break;
		}
	}

	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	if (!filename)
	{
		return -1;
	}

	if (strlen(filename) > FS_FILENAME_LEN)
	{
		return -1;
	}

	/* if the file is currently open */
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		if(strcmp((char *)file_des_table.file_t[i].filename, filename) == 0)
		{
			return -1;
		}
	}

	/* if there is no filename to delete */
	int pos;
	int check = 1;
	for (pos = 0; pos < FS_FILE_MAX_COUNT; pos++)
	{
		/* Find corresponding index of the file in root dir */
		if (strcmp((char *)root_t.entries_root[pos].filename, filename) == 0)
		{	
			check = 0;
			break;
		}
	}

	if(check)
	{
		return -1;
	}

	/* Delethe filename from the root dir */
	uint16_t data_index = root_t.entries_root[pos].first_data_index;
	while (data_index != FAT_EOC)
	{
		uint16_t next_index = fat_t.entries_fat[data_index];
		fat_t.entries_fat[data_index] = 0;
		data_index = next_index;
	}
	fat_t.entries_fat[data_index] = 0;
	struct entry empty_entry = {.filename = "", .file_size = 0, .first_data_index = FAT_EOC};
	root_t.entries_root[pos] = empty_entry;

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	if (super_t.signature[0] == 0)
	{
		return -1;
	}

	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp((char*) root_t.entries_root[i].filename, "") != 0)
		{
			printf("file: %s, size: %d, data_blk: %d\n", root_t.entries_root[i].filename, root_t.entries_root[i].file_size, root_t.entries_root[i].first_data_index);
		}
	}

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	if (!filename)
	{
		return -1;
	}

	if (strlen(filename) > FS_FILENAME_LEN)
	{
		return -1;
	}

	/* no filename to open */
	int no_file = 1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(filename, (const char *)root_t.entries_root[i].filename) == 0)
		{
			no_file = 0;
			break;
		}
	}
	if (no_file)
	{
		return -1;
	}

	if (file_des_table.num_open_file >= FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	size_t fd_id = 0;
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++)
	{
		/* find free index of file descriptor table which is file descriptor */
		if (file_des_table.file_t[i].filename[0] == '\0')
		{
			memcpy(file_des_table.file_t[i].filename, filename, FS_FILENAME_LEN);
			file_des_table.file_t[i].file_offset = 0;
			fd_id = i;
			file_des_table.num_open_file++;
			break;
		}
	}

	return (int) fd_id;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	if (file_des_table.file_t[fd].filename[0] == '\0')
	{
		return -1;
	}

	file_des_table.file_t[fd].filename[0] = '\0';
	file_des_table.file_t[fd].file_offset = 0;
	file_des_table.num_open_file--;

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	char *name;
	if (file_des_table.file_t[fd].filename[0] == '\0')
	{
		return -1;
	}
	else
	{
		name = (char *)(file_des_table.file_t[fd].filename);
	}

	int current_file_size = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp((char *)root_t.entries_root[i].filename, name) == 0)
		{
			current_file_size = root_t.entries_root[i].file_size;
			return current_file_size;
		}
	}

	return current_file_size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	if (file_des_table.file_t[fd].filename[0] == '\0')
	{
		return -1;
	}

	size_t current_file_size = fs_stat(fd);
	if (offset > current_file_size)
	{
		return -1;
	}

	file_des_table.file_t[fd].file_offset = offset;

	return 0;
}

/* Helper#1: Returns the index of the data block corresponding to the file's offset */
uint16_t data_index(size_t offset, uint16_t f_start)
{	
	size_t counter = BLOCK_SIZE;
	uint16_t ret_data_index = f_start;
	while(ret_data_index != FAT_EOC && counter < offset)
	{
		if(fat_t.entries_fat[ret_data_index] == FAT_EOC)
		{
			return ret_data_index;
		}
		ret_data_index = fat_t.entries_fat[ret_data_index];
		counter += BLOCK_SIZE;
	}

	return ret_data_index;
}
/* finds first empty entry in FAT*/
int first_fit() {
	for(int i = 1; i < super_t.num_FAT_blocks* 2048; i++) {
		if(fat_t.entries_fat[i] == AVAILABLE) {
			return i;
		}
	}
	return -1;
}
/* Gets the index of the file in the root directory*/
int find_pos_entry(uint8_t * filename) {
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp((char *) filename, (char *)root_t.entries_root[i].filename) == 0)
		{
			return i;
		}
	}
	return -1;
}
/* Write to a file */
int fs_write(int fd, void *buf, size_t count) {
	/* Error Checking */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	if (file_des_table.file_t[fd].filename[0] == '\0')
	{
		return -1;
	}

	if (count == 0)
	{
		return -1;
	}

	if (buf == NULL)
	{
		return -1;
	}

	/* Find correponding properties first */
	uint8_t *f_filename = file_des_table.file_t[fd].filename;
	size_t f_offset = file_des_table.file_t[fd].file_offset;
	uint16_t data_block_index;

	int pos_entry = find_pos_entry(f_filename); //get the entry in the root directory that is corresponding to filename 
	uint16_t f_start = root_t.entries_root[pos_entry].first_data_index;
	root_t.entries_root[pos_entry].file_size = count; 

	
	if(f_start != FAT_EOC) { //written to before
		data_block_index = data_index(f_offset, f_start) + super_t.data_start_index;
	} else { 
		data_block_index = first_fit() + super_t.data_start_index;
	}

	root_t.entries_root[pos_entry].first_data_index = abs(data_block_index - super_t.data_start_index);

	/* Bounce buffer */
	void *bounce = malloc(BLOCK_SIZE);	
	int bytes_wrote = 0;
	size_t count_check = BLOCK_SIZE;
	size_t diff = 0;
	uint16_t next_index;
	
	for(size_t i = 0; i <= count; i++) {
		if(i == 0 && count_check >= count) { //First and only block
			diff = count;
			memcpy(bounce, buf + f_offset, count);
			bytes_wrote += diff;
		}
		else if(i == 0 && count_check < count) { //First block 
			diff = BLOCK_SIZE;
			memcpy(bounce, buf + f_offset, BLOCK_SIZE);
			bytes_wrote += diff;
		}
		else if((size_t)bytes_wrote + BLOCK_SIZE >= count) { //Last block
			diff = count - (count_check - BLOCK_SIZE);
			memcpy(bounce, buf + f_offset, diff);
			bytes_wrote += diff;
		}
		else { //Middle blocks
			diff = BLOCK_SIZE;
			memcpy(bounce, buf + f_offset, BLOCK_SIZE);
			bytes_wrote += diff;
			
		}

		if(block_write(data_block_index, bounce) == -1) 
			return -1;
		
		f_offset = bytes_wrote; //changing the offset as we go
		count_check += BLOCK_SIZE;

		if((size_t)bytes_wrote == count) { //Done writing & set last data index to FAT_EOC so its not overwriiten when new files are added
			fat_t.entries_fat[abs(data_block_index - super_t.data_start_index)] = FAT_EOC;
			break;
		}
		
		if(fat_t.entries_fat[abs(data_block_index - super_t.data_start_index)] == 0) { //expand blocks
			fat_t.entries_fat[abs(data_block_index - super_t.data_start_index)] = FAT_EOC;
			next_index = first_fit();
			if(first_fit() == -1) //no more free data blocks
				return -1;
		} else { //overwrite existing blocks
			next_index = fat_t.entries_fat[abs(data_block_index - super_t.data_start_index)];
		}

		fat_t.entries_fat[abs(data_block_index - super_t.data_start_index)] = next_index;
		data_block_index = next_index + super_t.data_start_index;
	}
	free(bounce);
	
	return bytes_wrote;
}

/* Read from a file */
int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}

	if (file_des_table.file_t[fd].filename[0] == '\0')
	{
		return -1;
	}

	if (count == 0)
	{
		return -1;
	}

	if (buf == NULL)
	{
		return -1;
	}

	/* Find correponding properties first */
	uint8_t *f_filename = file_des_table.file_t[fd].filename;
	size_t f_offset = file_des_table.file_t[fd].file_offset;
	/* Index of the first data block of the file */
	
	uint16_t f_start;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp((char *) f_filename, (char *)root_t.entries_root[i].filename) == 0)
		{
			f_start = root_t.entries_root[i].first_data_index;
			break;
		}
	}
	/* Index of the data block corresponding to the file offset */
	uint16_t data_block_index = data_index(f_offset, f_start) + super_t.data_start_index;
	
	void *bounce = malloc(BLOCK_SIZE);	
	size_t count_check = BLOCK_SIZE;
	size_t diff = 0;
	size_t bytes_read = 0;
	void *temp = malloc(BLOCK_SIZE);

	for(size_t i = 0; i <= count; i++) {
		if(block_read(data_block_index, bounce) == -1)
			return -1;
	
		
		if(i == 0 && count_check >= count) { //First and only block
			memcpy(temp, bounce + f_offset, count);
			diff = count;
			bytes_read += count;
		}
		else if(i == 0 && count_check < count) { //First block to read 
			memcpy(temp , bounce + f_offset, BLOCK_SIZE);
			diff = BLOCK_SIZE;
			bytes_read +=  diff;
		}
		else if(bytes_read + BLOCK_SIZE >= count) { //Last block
			memcpy(temp, bounce, BLOCK_SIZE);
			diff = count - (count_check - BLOCK_SIZE);
			bytes_read += diff;
			memcpy(buf + (i* BLOCK_SIZE), temp, diff);
			break;
		}
		else { //Middle blocks
			memcpy(temp, bounce, BLOCK_SIZE);
			diff = BLOCK_SIZE;
			bytes_read += diff;
		}
		
		memcpy(buf + (i*diff), temp, diff);	

		
		if(bytes_read == count) {
			break;	
		}

		count_check += BLOCK_SIZE;

		/* Get next data block to be read*/
		uint16_t next_index = fat_t.entries_fat[data_block_index - super_t.data_start_index];
		data_block_index = next_index + super_t.data_start_index;
		
	}
	free(temp);
	free(bounce);

	return bytes_read;
}
