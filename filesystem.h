#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <asm/types.h>

#define DEFAULT_BLOCK_SIZE      1024
#define DEFAULT_FILENAME_SIZE   255
#define DEFAULT_MAX_FILE_SIZE   17179869184
#define DEFAULT_MAX_FILES       45
#define DATABLOCK_NUM           15

typedef struct{
    __u32       inodes_count;
    __u32       blocks_count;
    __u32       blocks_per_group;
    __u32       inodes_per_group;
    __u32       inode_blocks;
    __u32       block_size;
    __u32       max_filename_size;
    __u32       max_directory_files;
    __u64       max_file_size;
}mfs_superblock;

typedef struct{
    __u16       node_id;
    __u16       mode;
    __u64       file_size;
    __u32       creation_time;
    __u32       access_time;
    __u32       modification_time;
    __u32       datablocks[DATABLOCK_NUM];
}inode;

typedef struct{
    __u32       next_block;
    __u32       no_descriptors;
    __u32       max_descriptors;
}group_linker;

typedef struct{
    __u32       block_bitmap;
    __u32       inode_bitmap;
    __u32       inode_table;
    __u16       free_blocks;
    __u16       free_inodes;
}group_descriptor;

typedef struct{
    __u32       inodeptr;
    __u16       rec_len;
    __u8        name_len;
    __u8        file_type;
}directory_entry;

typedef struct list_node list_node;

struct list_node{
    list_node   *previous;
    list_node   *next;
    char        *filename;
    inode       mds;
};

typedef struct{
    list_node    *root;
    int         size;
}list_root;

list_root* mfs_listCreate();

int mfs_listAddNodeAB(list_root *root, inode toAdd, char *filename, int name_len);

int mfs_listAddNodeCT(list_root *root, inode toAdd, char *filename, int name_len);

void mfs_listPrint(list_root root, int lFlag);

void mfs_listDestroy(list_root *root);

#endif
