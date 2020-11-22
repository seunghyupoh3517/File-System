#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF;

/* TODO: Phase 1 */
struct __attribute__((__packed__)) super_block {
	uint64_t signature;
	uint16_t total_num_blocks;
	uint16_t root_direct_index;
	uint16_t data_block_start_index;
	uint16_t num_data_blocks;
	uint8_t num_fat_blocks;
	uint32_t padding[1020]; 
};
struct __attribute__((__packed__)) root_directory {
	uint8_t filename[FS_FILENAME_LEN];
	uint32_t filesize;
	uint16_t i_first_datablock;
	uint32_t padding[3];

};
static uint16_t* FAT;
static uint32_t num_open_files;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	/* write everything to disk */
	
	/*clean everything */
	free(FAT);

	if(num_open_files != 0) {
		return -1;
	}

	/*close virtual disk*/
	if(block_disk_close() == -1) {
		return -1;
	}
	


}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

