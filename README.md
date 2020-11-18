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


