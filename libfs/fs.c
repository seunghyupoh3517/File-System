#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
// Finish up Phase1 + Check Phase2 + Move on to Phase3

// /home/cs150jp/public/p3/apps/test_fs_student.sh testing script

/* TODO: Phase 1 */

/* FAT possible values for each entry - Nor = index of next data block */
#define FAT_EOC 0xFFFF
#define AVAILABLE 0

// __attribute__((packed)) specifies each member of the struct is placed to minimize memory required
struct __attribute__((packed)) super_block{
	/* ECS150FS */
	uint8_t signature[8]; 
	uint16_t total_num_blocks;
	uint16_t root_dir_index;
	uint16_t data_start_index;
	uint16_t num_data_blocks;
	uint8_t num_FAT_blocks;
	uint8_t unused[4079];
};

struct __attribute__((packed)) entry{
	/* "test1", 22000, 2 */
	uint8_t filename[FS_FILENAME_LEN];
	uint32_t file_size;
	uint16_t first_data_index;
	uint8_t unused[10];
};

struct __attribute__((packed)) root_directory{
	// 32bytes of entries - 128 entreis total
	struct entry entries_root[FS_FILE_MAX_COUNT];
};

struct __attribute__((packed)) FAT{
	/* Linked list of data blocks composing a file, 2048 entries */
	uint16_t *entries_fat;
};

struct __attribute__((packed)) file{
	uint8_t filename[FS_FILENAME_LEN];
	size_t file_offset;
};

struct __attribute__((packed)) file_descriptor_table{
	int num_open_file;
	struct file file_t[FS_OPEN_MAX_COUNT];
};

struct file_descriptor_table file_des_table;
struct super_block super_t;
struct root_directory root_t;
struct FAT fat_t;

// uint32_t num_open_files;

/* Open the virtual disk, read the metadata - superblock, root_directory, FAT */
int fs_mount(const char *diskname)
{	
	/* TODO: Phase 1 */
	if(block_disk_open(diskname) == -1)
		return -1;

	// Superblock
	if(block_read(0, &super_t) == -1)
		return -1;
	
	if(memcmp("ECS150FS", super_t.signature, sizeof(super_t.signature)))
		return -1;

	if(block_disk_count() != super_t.total_num_blocks)
		return -1;

	if(super_t.num_data_blocks + super_t.num_FAT_blocks + 2 != super_t.total_num_blocks)
		return -1;
	
	// FAT
	uint16_t extra = 0;
	if(((super_t.num_data_blocks * 2) % BLOCK_SIZE) != 0)
		extra = 1;
	if(super_t.num_FAT_blocks != (((super_t.num_data_blocks * 2) / BLOCK_SIZE) + extra))
		return -1;

	if(super_t.num_FAT_blocks + 1 != super_t.root_dir_index)
		return -1;

	// each fat block can have 2048 entries of 16 bit, 2 bytes - one data block = 2 bytes
	// total fat block = #data block * 16bit 
	// fat_t.entries_fat = malloc(super_t.num_data_blocks * sizeof(uint16_t)); Internal fragmentation?
	
	/*
	fat_t.entries_fat = malloc(BLOCK_SIZE * super_t.num_FAT_blocks);
	// malloc?
	void *fat_block = malloc(BLOCK_SIZE);
	for(int i = 1; i < super_t.root_dir_index; i++){
		
		if(block_read(i, &fat_block)== -1)
			return -1;
		// each block of FAT has block_read
		memcpy(fat_t.entries_fat  + ((i-1) * BLOCK_SIZE), fat_block, BLOCK_SIZE);
	}
	*/

	// free(fat_block);
	fat_t.entries_fat = malloc(super_t.num_data_blocks * sizeof(super_t.num_data_blocks));
	void *fat_block = malloc(BLOCK_SIZE);
	for (int i = 1; i < super_t.root_dir_index; i++) {
		if (block_read(i, fat_block) == -1)
			return -1;
		memcpy(fat_t.entries_fat + (i-1)*BLOCK_SIZE, fat_block, BLOCK_SIZE);
	}

	// Root Directory
	if(block_read(super_t.root_dir_index, &root_t) == -1)
		return -1;

	// Data
	if(super_t.root_dir_index + 1 != super_t.data_start_index)
		return -1;
	
	// ???
	// file_des_table.num_open_file = 0;
	// num_open_files = 0;

	return 0;
}

/* Close virtual disk - make sure that Virtual disk is up to date */
int fs_umount(void)
{
	/* TODO: Phase 1 */		
	/* write blocks to disk */
	if(block_write(0, &super_t) == -1)
		return -1;

	if(block_write(super_t.root_dir_index, &root_t) == -1)
		return -1;
	
	/*clean and reset everything */
	//free(fat_t.entries_fat);
	//memset(root_t.entries_root, 0, FS_FILE_MAX_COUNT); 
	//memcmp("", super_t.signature, sizeof(super_t.signature));
	super_t.total_num_blocks = 0;
	super_t.root_dir_index = 0;
	super_t.data_start_index = 0;
	super_t.num_data_blocks = 0;
	super_t.num_FAT_blocks = 0;

	if(file_des_table.num_open_file != 0) {
		return -1;
	}

	/*close virtual disk*/
	if(block_disk_close() == -1) {
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

	// Return: -1 if no underlying virtual disk was opened
	if(super_t.signature[0] == 0)
		return -1;

	int fat_free = 0;
	for(int i = 0; i < super_t.num_data_blocks; i++){
		if(fat_t.entries_fat[i] == AVAILABLE)
			fat_free++;
	}

	int rdir_free = 0;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(root_t.entries_root[i].filename[0] == 0)
			rdir_free++;
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
	int i;
	for(i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(strcmp((char *)root_t.entries_root[i].filename,"")) {
			strcpy((char *)root_t.entries_root[i].filename, filename);
			break;
		}
	}
	root_t.entries_root[i].file_size = 0;
	root_t.entries_root[i].first_data_index = FAT_EOC;
	return 0;
}

int fs_delete(const char *filename)
{
	int pos = 0;

	/* TODO: Phase 2 */
	for( pos = 0; pos < FS_FILE_MAX_COUNT; pos++) {
		if(strcmp((char *)root_t.entries_root[pos].filename,filename)) {
			break;
		}
	}
	//struct entry empty_entry;
	uint16_t data_index = root_t.entries_root[pos].first_data_index;

	while(data_index != 0xFFFF) {
		uint16_t next_index = fat_t.entries_fat[data_index];
		fat_t.entries_fat[data_index] = 0;
		data_index = next_index;
	}
	fat_t.entries_fat[data_index] = 0;
	//root_t.entries_root[pos] = empty_entry;

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	printf("FS Ls:\n");
	for(int i = 0; i< FS_FILE_MAX_COUNT; i++) {
		printf("file: %s, size: %d, data_blk: %d\n",root_t.entries_root[i].filename,root_t.entries_root[i].file_size,root_t.entries_root[i].first_data_index);
	}
	return 0;
}

// None of these functions should change the filesystem - new reference of strucutres
int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	if(!filename)
		return -1;

	if(strlen(filename) > FS_FILENAME_LEN)
		return -1;

	// no file named @filename to open
	int no_file = 0;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp(filename, (const char*)root_t.entries_root[i].filename) == 0)
			no_file = 1;
	}
	if(!no_file)
		return -1;
	
	if(file_des_table.num_open_file >= FS_OPEN_MAX_COUNT)
		return -1;

	int fd_id = 0;
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++){
		if(file_des_table.file_t[i].filename[0]  == '\0'){
			file_des_table.num_open_file++;
			fd_id = i;
			memcpy(file_des_table.file_t[i].filename, filename, FS_FILENAME_LEN);
			file_des_table.file_t[i].file_offset = 0;
			break;
		}
	}

	return fd_id;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	if(fd < 0  || fd > FS_OPEN_MAX_COUNT)
		return -1;
	
	if(file_des_table.file_t[fd].filename[0] == '\0')
		return -1;
	
	file_des_table.file_t[fd].filename[0] = '\0';
	file_des_table.file_t[fd].file_offset = 0;
	file_des_table.num_open_file--;

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	if(fd < 0  || fd > FS_OPEN_MAX_COUNT)
		return -1;

	char *name;
	if(file_des_table.file_t[fd].filename[0] == '\0')
		return -1;
	else
		name = (char *)(file_des_table.file_t[fd].filename);

	int current_file_size = -1;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp((char *)root_t.entries_root[i].filename, name) == 0){
			current_file_size = root_t.entries_root[i].file_size;
			return current_file_size;
		}
	}

	return current_file_size;
}

// set fs_stat(fd) -> fs_lseek(fd, "offset")
int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	if(fd < 0  || fd > FS_OPEN_MAX_COUNT)
		return -1;

	if(file_des_table.file_t[fd].filename[0] == '\0')
		return -1;

	size_t current_file_size = fs_stat(fd);
	if(offset > current_file_size)
		return -1;

	file_des_table.file_t[fd].file_offset = current_file_size;

	return 0;
}

/* 

Helper functions needed
1. Returns the index of the data block corresponding to the file's offset
2. fs_write: in the case file has to be extended in size -> function that allocates a new data block and link it at the end of file's data block chain
-> first fit strategy (first block available from the begininning of the FAT)
3. Deal with possible mismatches between file's current offset, amount of bytes to r/w, size of blocks

*/

/*
int ret_index_datablock(int fd)
{
	int index = 0;
	for(int i = 0; i <FS_FILE_MAX_COUNT; i++){
		if(strcmp(char *)root_t.entries_root[i].filename, ) == 0){
			index = 
		}
	}

	return index;
}
*/

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	if(fd < 0  || fd > FS_OPEN_MAX_COUNT)
		return -1;

	if(file_des_table.file_t[fd].filename[0] == '\0')
		return -1;
	
	if(count == 0)
		return -1;

	if(buf == NULL)
		return -1;

	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	if(fd < 0  || fd > FS_OPEN_MAX_COUNT)
		return -1;

	if(file_des_table.file_t[fd].filename[0] == '\0')
		return -1;

	if(count == 0)
		return -1;

	if(buf == NULL)
		return -1;
	
	return 0;
}
