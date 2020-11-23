#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h> // Check constraints

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */

/* FAT possible values for each entry - Nor = index of next data block */
#define FAT_EOC 0xFFFF
#define AVAILABLE 0

// __attribute__((packed)) specifieseach member of the struct is placed to minimize memory required
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

struct super_block super_t;
struct root_directory root_t;
struct FAT fat_t;

uint32_t num_open_files;


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

	// 2048 entries of 16 bit, 2 bytes 
	// allocate fat_t array then allocate memory to each fat block + error check
	fat_t.entries_fat = malloc(BLOCK_SIZE * sizeof(uint16_t));

	//
	// Incompleted, Recheck needed
	//

	// Root Directory
	if(block_read(super_t.root_dir_index, &root_t) == -1)
		return -1;

	// Data
	if(super_t.root_dir_index + 1 != super_t.data_start_index)
		return -1;
	
	num_open_files = 0;

	return 0;
}

/* Close virtual disk - make sure that Virtual disk is up to date */
int fs_umount(void)
{
	/* TODO: Phase 1 */		
	/* write blocks to disk */
	block_write(0, &super_t);
	block_write(super_t.root_dir_index, &root_t);
	
	/*clean and reset everything */
	free(fat_t.entries_fat);
	memset(root_t.entries_root, 0, FS_FILE_MAX_COUNT); 
	memcmp("", super_t.signature, sizeof(super_t.signature));
	super_t.total_num_blocks = 0;
	super_t.root_dir_index = 0;
	super_t.data_start_index = 0;
	super_t.num_data_blocks = 0;
	super_t.num_FAT_blocks = 0;

	if(num_open_files != 0) {
		return -1;
	}

	/*close virtual disk*/
	if(block_disk_close() == -1) {
		return -1;
	}

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
		if(root_t.entries_root[i].filename == "") {
			*root_t.entries_root[i].filename = filename;
			break;
		}
	}
	root_t.entries_root[i].file_size = 0;
	root_t.entries_root[i].first_data_index = FAT_EOC;
	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	
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

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}
