#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include "commands.h"
#include "login.h"

const __u32 const ACCEPT_BLOCK_SIZE[] = {512, 1024, 2048, 4096, 8192};

int readCommand(char *command){
    int i = 0, wordCount = 0, c, whiteSpace = 0;

    while((c = getcharSilent()) != EOF){
        if(c == VEOF){
            return -1;
        }else if(c == '\t' || c == ' '){
            putchar(c);
            command[i] = c;
            i++;
            if(whiteSpace){
                whiteSpace = 0;
            }
        }else if(c == BACKSPACE){
            if(i){
                i--;
                printf("\b \b");
            }
        }else if(c == '\n'){
            command[i] = '\0';
            putchar(c);
            return wordCount;
        }else{
            if(c > 31 && c < 127){
                if(!whiteSpace){
                    whiteSpace = -1;
                    wordCount++;
                }
                command[i] = c;
                putchar(c);
                i++;
            }
        }
    }

    return -1;
}

char** splitCommand(int wordCount, char *command, int *commandType){
    int i, j;
    char **returnArray, *token;

    token = strtok(command, " \t");
    *commandType = isValidCommand(command, wordCount);
    if(*commandType == -1){
        return NULL;
    }else{
        returnArray = malloc(wordCount * sizeof(char*));
        if(returnArray == NULL){
            perror("splitCommand malloc");
            return NULL;
        }
        for(i = 0; i < wordCount; i++){
            returnArray[i] = malloc(BUFFER_SIZE * sizeof(char));
            if(returnArray[i] == NULL){
                perror("splitCommand malloc");
                for(j = 0; j < i; j++){
                    free(returnArray[j]);
                }
                free(returnArray);
                return NULL;
            }
        }
    }
    i = 0;
    while(token != NULL){
        strcpy(returnArray[i], token);
        token = strtok(NULL, " \t");
        i++;
    }

    return returnArray;
}

int isValidCommand(char *command, int wordCount){
    if(!strcmp("mfs_workwith", command)){
        if(wordCount != 2){
            fprintf(stderr, "mfs_workwith: Invalid arguments.\n");
            return -1;
        }
        return WORKWITH;
    }else if(!strcmp("mfs_ls", command)){
        return LS;
    }else if(!strcmp("mfs_cd", command)){
        if(wordCount != 2){
            fprintf(stderr, "mfs_cd: Invalid arguments.\n");
            return -1;
        }
        return CD;
    }else if(!strcmp("mfs_pwd", command)){
        if(wordCount != 1){
            fprintf(stderr, "mfs_pwd: Invalid arguments.\n");
            return -1;
        }
        return PWD;
    }else if(!strcmp("mfs_cp", command)){
        if(wordCount < 3){
            fprintf(stderr, "mfs_cp: Invalid arguments.\n");
            return -1;
        }
        return CP;
    }else if(!strcmp("mfs_mv", command)){
        if(wordCount < 3){
            fprintf(stderr, "mfs_mv: Invalid arguments.\n");
            return -1;
        }
        return MV;
    }else if(!strcmp("mfs_rm", command)){
        if(wordCount < 2){
            fprintf(stderr, "mfs_rm: Invalid arguments.\n");
            return -1;
        }
        return RM;
    }else if(!strcmp("mfs_mkdir", command)){
        if(wordCount < 2){
            fprintf(stderr, "mfs_mkdir: Invalid arguments.\n");
            return -1;
        }
        return MKDIR;
    }else if(!strcmp("mfs_touch", command)){
        if(wordCount < 2){
            fprintf(stderr, "mfs_touch: Invalid arguments.\n");
            return -1;
        }
        return TOUCH;
    }else if(!strcmp("mfs_import", command)){
        if(wordCount < 3){
            fprintf(stderr, "mfs_import: Invalid arguments.\n");
            return -1;
        }
        return IMPORT;
    }else if(!strcmp("mfs_export", command)){
        if(wordCount < 3){
            fprintf(stderr, "mfs_export: Invalid arguments.\n");
            return -1;
        }
        return EXPORT;
    }else if(!strcmp("mfs_cat", command)){
        if(wordCount < 3){
            fprintf(stderr, "mfs_cat: Invalid arguments.\n");
            return -1;
        }
        return CAT;
    }else if(!strcmp("mfs_create", command)){
        if(wordCount < 2 || wordCount > 10 || wordCount % 2 == 1){
            fprintf(stderr, "mfs_create: Invalid arguments.\n");
            return -1;
        }
        return CREATE;
    }else{
        fprintf(stderr, "Invalid command.\n");
        return -1;
    }
}

int mfs_create(char** command, int argc){
    int                 bsFlag = 0, fnsFlag = 0, mfsFlag = 0, mdfnFlag = 0,
                        path = 0, err = 0, i;
    __u32               offset, inodes_per_block;
    int                 newMFS;
    char                *buffer, *argCheck;
    ssize_t             wr;
    mfs_superblock      sblock;
    group_linker        grlink;
    group_descriptor    grDesc;
    inode               root;
    directory_entry     entry;

    for(i = 1; i < argc; i += 2){
        if(!strcmp(command[i], "-bs")){
            if(!bsFlag) bsFlag = i + 1;
            else err = -1;
        }else if(!strcmp(command[i], "-fns")){
            if(!fnsFlag) fnsFlag = i + 1;
            else err = -1;
        }else if(!strcmp(command[i], "-mfs")){
            if(!mfsFlag) mfsFlag = i + 1;
            else err = -1;
        }else if(!strcmp(command[i], "-mdfn")){
            if(!mdfnFlag) mdfnFlag = i + 1;
            else err = -1;
        }else{
            if(!path) path = i;
            else err = -1;
        }
        if(err){
            fprintf(stderr, "mfs_create: Invalid/duplicate argument.\n");
            return -1;
        }
    }

    if(mfs_validFilename(command[path])){
        return -1;
    }

    if(bsFlag){
        sblock.block_size = (__u32) strtol(command[bsFlag], &argCheck, 0);
        if(*argCheck != '\0'){
            fprintf(stderr, "\n Invalid argument.\n");
            return -1;
        }
    }else{
        sblock.block_size = DEFAULT_BLOCK_SIZE;
    }
    if(fnsFlag){
        sblock.max_filename_size = (__u32) strtol(command[fnsFlag], &argCheck, 0);
        if(*argCheck != '\0'){
            fprintf(stderr, "\n Invalid argument.\n");
            return -1;
        }
    }else{
        sblock.max_filename_size = DEFAULT_FILENAME_SIZE;
    }
    if(mdfnFlag){
        sblock.max_directory_files = (__u32) strtol(command[mdfnFlag], &argCheck, 0);
        if(*argCheck != '\0'){
            fprintf(stderr, "\n Invalid argument.\n");
            return -1;
        }
    }else{
        sblock.max_directory_files = DEFAULT_MAX_FILES;
    }
    if(mfsFlag){
        sblock.max_file_size = (__u32) strtol(command[mfsFlag], &argCheck, 0);
        if(*argCheck != '\0'){
            fprintf(stderr, "\n Invalid argument.\n");
            return -1;
        }
    }else{
        sblock.max_file_size = DEFAULT_MAX_FILE_SIZE;
    }

    sblock.inodes_count = 1;
    sblock.blocks_count = 6;
    sblock.blocks_per_group = sblock.block_size * 8;
    sblock.inodes_per_group = sblock.block_size * 8;

    inodes_per_block = sblock.block_size / sizeof(inode);
    sblock.inode_blocks = (int) ceil((double) sblock.inodes_per_group /
                          inodes_per_block);

    newMFS = open(command[path], O_WRONLY | O_CREAT | O_EXCL, 0666);
    if(newMFS == -1){
        perror("open");
        return -1;
    }

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("buffer malloc");
        close(newMFS);
        unlink(command[path]);
        return -1;
    }

    memset(buffer, 0, sblock.block_size);
    memcpy(buffer, &sblock, sizeof(mfs_superblock));
    wr = write(newMFS, buffer, sblock.block_size);
    if(wr < sblock.block_size){
        mfs_create_error(command[path], buffer, newMFS);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);

    grlink.next_block = 0;
    grlink.no_descriptors = 1;
    grlink.max_descriptors = (sblock.block_size - sizeof(group_linker)) /
                             sizeof(group_descriptor);

    grDesc.block_bitmap = 2;
    grDesc.inode_bitmap = 3;
    grDesc.inode_table = 4;
    grDesc.free_blocks = sblock.block_size * 8 - 1;
    grDesc.free_inodes = sblock.block_size * 8 - 1;

    memcpy(buffer, &grlink, sizeof(group_linker));
    memcpy(buffer + sizeof(group_linker), &grDesc, sizeof(group_descriptor));
    wr = write(newMFS, buffer, sblock.block_size);
    if(wr < sblock.block_size){
        mfs_create_error(command[path], buffer, newMFS);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);

    mfs_setBit(buffer, 0);
    for(i = 0; i < 2; i++){
        wr = write(newMFS, buffer, sblock.block_size);
        if(wr < sblock.block_size){
            mfs_create_error(command[path], buffer, newMFS);
            return -1;
        }
    }
    memset(buffer, 0, sblock.block_size);

    root.node_id = 1;
    root.mode = 0;
    root.file_size = sblock.block_size;
    root.creation_time = time(NULL);
    root.access_time = time(NULL);
    root.modification_time = time(NULL);
    memset(root.datablocks, 0, DATABLOCK_NUM);
    root.datablocks[0] = 4 + sblock.inode_blocks;

    memcpy(buffer, &root, sizeof(inode));
    wr = write(newMFS, buffer, sblock.block_size);
    if(wr < sblock.block_size){
        mfs_create_error(command[path], buffer, newMFS);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);

    for(i = 0; i < sblock.inode_blocks - 1; i++){
        wr = write(newMFS, buffer, sblock.block_size);
        if(wr < sblock.block_size){
            mfs_create_error(command[path], buffer, newMFS);
            return -1;
        }
    }

    offset = 2 * sizeof(directory_entry) + 7;
    memcpy(buffer, &offset, 4);
    entry.inodeptr = 1;
    entry.rec_len = sizeof(directory_entry) + 1;
    entry.name_len = 1;
    entry.file_type = 0;
    memcpy(buffer + 4, &entry, sizeof(directory_entry));
    buffer[4 + sizeof(directory_entry)] = '.';
    entry.rec_len = sizeof(directory_entry) + 2;
    entry.name_len = 2;
    memcpy(buffer + 5 + sizeof(directory_entry), &entry, sizeof(directory_entry));
    buffer[5 + 2 * sizeof(directory_entry)] = '.';
    buffer[6 + 2 * sizeof(directory_entry)] = '.';

    wr = write(newMFS, buffer, sblock.block_size);
    if(wr < sblock.block_size){
        mfs_create_error(command[path], buffer, newMFS);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);

    for(i = 0; i < sblock.block_size * 8 - 1; i++){
        wr = write(newMFS, buffer, sblock.block_size);
        if(wr < sblock.block_size){
            mfs_create_error(command[path], buffer, newMFS);
            return -1;
        }
    }

    close(newMFS);
    free(buffer);
    return 0;
}

int mfs_validFilename(char *filename){
    int     length, i, j = 3;
    char    temp[5];

    temp[4] = '\0';
    length = strlen(filename);
    for(i = length - 1; i > length - 5; i--){
        temp[j] = filename[i];
        j--;
    }

    if(strcmp(temp, ".mfs")){
        fprintf(stderr, "Not a valid mfs filename.\n");
        return -1;
    }

    return 0;
}

void mfs_checkValues(__u32 *bsize, __u32 *fname, __u64 *fsize, __u32 *dir){
    int flag = -1, i;
    for(i = 0; i < 5; i++){
        if(*bsize == ACCEPT_BLOCK_SIZE[i]) flag = 0;
    }
    if(flag) *bsize = DEFAULT_BLOCK_SIZE;
    if(*fname < 10) *fname = DEFAULT_FILENAME_SIZE;
    if(*fsize < 12 * (*bsize)) *fsize = DEFAULT_MAX_FILE_SIZE;
    if(*dir != -1 && *dir < 10) *dir = -1;
}

void mfs_create_error(char *path, char *buffer, int fd){
    perror("mfs_create write");
    close(fd);
    unlink(path);
    free(buffer);
}

int mfs_workwith(char** command, mfs_superblock *sblock, int *fd, char *fs,
                 inode *root){
    int     mfs;
    ssize_t rd;
    off_t   seek;
    char    *buffer;

    if(get_filename(fs, command[1])){
        return -1;
    }
    mfs = open(command[1], O_RDWR, 0);
    if(mfs == -1){
        perror("mfs_workwith open");
        return -1;
    }
    rd = read(mfs, sblock, sizeof(mfs_superblock));
    if(rd == -1){
        perror("mfs_workwith read");
        return -1;
    }
    seek = lseek(mfs, 4 * sblock->block_size, SEEK_SET);
    if(seek == -1){
        perror("mfs_workwith seek");
        return -1;
    }

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_workwith malloc");
        return -1;
    }

    rd = read(mfs, buffer, sblock->block_size);
    if(rd < sblock->block_size){
        perror("mfs_workwith read");
        free(buffer);
        return -1;
    }
    memcpy(root, buffer, sizeof(inode));
    *fd = mfs;
    free(buffer);
    return 0;
}

int get_filename(char *dest, char *source){
    int     length, i, j = 3, k;
    char    temp[5];

    temp[4] = '\0';
    length = strlen(source);
    for(i = length - 1; i > length - 5; i--){
        temp[j] = source[i];
        j--;
    }

    if(strcmp(temp, ".mfs")){
        fprintf(stderr, "Not a valid mfs file.\n");
        return -1;
    }

    while(i){
        if(source[i] == '/') break;
        i--;
    }

    k = 0;
    if(!i) i = -1;
    for(j = i + 1; j < length - 4; j++){
        dest[k] = source[j];
        k++;
    }
    dest[k] = '\0';

    return 0;
}

int mfs_import(char **command, int fd, mfs_superblock *sblock, inode *curDir, int argc){
    int     i, foundFolder = -1, empty, j, dblock;
    int     toCopy;
    off64_t file_size;
    __u32   reqBlocks, *indirectBlock, *dIndirectBlock, *tIndirectBlock,
            blockNo = 1, grDescNo = 0;
    char    *buffer, *impBuffer;
    inode   targetFolder, newInode;

    foundFolder = mfs_followPath(fd, *sblock, command[argc - 1], &targetFolder, 0);
    if(foundFolder == -1 || targetFolder.mode != 0){
        fprintf(stderr, "Target not found or is not a directory.\n");
        return -1;
    }

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_import malloc");
        return -1;
    }

    impBuffer = malloc(sblock->block_size);
    if(impBuffer == NULL){
        perror("mfs_import malloc");
        free(buffer);
        return -1;
    }

    memcpy(&targetFolder, curDir, sizeof(inode));

    for(i = 1; i < argc - 1; i++){
        toCopy = open(command[i], O_RDONLY, 0);
        if(toCopy == -1){
            fprintf(stderr, "%s failed to open.\n", command[i]);
        }else if(mfs_findEntry(fd, *sblock, targetFolder, command[i], 1) != -1){
            fprintf(stderr, "%s already exists at destination.\n", command[i]);
        }else{
            file_size = lseek64(toCopy, 0, SEEK_END);
            if(file_size == -1){
                fprintf(stderr, "%s:", command[i]);
                perror("mfs_import seek");
            }else if(file_size > sblock->max_file_size){
                fprintf(stderr, "%s is too large for this filesystem.\n", command[1]);
            }else{
                reqBlocks = (__u32) ceil((double) file_size / sblock->block_size);

                newInode.mode = 1;
                newInode.file_size = file_size;
                newInode.creation_time = time(NULL);
                newInode.access_time = time(NULL);
                newInode.modification_time = time(NULL);
                memset(newInode.datablocks, 0, DATABLOCK_NUM);

                if(lseek64(toCopy, 0, SEEK_SET) == -1){
                    perror("mfs_import seek");
                    continue;
                }

                mfs_copyFromFile(fd, toCopy, sblock, &blockNo, &grDescNo, 12,
                                 reqBlocks, newInode.datablocks, 0, file_size);
                if(reqBlocks > 12){
                    indirectBlock = malloc(sblock->block_size);
                    memset(indirectBlock, 0, sblock->block_size / 4);
                    mfs_copyFromFile(fd, toCopy, sblock, &blockNo, &grDescNo,
                                     sblock->block_size / 4, reqBlocks,
                                     indirectBlock, 12, file_size);
                    empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                    while(empty == -2){
                        empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                    }
                    memcpy(impBuffer, indirectBlock, sblock->block_size);
                    mfs_writeData(fd, impBuffer, *sblock, blockNo, grDescNo,
                                  newInode.datablocks, empty, 12);
                }
                if(reqBlocks > 12 + sblock->block_size / 4){
                    memset(indirectBlock, 0, sblock->block_size / 4);
                    dIndirectBlock = malloc(sblock->block_size);
                    memset(dIndirectBlock, 0, sblock->block_size / 4);
                    memset(impBuffer, 0, sblock->block_size);
                    j = 0;
                    dblock = 0;
                    while(j + 12 + sblock->block_size / 4 < reqBlocks &&
                        j < sblock->block_size * sblock->block_size / 16){
                        mfs_copyFromFile(fd, toCopy, sblock, &blockNo, &grDescNo,
                                         sblock->block_size / 4, reqBlocks,
                                         dIndirectBlock, 12 + sblock->block_size
                                         / 4 + j, file_size);
                        empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                        while(empty == -2){
                            empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                        }
                        memcpy(impBuffer, dIndirectBlock, sblock->block_size);
                        mfs_writeData(fd, impBuffer, *sblock, blockNo, grDescNo,
                                      indirectBlock, empty, dblock);
                        j += sblock->block_size / 4;
                        if(j % (sblock->block_size / 4) == 0){
                            dblock++;
                            memset(dIndirectBlock, 0, sblock->block_size / 4);
                        }
                    }
                    empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                    while(empty == -2){
                        empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 1);
                    }
                    memcpy(impBuffer, indirectBlock, sblock->block_size);
                    mfs_writeData(fd, impBuffer, *sblock, blockNo, grDescNo,
                                  newInode.datablocks, empty, 13);
                }
                if(reqBlocks > 12 + sblock->block_size / 4 +
                   (sblock->block_size * sblock->block_size / 16)){
                    memset(indirectBlock, 0, sblock->block_size / 4);
                    tIndirectBlock = malloc(sblock->block_size);
                    memset(dIndirectBlock, 0, sblock->block_size / 4);
                    memset(tIndirectBlock, 0, sblock->block_size / 4);
                    j = 0;
                    while(j + 12 + sblock->block_size / 4 + sblock->block_size *
                          sblock->block_size / 16 < reqBlocks){

                    }
                }
                blockNo = 1;
                grDescNo = 0;
                empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 0);
                while(empty == -2){
                    empty = mfs_findFree(fd, &blockNo, &grDescNo, sblock, 0);
                }
                newInode.node_id = empty + 1;
                mfs_writeInode(fd, &newInode, *sblock, blockNo, grDescNo, empty, 0);
                mfs_insertEntry(fd, sblock, targetFolder, newInode, command[i]);
            }
        }
        close(toCopy);
    }

    free(buffer);
    free(impBuffer);
    return 0;
}

int mfs_copyFromFile(int fd, int toCopy, mfs_superblock *sblock, __u32 *blockNo,
                     __u32 *grDescNo, __u32 limit, __u32 reqBlocks, __u32 *array,
                     __u32 prevLimit, __u64 file_size){
    int     empty;
    char    *buffer;
    __u32   i;

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_copyFromFile malloc");
        return -1;
    }

    i = 0;
    while(i < limit && i < reqBlocks){
        empty = mfs_findFree(fd, blockNo, grDescNo, sblock, 1);
        while(empty == -2){
            empty = mfs_findFree(fd, blockNo, grDescNo, sblock, 1);
        }
        memset(buffer, 0, sblock->block_size);
        if(reqBlocks < limit + 1 && i + prevLimit == reqBlocks - 1){
            read(toCopy, buffer, file_size - i * sblock->block_size);
            mfs_writeData(fd, buffer, *sblock, *blockNo, *grDescNo,
                          array, empty, i);
        }else{
            read(toCopy, buffer, sblock->block_size);
            mfs_writeData(fd, buffer, *sblock, *blockNo, *grDescNo,
                          array, empty, i);
        }
        i++;
    }

    free(buffer);
    return 0;
}

int mfs_insertEntry(int fd, mfs_superblock *sblock, inode folder, inode toInsert,
                    char *path){
    int                 i, wr = -1, empty;
    __u32               blockNo, offset, curOffset, block = 1, grDesc = 0;
    char                *buffer, *filename;
    size_t              name_len;
    directory_entry     entry, checkEntry;

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_insertEntry malloc");
        return -1;
    }

    filename = mfs_extractFilename(path);
    name_len = strlen(filename);
    if(name_len > sblock->max_filename_size) name_len = sblock->max_filename_size;
    entry.inodeptr = toInsert.node_id;
    entry.rec_len = sizeof(directory_entry) + name_len;
    entry.name_len = name_len;
    entry.file_type = toInsert.mode;
    for(i = 0; i < DATABLOCK_NUM; i++){
        blockNo = folder.datablocks[i];
        if(blockNo == 0){
            memset(buffer, 0, sblock->block_size);
            offset = 4;
            memcpy(buffer, &offset, 4);
            empty = (__u32) mfs_findFree(fd, &block, &grDesc, sblock, 1);
            while(empty == -2){
                empty = (__u32) mfs_findFree(fd, &block, &grDesc, sblock, 1);
            }
            mfs_writeData(fd, buffer, *sblock, block, grDesc, folder.datablocks,
                          empty, (__u32) i);
            i--;
        }else{
            if(lseek64(fd, blockNo * sblock->block_size, SEEK_SET) == -1){
                perror("mfs_insertEntry seek");
                free(buffer);
                return -1;
            }
            if(read(fd, buffer, sblock->block_size) < sblock->block_size){
                perror("mfs_insertEntry read");
                free(buffer);
                return -1;
            }
            memcpy(&offset, buffer, 4);
            if(offset + sizeof(directory_entry) + name_len < sblock->block_size){
                memcpy(buffer + offset, &entry, sizeof(directory_entry));
                memcpy(buffer + offset + sizeof(directory_entry), filename, name_len);
                offset += sizeof(directory_entry) + name_len;
                memcpy(buffer, &offset, 4);
                wr = 0;
            }else{
                curOffset = 4;
                while(wr && curOffset < offset){
                    memcpy(&checkEntry, buffer + curOffset, sizeof(directory_entry));
                    if(checkEntry.inodeptr == 0 && checkEntry.rec_len >=
                       name_len + sizeof(directory_entry)){
                        entry.rec_len = checkEntry.rec_len;
                        memcpy(buffer + curOffset, &entry, sizeof(directory_entry));
                        memcpy(buffer + curOffset + sizeof(directory_entry),
                               filename, name_len);
                        wr = 0;
                    }
                    curOffset += checkEntry.rec_len;
                }
            }
            if(!wr){
                if(lseek64(fd, blockNo * sblock->block_size, SEEK_SET) == -1){
                    perror("mfs_insertEntry seek");
                    free(buffer);
                    return -1;
                }
                if(write(fd, buffer, sblock->block_size) < sblock->block_size){
                    perror("mfs_insertEntry write");
                    free(buffer);
                    return -1;
                }
                free(buffer);
                return 0;
            }
        }
    }

    return -1;
}

char* mfs_extractFilename(char *path){
    char *token, *returnToken;

    if(path[strlen(path) - 1] == '/') return NULL;

    token = strtok(path, "/");
    while(token != NULL){
        returnToken = token;
        token = strtok(NULL, "/");
    }

    return returnToken;
}

int mfs_writeInode(int fd, inode *toInsert, mfs_superblock sblock, __u32 blockNo,
                   __u32 grDescNo, __u32 pos, int mode){
    char                *buffer = NULL, *buffer2 = NULL;
    __u32               toWrite;
    group_descriptor    grDesc;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        mfs_write_error(buffer, buffer2, 0);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);
    buffer2 = malloc(sblock.block_size);
    if(buffer2 == NULL){
        mfs_write_error(buffer, buffer2, 0);
        return -1;
    }
    memset(buffer2, 0, sblock.block_size);

    if(lseek64(fd, blockNo * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(read(fd, buffer, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 2);
        return -1;
    }
    memcpy(&grDesc, buffer + sizeof(group_linker) + grDescNo * sizeof(group_descriptor),
           sizeof(group_descriptor));
    if(!mode) grDesc.free_inodes--;

    toWrite = grDesc.inode_table + pos / (sblock.block_size / sizeof(inode));

    if(lseek64(fd, toWrite * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(read(fd, buffer2, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 2);
        return -1;
    }

    memcpy(buffer2 + (pos % (sblock.block_size / sizeof(inode)) * sizeof(inode)),
           toInsert, sizeof(inode));
    if(lseek64(fd, toWrite * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(write(fd, buffer2, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 3);
        return -1;
    }

    if(!mode){
        if(lseek64(fd, grDesc.inode_bitmap * sblock.block_size, SEEK_SET) == -1){
            mfs_write_error(buffer, buffer2, 1);
            return -1;
        }
        if(read(fd, buffer2, sblock.block_size) < sblock.block_size){
            mfs_write_error(buffer, buffer2, 2);
            return -1;
        }
        mfs_setBit(buffer2, pos);
        if(lseek64(fd, grDesc.inode_bitmap * sblock.block_size, SEEK_SET) == -1){
            mfs_write_error(buffer, buffer2, 1);
            return -1;
        }
        if(write(fd, buffer2, sblock.block_size) < sblock.block_size){
            mfs_write_error(buffer, buffer2, 3);
            return -1;
        }

        memcpy(buffer + sizeof(group_linker) + grDescNo * sizeof(group_descriptor),
               &grDesc, sizeof(group_descriptor));
        if(lseek64(fd, blockNo * sblock.block_size, SEEK_SET) == -1){
            mfs_write_error(buffer, buffer2, 1);
            return -1;
        }
        write(fd, buffer, sblock.block_size);
    }

    free(buffer);
    free(buffer2);
    return 0;
}

int mfs_writeData(int fd, char *toCopy, mfs_superblock sblock, __u32 blockNo,
                  __u32 grDescNo, __u32 *datablocks, __u32 pos, __u32 dataIndex){
    char                *buffer = NULL, *buffer2 = NULL;
    __u32               toWrite;
    group_descriptor    grDesc;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        mfs_write_error(buffer, buffer2, 0);
        return -1;
    }
    memset(buffer, 0, sblock.block_size);
    buffer2 = malloc(sblock.block_size);
    if(buffer2 == NULL){
        mfs_write_error(buffer, buffer2, 0);
        return -1;
    }
    memset(buffer2, 0, sblock.block_size);

    if(lseek64(fd, blockNo * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(read(fd, buffer, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 2);
        return -1;
    }
    memcpy(&grDesc, buffer + sizeof(group_linker) + grDescNo * sizeof(group_descriptor),
           sizeof(group_descriptor));
    grDesc.free_blocks--;

    toWrite = grDesc.inode_table + sblock.inode_blocks + pos;
    datablocks[dataIndex] = toWrite;

    if(lseek64(fd, toWrite * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(write(fd, toCopy, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 3);
        return -1;
    }

    memset(buffer2, 0, sblock.block_size);
    if(lseek64(fd, grDesc.block_bitmap * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(read(fd, buffer2, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 2);
        return -1;
    }

    mfs_setBit(buffer2, pos);
    if(lseek64(fd, grDesc.block_bitmap * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    if(write(fd, buffer2, sblock.block_size) < sblock.block_size){
        mfs_write_error(buffer, buffer2, 3);
        return -1;
    }
    memcpy(buffer + sizeof(group_linker) + grDescNo * sizeof(group_descriptor),
           &grDesc, sizeof(group_descriptor));
    if(lseek64(fd, blockNo * sblock.block_size, SEEK_SET) == -1){
        mfs_write_error(buffer, buffer2, 1);
        return -1;
    }
    write(fd, buffer, sblock.block_size);

    free(buffer);
    free(buffer2);
    return 0;
}

void mfs_write_error(char *buffer1, char *buffer2, int errorType){
    if(errorType == 0) perror("mfs_write malloc");
    else if(errorType == 1) perror("mfs_write seek");
    else if(errorType == 1) perror("mfs_write read");
    else perror("mfs_write write");

    if(buffer1) free(buffer1);
    if(buffer2) free(buffer2);
}

int mfs_followPath(int fd, mfs_superblock sblock, char *path, inode *ptr, int mode){
    int     found;
    char    *buffer, *token;
    inode   curFolder;

    if(path[0] == '/' || path[0] == '.' || (path[0] > 64 && path[0] < 91) ||
       (path[0] > 96 && path[0] < 123)){
        if(!strcmp(path, ".")){
            return 0;
        }
        buffer = malloc(sblock.block_size);
        if(buffer == NULL){
            perror("mfs_followPath malloc");
            return -1;
        }
        if(path[0] == '/'){
            if(lseek(fd, 4 * sblock.block_size, SEEK_SET) == -1){
                perror("mfs_followPath seek");
                free(buffer);
                return -1;
            }
            if(read(fd, buffer, sblock.block_size) < sblock.block_size){
                perror("mfs_followPath read");
                free(buffer);
                return -1;
            }
            if(strcmp(path, "/")){
                memcpy(ptr, buffer, sizeof(inode));
                free(buffer);
                return 0;
            }else{
                memcpy(&curFolder, buffer, sizeof(inode));
            }
        }else{
            memcpy(&curFolder, ptr, sizeof(inode));
        }
        token = strtok(path, "/");
        while(token != NULL){
            found = mfs_findEntry(fd, sblock, curFolder, token, mode);
            if(found == -1){
                free(buffer);
                return -1;
            }
            if(mfs_findInode(fd, sblock, found, &curFolder) == -1){
                free(buffer);
                return -1;
            }
            token = strtok(NULL, "/");
        }
        memcpy(ptr, &curFolder, sizeof(inode));
        return 0;
    }else{
        fprintf(stderr, "Invalid path.\n");
        return -1;
    }

    return -1;
}

int mfs_findEntry(int fd, mfs_superblock sblock, inode curFolder, char *name,
                  int file_type){
    char            *buffer, curName[256];
    int             i = 0;
    int             curOffset, offset, namelen;
    directory_entry entry;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_findEntry malloc");
        return -1;
    }
    namelen = strlen(name);
    while(curFolder.datablocks[i] != 0){
        if(lseek(fd, curFolder.datablocks[i] * sblock.block_size, SEEK_SET) == -1){
            perror("mfs_findEntry seek");
            free(buffer);
            return -1;
        }
        if(read(fd, buffer, sblock.block_size) < sblock.block_size){
            perror("mfs_findEntry read");
            free(buffer);
            return -1;
        }
        memcpy(&offset, buffer, 4);
        curOffset = 4;
        while(curOffset < offset){
            memcpy(&entry, buffer + curOffset, sizeof(directory_entry));
            if(entry.inodeptr != 0 && namelen == entry.name_len){
                memcpy(curName, buffer + curOffset + sizeof(directory_entry),
                       entry.name_len);
                if(!strncmp(name, curName, namelen)){
                    free(buffer);
                    if(entry.file_type == file_type){
                        return entry.inodeptr;
                    }else{
                        fprintf(stderr, "%s not the requested file type\n", name);
                        return -1;
                    }
                }
            }
            curOffset += entry.rec_len;
        }
        i++;
    }

    return -1;
}

int mfs_findInode(int fd, mfs_superblock sblock, __u32 inodeptr, inode *requested){
    int                 block_group, index, desc_block, dpos, i, block = 1,
                        inode_block, ipos;
    char                *buffer;
    group_linker        link;
    group_descriptor    grDesc;

    block_group = (inodeptr - 1) / sblock.inodes_per_group;
    index = (inodeptr - 1) % sblock.inodes_per_group;
    desc_block = block_group / ((sblock.block_size - sizeof(group_linker)) /
                 sizeof(group_descriptor));
    dpos = block_group % ((sblock.block_size - sizeof(group_linker)) /
                 sizeof(group_descriptor));
    inode_block = index / (sblock.block_size / sizeof(inode));
    ipos = index % (sblock.block_size / sizeof(inode));

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_findInode malloc");
        return -1;
    }

    for(i = 0; i < desc_block + 1; i++){
        if(lseek(fd, block * sblock.block_size, SEEK_SET) == -1){
            perror("mfs_findInode seek");
            free(buffer);
            return -1;
        }
        if(read(fd, buffer, sblock.block_size) < sblock.block_size){
            perror("mfs_findInode read");
            free(buffer);
            return -1;
        }
        memcpy(&link, buffer, sizeof(group_linker));
        block = link.next_block;
    }

    memcpy(&grDesc, buffer + sizeof(group_linker) + dpos * sizeof(group_descriptor),
           sizeof(group_descriptor));
    if(lseek(fd, (grDesc.inode_table + inode_block) * sblock.block_size, SEEK_SET) == -1){
        perror("mfs_findInode seek");
        free(buffer);
        return -1;
    }
    if(read(fd, buffer, sblock.block_size) < sblock.block_size){
        perror("mfs_findInode read");
        free(buffer);
        return -1;
    }
    memcpy(requested, buffer + ipos * sizeof(inode), sizeof(inode));

    free(buffer);
    return 0;
}

int mfs_findFree(int fd, __u32 *blockNo, __u32 *grDescNo, mfs_superblock *sblock,
                 int mode){
    int                 empty = -1, i, freeptr;
    char                *buffer;
    group_descriptor    grDesc;
    group_linker        grlink;
    ssize_t             rd;
    off_t               seek;

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_findFree malloc");
        return -1;
    }

    seek = lseek(fd, (*blockNo) * sblock->block_size, SEEK_SET);
    if(seek == -1){
        perror("mfs_findFree seek");
        return -1;
    }

    rd = read(fd, buffer, sblock->block_size);
    if(rd < sblock-> block_size){
        perror("mfs_findFree read");
        return -1;
    }

    memcpy(&grlink, buffer, sizeof(group_linker));
    for(i = *grDescNo; i < grlink.no_descriptors; i++){
        memcpy(&grDesc, buffer + sizeof(group_linker) + i * sizeof(group_descriptor),
               sizeof(group_descriptor));
        if(!mode) freeptr = grDesc.free_inodes;
        else freeptr = grDesc.free_blocks;
        if(freeptr != 0){
            if(!mode) empty = mfs_fzeroBit(fd, *sblock, grDesc.inode_bitmap);
            else empty = mfs_fzeroBit(fd, *sblock, grDesc.block_bitmap);
            *grDescNo = i;
            free(buffer);
            return empty;
        }
    }

    if(grlink.next_block != 0){
        *blockNo = grlink.next_block;
        *grDescNo = 0;
        free(buffer);
        return -2;
    }else{
        if(i == grlink.max_descriptors){
            if(!mfs_newGroupDescriptor(fd, sblock, blockNo, grDescNo, &grlink, 0)){
                *blockNo = grlink.next_block;
                *grDescNo = 0;
                free(buffer);
                return -2;
            }
        }else{
            if(!mfs_newGroupDescriptor(fd, sblock, blockNo, grDescNo, &grlink, i)){
                *grDescNo += 1;
                free(buffer);
                return -2;
            }
        }
    }

    return -1;
}

__u32 mfs_fzeroBit(int fd, mfs_superblock sblock, __u32 offset){
    int     i, pos = 0;
    char    *buffer;
    __u32   returnValue = 0, bitpack, invBitpack;
    off_t   seek;
    ssize_t rd;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_fzeroBit malloc");
        return 0;
    }

    seek = lseek(fd, offset * sblock.block_size, SEEK_SET);
    if(seek == -1){
        free(buffer);
        perror("mfs_fzeroBit seek");
        return 0;
    }

    rd = read(fd, buffer, sblock.block_size);
    if(rd < sblock.block_size){
        free(buffer);
        perror("mfs_fzeroBit read");
        return 0;
    }

    for(i = 0; i < sblock.block_size / 4; i++){
        memcpy(&bitpack, buffer + i * 4, 4);
        if(bitpack == 0){
            free(buffer);
            return returnValue;
        }else{
            invBitpack = ~bitpack;
            while(invBitpack >>= 1){
                pos++;
            }
            if(bitpack != 0xffffffff){
                free(buffer);
                return returnValue + 31 - pos;
            }else{
                returnValue += 32;
            }
        }
    }

    return 0;
}

void mfs_setBit(char *buffer, __u32 index){
    __u32     whichInt, whichBit;
    __u32     number = 0;

    whichInt = index / 32;
    whichBit = index % 32;

    memcpy(&number, buffer + whichInt * 4, 4);
    number |= 1 << (31 - whichBit);

    memcpy(buffer + whichInt * 4, &number, 4);
}

int mfs_newGroupDescriptor(int fd, mfs_superblock *sblock, __u32 *blockNo,
                           __u32 *grDescNo, group_linker *grlink, __u32 pos){
    int                 i, ptr;
    char                *buffer;
    group_descriptor    grDesc;
    group_linker        newGrlink;
    off_t               seek;

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_newGroupDescriptor malloc");
        return -1;
    }
    memset(buffer, 0, sblock->block_size);

    seek = lseek(fd, 0 , SEEK_END);
    if(seek == -1){
        perror("mfs_newGroupDescriptor seek");
        free(buffer);
        return -1;
    }

    ptr = seek / sblock->block_size;
    grDesc.free_blocks = sblock->block_size * 8;
    grDesc.free_inodes = grDesc.free_blocks;

    if(!pos){
        grlink->next_block = ptr;
        newGrlink.next_block = 0;
        newGrlink.no_descriptors = 1;
        newGrlink.max_descriptors = grlink->max_descriptors;
        grDesc.block_bitmap = ptr + 1;
        grDesc.inode_bitmap = ptr + 2;
        grDesc.inode_table = ptr + 3;
        memcpy(buffer, &newGrlink, sizeof(group_linker));
        memcpy(buffer + sizeof(group_linker), &grDesc, sizeof(group_descriptor));
        if(write(fd, buffer, sblock->block_size) < sblock->block_size){
            perror("mfs_newGroupDescriptor write");
            free(buffer);
            return -1;
        }
    }else{
        grlink->no_descriptors++;
        grDesc.block_bitmap = ptr;
        grDesc.inode_bitmap = ptr + 1;
        grDesc.inode_table = ptr + 2;
        seek = lseek(fd, (*blockNo) * sblock->block_size, sblock->block_size);
        if(seek == -1){
            perror("mfs_newGroupDescriptor seek");
            free(buffer);
            return -1;
        }
        if(read(fd, buffer, sblock->block_size)){

        }
        memcpy(buffer, grlink, sizeof(group_linker));
        memcpy(buffer + sizeof(group_linker) + pos * sizeof(group_descriptor),
               &grDesc, sizeof(group_descriptor));
        if(lseek(fd, (*blockNo) * sblock->block_size, sblock->block_size) == -1){
            perror("mfs_newGroupDescriptor seek");
            free(buffer);
            return -1;
        }
        if(write(fd, buffer, sblock->block_size) < sblock->block_size){
            perror("mfs_newGroupDescriptor write");
            free(buffer);
            return -1;
        }
        if(lseek(fd, 0, SEEK_END) == -1){
            perror("mfs_newGroupDescriptor seek");
            free(buffer);
            return -1;
        }
    }
    memset(buffer, 0, sblock->block_size);
    for(i = 0; i < 2 + sblock->block_size * 8 + sblock->block_size * 8 /
        (sblock->block_size / sizeof(inode)); i++){
        if(write(fd, buffer, sblock->block_size) < sblock->block_size){
            perror("mfs_newGroupDescriptor write");
            free(buffer);
            return -1;
        }
    }

    free(buffer);
    return 0;
}

int mfs_export(char **command, int fd, mfs_superblock sblock, inode *curDir, int argc){
    int         i, newFile, error;
    char        *buffer, *path, *filename;
    __u32       j, reqBlocks, toWrite, *indirectBlock = NULL,
                *sIndirectBlock = NULL, *tIndirectBlock = NULL;
    __u64       remSize;
    DIR         *checkPath;
    inode       target;

    checkPath = opendir(command[argc - 1]);
    if(checkPath == NULL){
        perror("mfs_export opendir");
        return -1;
    }

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_export malloc");
        return -1;
    }
    path = malloc(strlen(command[argc - 1]) + sblock.max_filename_size + 1);
    if(path == NULL){
        perror("mfs_export malloc");
        mfs_export_clean(buffer, indirectBlock, sIndirectBlock, tIndirectBlock);
        return -1;
    }

    indirectBlock = malloc(sblock.block_size);
    if(indirectBlock == NULL){
        perror("mfs_export malloc");
        mfs_export_clean(buffer, indirectBlock, sIndirectBlock, tIndirectBlock);
        return -1;
    }

    sIndirectBlock = malloc(sblock.block_size);
    if(sIndirectBlock == NULL){
        perror("mfs_export malloc");
        mfs_export_clean(buffer, indirectBlock, sIndirectBlock, tIndirectBlock);
        return -1;
    }

    memcpy(&target, curDir, sizeof(inode));

    for(i = 1; i < argc - 1; i++){
        if(mfs_followPath(fd, sblock, command[i], &target, 1) == -1){
            fprintf(stderr, "%s not found.\n", command[i]);
        }else{
            filename = mfs_extractFilename(command[i]);
            if(filename == NULL){
                fprintf(stderr, "No filename given.\n");
                continue;
            }
            strcpy(path, command[argc - 1]);
            if(path[strlen(path) - 1] != '/') strcat(path, "/");
            strcat(path, filename);
            newFile = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
            if(newFile == -1){
                perror("mfs_export open");
            }else{
                reqBlocks = (__u32) ceil((double) target.file_size / sblock.block_size);
                remSize = target.file_size;
                j = 0;
                error = -1;
                while(error && j < DATABLOCK_NUM - 3 && j < reqBlocks){
                    memset(buffer, 0, sblock.block_size);
                    if(mfs_read(fd, sblock, buffer, target.datablocks[j]) == -1){
                        error = 0;
                    }else{
                        if(remSize >= sblock.block_size){
                            toWrite = sblock.block_size;
                            remSize -= sblock.block_size;
                        }else{
                            toWrite = remSize;
                            remSize = 0;
                        }
                        if(write(newFile, buffer, toWrite) < toWrite){
                            perror("mfs_export write");
                            error = 0;
                        }
                    }
                    j++;
                }
                if(remSize){
                    mfs_read(fd, sblock, buffer, target.datablocks[12]);
                    memcpy(indirectBlock, buffer, sblock.block_size);
                    j = 0;
                    while(j < sblock.block_size / 4 && j + 12 < reqBlocks){
                        mfs_read(fd, sblock, buffer, indirectBlock[j]);
                        if(remSize >= sblock.block_size){
                            toWrite = sblock.block_size;
                            remSize -= sblock.block_size;
                        }else{
                            toWrite = remSize;
                            remSize = 0;
                        }
                        if(write(newFile, buffer, toWrite) < toWrite){
                            perror("mfs_export write");
                            error = 0;
                        }
                        j++;
                    }
                }
                if(remSize){
                    mfs_read(fd, sblock, buffer, target.datablocks[13]);
                    memcpy(indirectBlock, buffer, sblock.block_size);
                    j = 0;
                    while(j < sblock.block_size * sblock.block_size / 16 &&
                          j + 12 + sblock.block_size / 4 < reqBlocks){
                        if(j % (sblock.block_size / 4) == 0){
                            mfs_read(fd, sblock, buffer,
                                     indirectBlock[j / (sblock.block_size / 4)]);
                            memcpy(sIndirectBlock, buffer, sblock.block_size);
                        }
                        mfs_read(fd, sblock, buffer,
                                 sIndirectBlock[j % (sblock.block_size / 4)]);
                        if(remSize >= sblock.block_size){
                            toWrite = sblock.block_size;
                            remSize -= sblock.block_size;
                        }else{
                            toWrite = remSize;
                            remSize = 0;
                        }
                        if(write(newFile, buffer, toWrite) < toWrite){
                            perror("mfs_export write");
                            error = 0;
                        }
                        j++;
                    }
                }
                if(remSize){
                    mfs_read(fd, sblock, buffer, target.datablocks[14]);
                    memcpy(indirectBlock, buffer, sblock.block_size);
                    j = 0;
                    while(j < sblock.block_size * sblock.block_size * sblock.block_size
                          && j + 12 + sblock.block_size / 4 + sblock.block_size *
                          sblock.block_size / 16){
                        if(j % (sblock.block_size * sblock.block_size / 16) == 0){
                            mfs_read(fd, sblock, buffer,
                                     indirectBlock[j / (sblock.block_size * sblock.block_size / 16)]);
                            memcpy(sIndirectBlock, buffer, sblock.block_size);
                        }
                        if(j % (sblock.block_size / 4) == 0){
                            mfs_read(fd, sblock, buffer,
                                     sIndirectBlock[j / (sblock.block_size / 4)]);
                            memcpy(tIndirectBlock, buffer, sblock.block_size);
                        }
                        mfs_read(fd, sblock, buffer,
                                 tIndirectBlock[j % (sblock.block_size / 4)]);
                        if(remSize >= sblock.block_size){
                            toWrite = sblock.block_size;
                            remSize -= sblock.block_size;
                        }else{
                            toWrite = remSize;
                            remSize = 0;
                        }
                        if(write(newFile, buffer, toWrite) < toWrite){
                            perror("mfs_export write");
                            error = 0;
                        }
                        j++;
                    }
                }
                close(newFile);
                if(!error) unlink(path);
            }
        }
    }

    mfs_export_clean(buffer, indirectBlock, sIndirectBlock, tIndirectBlock);
    return 0;
}

void mfs_export_clean(char *buffer, __u32 *array1, __u32 *array2, __u32 *array3){
    if(buffer != NULL) free(buffer);
    if(array1 != NULL) free(array1);
    if(array2 != NULL) free(array2);
    if(array3 != NULL) free(array3);
}

int mfs_read(int fd, mfs_superblock sblock, char *buffer, __u32 block){
    if(lseek64(fd, block * sblock.block_size, SEEK_SET) == -1){
        perror("mfs_read seek");
        return -1;
    }
    if(read(fd, buffer, sblock.block_size) < sblock.block_size){
        perror("mfs_read read");
        return -1;
    }

    return 0;
}

int mfs_mkdir(int fd, mfs_superblock *sblock, char **command, inode curDir,
              int argc){
    int                 i, entry, error;
    __u32               iblock, igrDesc, empty, offset, block, grDesc;
    char                *toInsert, *token, *buffer;
    inode               dir, newDir;
    directory_entry     dirEntry;

    buffer = malloc(sblock->block_size);
    if(buffer == NULL){
        perror("mfs_mkdir malloc");
        return -1;
    }

    offset = 2 * sizeof(directory_entry) + 3 + 4;

    for(i = 1; i < argc; i++){
        toInsert = mfs_extractFilename(command[i]);
        if(toInsert == NULL){
            fprintf(stderr, "No filename given.\n");
            continue;
        }
        entry = 0;
        error = 0;
        iblock = 1;
        igrDesc = 0;
        if(command[i][0] == '/'){
            mfs_findInode(fd, *sblock, 1, &dir);
        }else{
            memcpy(&dir, &curDir, sizeof(inode));
        }
        token = strtok(command[i], "/");
        while(!error && entry != -1 && token != NULL){
            if(!strcmp(toInsert, token)) break;
            entry = mfs_findEntry(fd, *sblock, dir, token, 0);
            if(entry != -1){
                error = mfs_findInode(fd, *sblock, entry, &dir);
            }
            token = strtok(NULL, "/");
        }
        empty = mfs_findFree(fd, &iblock, &igrDesc, sblock, 0);
        while(empty == -2){
            empty = mfs_findFree(fd, &iblock, &igrDesc, sblock, 0);
        }
        newDir.node_id = empty;
        newDir.mode = 0;
        newDir.creation_time = time(NULL);
        newDir.access_time = time(NULL);
        newDir.modification_time = time(NULL);
        memset(newDir.datablocks, 0, DATABLOCK_NUM);
        block = 1;
        grDesc = 0;
        empty = mfs_findFree(fd, &block, &grDesc, sblock, 1);
        while(empty == -2){
            empty = mfs_findFree(fd, &block, &grDesc, sblock, 1);
        }
        memcpy(buffer, &offset, 4);
        dirEntry.inodeptr = newDir.node_id;
        dirEntry.rec_len = sizeof(directory_entry) + 1;
        dirEntry.name_len = 1;
        dirEntry.file_type = 0;
        memcpy(buffer + 4, &dirEntry, sizeof(directory_entry));
        buffer[4 + sizeof(directory_entry)] = '.';
        dirEntry.inodeptr = dir.node_id;
        dirEntry.rec_len = sizeof(directory_entry) + 2;
        dirEntry.name_len = 2;
        memcpy(buffer + 4 + sizeof(directory_entry) + 1, &dirEntry,
               sizeof(directory_entry));
        buffer[4 + 2 * sizeof(directory_entry) + 1] = '.';
        buffer[4 + 2 * sizeof(directory_entry) + 2] = '.';
        if(mfs_writeData(fd, buffer, *sblock, block, grDesc, newDir.datablocks,
                      empty, 0) != -1){
            mfs_writeInode(fd, &newDir, *sblock, iblock, igrDesc,
                           newDir.node_id % (sblock->block_size * 8), 0);
            mfs_insertEntry(fd, sblock, dir, newDir, command[i]);
        }
    }

    free(buffer);
    return 0;
}

int mfs_touch(char **command, int fd, mfs_superblock sblock, int argc, inode *curDir){
    __u32   newTime, block, grDesc;
    int     mode = 0, i, j = 0;
    inode   cur;

    newTime = time(NULL);

    if(!strcmp(command[1], "-a")){
        mode = 1;
        j = 1;
    }else if(!strcmp(command[1], "-m")){
        mode = 2;
        j = 1;
    }

    memcpy(&cur, curDir, sizeof(inode));

    for(i = 1 + j; i < argc; i++){
        if(mfs_followPath(fd, sblock, command[i], &cur, 1) != -1){
            if(!mode){
                cur.access_time = newTime;
                cur.modification_time = newTime;
            }else if(mode == 1){
                cur.access_time = newTime;
            }else{
                cur.modification_time = newTime;
            }
            grDesc = cur.node_id / (sblock.block_size * 8);
            block = grDesc / (sblock.block_size / sizeof(group_descriptor));
            grDesc = grDesc % (sblock.block_size / sizeof(group_descriptor));
            mfs_writeInode(fd, &cur, sblock, block, grDesc,
                           cur.node_id % (sblock.block_size * 8), 1);
        }else{
            fprintf(stderr, "%s not found.\n", command[i]);
        }
    }

    return 0;
}

void mfs_goUp(char *buffer){
    int i;

    i = strlen(buffer) - 2;
    while(buffer[i] != '/') i--;
    buffer[i + 1] = '\0';
}

int mfs_ls(char **command, int fd, mfs_superblock sblock, int argc, inode *curDir){
    int             aFlag = -1, rFlag = -1, lFlag = -1, uFlag = -1, dFlag = -1,
                    error = -1, argCount = 0, i, j, flags, k;
    __u32           offset, curOffset;
    char            *buffer, filename[255], **recursiveArray;
    list_root       *list;
    list_node       *curNode;
    inode           cur, reqInode;
    directory_entry entry;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_ls malloc");
        return -1;
    }

    memcpy(&cur, curDir, sizeof(inode));

    for(i = 1; i < 6; i++){
        if(i == argc) break;
        if(!strcmp(command[i], "-a")){
            argCount++;
            if(aFlag){
                aFlag = 0;
            }else{
                error = 0;
                break;
            }
        }else if(!strcmp(command[i], "-r")){
            argCount++;
            if(rFlag){
                rFlag = 0;
            }else{
                error = 0;
                break;
            }
        }else if(!strcmp(command[i], "-l")){
            argCount++;
            if(lFlag){
                lFlag = 0;
            }else{
                error = 0;
                break;
            }
        }else if(!strcmp(command[i], "-U")){
            argCount++;
            if(uFlag){
                uFlag = 0;
            }else{
                error = 0;
                break;
            }
        }else if(!strcmp(command[i], "-d")){
            argCount++;
            if(dFlag){
                dFlag = 0;
            }else{
                error = 0;
                break;
            }
        }
    }
    if(!error){
        fprintf(stderr, "Duplicate argument.\n");
        return -1;
    }

    for(i = 1 + argCount; i < argc; i++){
        if(mfs_followPath(fd, sblock, command[i], &cur, 1) != -1){
            list = mfs_listCreate();
            if(list == NULL){
                continue;
            }
            j = 0;
            while(cur.datablocks[j] != 0 && j < DATABLOCK_NUM){
                mfs_read(fd, sblock, buffer, cur.datablocks[j]);
                memcpy(&offset, buffer, 4);
                curOffset = 4;
                while(curOffset < offset){
                    memcpy(&entry, buffer + curOffset, sizeof(directory_entry));
                    if(entry.inodeptr != 0){
                        mfs_findInode(fd, sblock, entry.inodeptr, &reqInode);
                        memcpy(filename, buffer + curOffset + sizeof(directory_entry),
                               entry.name_len);
                        if((buffer[curOffset + sizeof(directory_entry)] != '.' ||
                           !aFlag) && (entry.file_type == 0 || dFlag)){
                            if(uFlag){
                                mfs_listAddNodeAB(list, reqInode, filename,
                                                  entry.name_len);
                            }else{
                                mfs_listAddNodeCT(list, reqInode, filename,
                                                  entry.name_len);
                            }
                        }
                    }
                    curOffset += entry.rec_len;
                }
                j++;
            }
            mfs_listPrint(*list, lFlag);
            if(!rFlag){
                flags = aFlag + dFlag + uFlag + lFlag + rFlag;
                flags += 5;
                recursiveArray = malloc((2 + flags) * sizeof(char*));
                recursiveArray[0] = malloc(1);
                for(k = 1; k < flags; k++){
                    recursiveArray[k] = malloc(3);
                }
                recursiveArray[1 + flags] = malloc(sblock.max_filename_size);
                curNode = list->root;
                while(curNode != NULL){
                    if(curNode->mds.mode == 0){
                        mfs_ls(recursiveArray, fd, sblock, argc, &(curNode->mds));
                    }
                }
            }
            mfs_listDestroy(list);
        }else{
            fprintf(stderr, "%s not found.\n", command[i]);
        }
    }

    for(k = 0; k < 2 + flags; k++){
        free(recursiveArray[k]);
    }
    free(recursiveArray);
    free(buffer);
    return 0;
}

int mfs_move(char ** command, int fd, mfs_superblock sblock, int argc, inode *curDir){
    int         iFlag = 1, i;
    char        c, *newFilename, *targetPath, *sourcePath;
    inode       source, sourceDir, target;

    if(command[argc - 1][0] == '/'){
        mfs_findInode(fd, sblock, 1, &target);
    }else{
        memcpy(&target, curDir, sizeof(inode));
    }

    if(!strcmp(command[1], "-i")){
        iFlag = 0;
    }

    if((!iFlag && argc == 4) || argc == 3){
        if(command[argc - 2][0] == '/'){
            mfs_findInode(fd, sblock, 1, &sourceDir);
        }else{
            memcpy(&sourceDir, curDir, sizeof(inode));
        }

        sourcePath = mfs_extractPath(command[argc - 2]);
        if(mfs_followPath(fd, sblock, sourcePath, &sourceDir, 0) == -1){
            fprintf(stderr, "Source directory does not exist.\n");
            return -1;
        }
        if(mfs_followPath(fd, sblock, command[argc - 2], &source, 1) == -1){
            fprintf(stderr, "Source does not exist.\n");
            return -1;
        }

        newFilename = mfs_extractFilename(command[argc - 1]);
        if(newFilename == NULL){
            fprintf(stderr, "No filename given.\n");
            return -1;
        }
        targetPath = mfs_extractPath(command[argc - 1]);

        if(mfs_followPath(fd, sblock, targetPath, &target, 0) == -1){
            fprintf(stderr,"%s does not exist.\n", targetPath);
            return -1;
        }
        if(!iFlag){
            printf("Move %s to %s? (y/n) ", command[argc - 2], command[argc - 1]);
            c = getcharSilent();
            while(c != 'y' && c != 'n'){
                c = getcharSilent();
            }
            putchar(c);
        }
        if(iFlag || c == 'y'){
            if(mfs_insertEntry(fd, &sblock, target, source, command[argc - 1]) != -1){
                if(mfs_clearEntry(fd, sblock, sourceDir, source) == -1){
                    fprintf(stderr, "Failed to clear entry. Possible duplicate entries\n");
                }
            }
        }
        free(sourcePath);
        free(targetPath);
        return 0;
    }else{
        if(mfs_followPath(fd, sblock, command[argc - 1], &target, 0) == -1){
            fprintf(stderr,"%s does not exist.\n", command[argc - 1]);
            return -1;
        }
    }
    for(i = 2 + iFlag; i < argc - 1; i++){
        if(command[i][0] == '/'){
            mfs_findInode(fd, sblock, 1, &sourceDir);
        }else{
            memcpy(&sourceDir, curDir, sizeof(inode));
        }

        sourcePath = mfs_extractPath(command[i]);
        if(mfs_followPath(fd, sblock, sourcePath, &sourceDir, 0) == -1){
            fprintf(stderr, "Source directory does not exist.\n");
            continue;
        }
        if(mfs_followPath(fd, sblock, command[i], &source, 1) == -1){
            fprintf(stderr, "Source does not exist.\n");
            continue;
        }
        if(!iFlag){
            printf("Move %s to %s? (y/n) ", command[i], command[argc - 1]);
            c = getcharSilent();
            while(c != 'y' && c != 'n'){
                c = getcharSilent();
            }
            putchar(c);
        }
        if(iFlag || c == 'y'){
            if(mfs_insertEntry(fd, &sblock, target, source, command[argc - 1]) != -1){
                if(mfs_clearEntry(fd, sblock, sourceDir, source) == -1){
                    fprintf(stderr, "Failed to clear entry. Possible duplicate entries\n");
                }
            }
        }
    }
    free(sourcePath);
    return 0;
}

int mfs_clearEntry(int fd, mfs_superblock sblock, inode dir, inode toClear){
    char            *buffer;
    int             i = 0;
    int             curOffset, offset;
    directory_entry entry;

    buffer = malloc(sblock.block_size);
    if(buffer == NULL){
        perror("mfs_clearEntry malloc");
        return -1;
    }

    while(dir.datablocks[i] != 0){
        if(lseek(fd, dir.datablocks[i] * sblock.block_size, SEEK_SET) == -1){
            perror("mfs_clearEntry seek");
            free(buffer);
            return -1;
        }
        if(read(fd, buffer, sblock.block_size) < sblock.block_size){
            perror("mfs_clearEntry read");
            free(buffer);
            return -1;
        }
        memcpy(&offset, buffer, 4);
        curOffset = 4;
        while(curOffset < offset){
            memcpy(&entry, buffer + curOffset, sizeof(directory_entry));
            if(entry.inodeptr == toClear.node_id){
                entry.inodeptr = 0;
                memcpy(buffer + curOffset, &entry, sizeof(directory_entry));
                if(lseek(fd, dir.datablocks[i] * sblock.block_size, SEEK_SET) == -1){
                    perror("mfs_clearEntry seek");
                    free(buffer);
                    return -1;
                }
                if(write(fd, buffer, sblock.block_size) < sblock.block_size){
                    perror("mfs_clearEntry write");
                    free(buffer);
                    return -1;
                }
                return 0;
            }
            curOffset += entry.rec_len;
        }
        i++;
    }

    return -1;
}

char* mfs_extractPath(char *buffer){
    int     len, i, flag = -1;
    char    *path;

    len = strlen(buffer);
    path = malloc(len + 1);

    i = len - 1;
    while(i >= 0){
        if(!flag){
            path[i] = buffer[i];
        }else if(buffer[i] == '/'){
            flag = 0;
            path[i] = '\0';
        }
        i--;
    }

    if(flag){
        path[0] = '.';
        path[1] = '\0';
    }

    return path;
}
