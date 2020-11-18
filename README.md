# File-System
UCD Fall 2020 ECS150(file:///Users/alexoh/Desktop/project_3.html)

## Introduction & Objectives
The goal of this project is to implement the support of a very simple file system, ECS150-FS. This file system is based on a FAT (File Allocation Table) and supports up to 128 files in a single root directory.

The file system is implemented on top of a virtual disk. This virtual disk is actually a simple binary file that is stored on the “real” file system provided by your computer.

Exactly like real hard drives which are split into sectors, the virtual disk is logically split into blocks. The first software layer involved in the file system implementation is the block API and is provided to you. This block API is used to open or close a virtual disk, and read or write entire blocks from it.

Above the block layer, the FS layer is in charge of the actual file system management. Through the FS layer, you can mount a virtual disk, list the files that are part of the disk, add or delete new files, read from files or write to files, etc.

## Layout & Four consecutive logical parts
1. Superblock: The Superblock is the very first block of the disk and contains information about the file system (number of blocks, size of the FAT, etc.)
2. File Allocation Table: The File Allocation Table is located on one or more blocks, and keeps track of both the free data blocks and the mapping between files and the data blocks holding their content.
3. Root directory: The Root directory is in the following block and contains an entry for each file of the file system, defining its name, size and the location of the first data block for this file.
4. Data blocks: All the remaining blocks are Data blocks and are used by the content of files (The size of virtual disk blocks is 4096 bytes).

------------------------------------------------------------------------------------------------------------------
Super Block | FAT#0 | FAT#1 | ... | FAT#End | Root Directory | Data Block#0 | Data Block#1 | ... | Data Block#End |
------------------------------------------------------------------------------------------------------------------

## Work phases
### Phase0: Skeleton code
The code is organized in two parts. In the subdirectory apps, there is one test application that you can use (and enhance to your needs).

In the subdirectory libfs, there are the files composing the file-system library that you must complete. The files to complete are marked with a star (you should have no reason to touch any of the headers which are not marked with a star, even if you think you do).
=> fs.c* / Makefile*

### Phase1: Mounting/Unmounting
In this first phase, you must implement the function fs_mount() and fs_umount().

fs_mount() makes the file system contained in the specified virtual disk “ready to be used”. You need to open the virtual disk, using the block API, and load the meta-information that is necessary to handle the file system operations described in the following phases. fs_umount() makes sure that the virtual disk is properly closed and that all the internal data structures of the FS layer are properly cleaned.

For this phase, you should probably start by defining the data structures corresponding to the blocks containing the meta-information about the file system (superblock, FAT and root directory).

In order to correctly describe these data structures, you will probably need to use the integer types defined in stdint.h, such as int8_t, uint8_t, uint16_t, etc. Also, when describing your data structures and in order to avoid the compiler to interfere with their layout, it’s always good practice to attach the attribute packed to these data structures.

Don’t forget that your function fs_mount() should perform some error checking in order to verify that the file system has the expected format. For example, the signature of the file system should correspond to the one defined by the specifications, the total amount of block should correspond to what block_disk_count() returns, etc.

Once you’re able to mount a file system, you can implement the function fs_info() which prints some information about the mounted file system and make sure that the output corresponds exactly to the reference program.

It is important to observe that the file system must provide persistent storage. Let’s assume that you have created a file system on a virtual disk and mounted it. Then, you create a few files and write some data to them. Finally, you unmount the file system. At this point, all data must be written onto the virtual disk. Another application that mounts the file system at a later point in time must see the previously created files and the data that was written. This means that whenever umount_fs() is called, all meta-information and file data must have been written out to disk.

### Phase2: File Creating/Deletion
In this second phase, you must implement fs_create() and fs_delete() which add or remove files from the file system.

In order to add a file, you need to find an empty entry in the root directory and fill it out with the proper information. At first, you only need to specify the name of the file and reset the other information since there is no content at this point. The size should be set to 0 and the first index on the data blocks should be set to FAT_EOC.

Removing a file is the opposite procedure: the file’s entry must be emptied and all the data blocks containing the file’s contents must be freed in the FAT.

Once you’re able to add and remove files, you can implement the function fs_ls() which prints the listing of all the files in the file system. Make sure that the output corresponds exactly to the reference program.

### Phase3: File descriptor operations
In order for applications to manipulate files, the FS API offers functions which are very similar to the Linux file system operations. fs_open() opens a file and returns a file descriptor which can then be used for subsequent operations on this file (reading, writing, changing the file offset, etc). fs_close() closes a file descriptor.

Your library must support a maximum of 32 file descriptors that can be open simultaneously. The same file (i.e. file with the same name) can be opened multiple times, in which case fs_open() must return multiple independent file descriptors.

A file descriptor is associated to a file and also contains a file offset. The file offset indicates the current reading/writing position in the file. It is implicitly incremented whenever a read or write is performed, or can be explicitly set by fs_lseek().

Finally, the function fs_stat() must return the size of the file corresponding to the specified file descriptor. To append to a file, one can, for example, call fs_lseek(fd, fs_stat(fd));.

### Phase4: File reading/writing
In the last phase, you must implement fs_read() and fs_write() which respectively read from and write to a file.

It is advised to start with fs_read() which is slightly easier to program. You can use the reference program to write a file in a disk image, which you can then read using your implementation.

For these functions, you will probably need a few helper functions. For example, you will need a function that returns the index of the data block corresponding to the file’s offset. For writing, in the case the file has to be extended in size, you will need a function that allocates a new data block and link it at the end of the file’s data block chain. Note that the allocation of new blocks should follow the first-fit strategy (first block available from the beginning of the FAT).

When reading or writing a certain number of bytes from/to a file, you will also need to deal properly with possible “mismatches” between the file’s current offset, the amount of bytes to read/write, the size of blocks, etc.

For example, let’s assume a reading operation for which the file’s offset is not aligned to the beginning of the block or the amount of bytes to read doesn’t span the whole block. You will probably need to read the entire block into a bounce buffer first, and then copy only the right amount of bytes from the bounce buffer into the user-supplied buffer.

The same scenario for a writing operation would slightly trickier. You will probably need to first read the block from disk, then modify only the part starting from the file’s offset with the user-supplied buffer, before you can finally write the dirty block back to the disk.

These special cases happen mostly for small reading/writing operations, or at the beginning or end of a big operation. In big operations (spanning multiple blocks), offsets and sizes are perfectly aligned for all the middle blocks and the procedure is then quite simple, as blocks can be read or written entirely.


