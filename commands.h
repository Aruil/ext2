#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#define COMMAND_SIZE 4096

#define WORKWITH 0
#define LS 1
#define CD 2
#define PWD 3
#define CP 4
#define MV 5
#define RM 6
#define MKDIR 7
#define TOUCH 8
#define IMPORT 9
#define EXPORT 10
#define CAT 11
#define CREATE 12

#include "filesystem.h"

int readCommand(char *command);

char** splitCommand(int wordCount, char *command, int *commandType);

int isValidCommand(char *command, int wordCount);

int mfs_workwith(char **command, mfs_superblock *sblock, int *fd, char *fs,
                 inode *root);

int get_filename(char *dest, char *source);

int mfs_ls(char **command, int fd, mfs_superblock sblock, int argc, inode *curDir);

int mfs_cp();

int mfs_mv(char ** command, int fd, mfs_superblock sblock, int argc, inode *curDir);

int mfs_rm();

int mfs_mkdir(int fd, mfs_superblock *sblock, char **command, inode curDir,
              int argc);

int mfs_touch(char **command, int fd, mfs_superblock sblock, int argc, inode *curDir);

int mfs_import(char **command, int fd, mfs_superblock *sblock, inode *curDir, int argc);

int mfs_copyFromFile(int fd, int toCopy, mfs_superblock *sblock, __u32 *blockNo,
                     __u32 *grDescNo, __u32 limit, __u32 reqBlocks, __u32 *array,
                     __u32 prevLimit, __u64 file_size);

int mfs_insertEntry(int fd, mfs_superblock *sblock, inode folder, inode toInsert,
                    char *path);

char* mfs_extractFilename(char *path);

int mfs_writeInode(int fd, inode *toInsert, mfs_superblock sblock, __u32 blockNo,
                   __u32 grDescNo, __u32 pos, int mode);

int mfs_writeData(int fd, char *toCopy, mfs_superblock sblock, __u32 blockNo,
                  __u32 grDescNo, __u32 *datablocks, __u32 pos, __u32 dataIndex);

void mfs_write_error(char *buffer1, char *buffer2, int errorType);

int mfs_followPath(int fd, mfs_superblock sblock, char *path, inode *ptr, int mode);

int mfs_findEntry(int fd, mfs_superblock sblock, inode curFolder, char *name,
                  int file_type);

int mfs_findInode(int fd, mfs_superblock sblock, __u32 inodeptr, inode *requested);

int mfs_findFree(int fd, __u32 *blockNo, __u32 *grDescNo, mfs_superblock *sblock,
                 int mode);

void mfs_setBit(char *buffer, __u32 index);

__u32 mfs_fzeroBit(int fd, mfs_superblock sblock, __u32 offset);

int mfs_newGroupDescriptor(int fd, mfs_superblock *sblock, __u32 *blockNo,
                           __u32 *grDescNo, group_linker *grlink, __u32 pos);

int mfs_export(char **command, int fd, mfs_superblock sblock, inode *curDir, int argc);

void mfs_export_clean(char *buffer, __u32 *array1, __u32 *array2, __u32 *array3);

int mfs_read(int fd, mfs_superblock sblock, char *buffer, __u32 block);

int mfs_cat();

int mfs_create(char **command, int argc);

int mfs_validFilename(char *filename);

void mfs_checkValues(__u32 *bsize, __u32 *fname, __u64 *fsize, __u32 *dir);

void mfs_create_error(char *path, char *buffer, int fd);

void mfs_goUp(char *buffer);

int mfs_ls(char **command, int fd, mfs_superblock sblock, int argc, inode *curDir);

int mfs_clearEntry(int fd, mfs_superblock sblock, inode dir, inode toClear);

char* mfs_extractPath(char *buffer);

#endif