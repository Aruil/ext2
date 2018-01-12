#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include "login.h"
#include "commands.h"

int main(int argc, char *argv[]){
    char            *command, **spltCommand, fileSystem[BUFFER_SIZE],
                    path[BUFFER_SIZE] = "/", *username = NULL;
    int             i = 0, openedFS = -1, wordCount, commandType, userID, flag;
    int             fd;
    mfs_superblock  sblock;
    inode           currentFolder;

    command = malloc(COMMAND_SIZE * sizeof(char));
    if(command == NULL){
        perror("command malloc");
        exit(1);
    }

    if(!loginMenu(&username, &userID)){
        while(1){
            if(openedFS) printf("%s@(nofilesystem) ~ $ ", username);
            else printf("%s@%s %s $ ", username, fileSystem, path);
            wordCount = readCommand(command);
            if(wordCount == -1){
                printf("Exiting...\n");
                break;
            }
            spltCommand = splitCommand(wordCount, command, &commandType);
            if(commandType != WORKWITH && commandType != CREATE && openedFS){
                fprintf(stderr, "No filesystem open to work with.\n");
            }else{
                switch(commandType){
                    case WORKWITH:
                        openedFS = mfs_workwith(spltCommand, &sblock, &fd,
                                                fileSystem, &currentFolder);
                        break;
                    case LS:
                        mfs_ls(spltCommand, fd, sblock, wordCount, &currentFolder);
                        break;
                    case CD:
                        flag = mfs_followPath(fd, sblock, spltCommand[1],
                                              &currentFolder, 0);
                        if(!flag && !strcmp(spltCommand[1], "..")){
                            if(strcmp(path, "/")){
                                mfs_goUp(path);
                            }
                        }else if(!flag && strcmp(spltCommand[1], ".")){
                            if(spltCommand[1][0] == '/') strcpy(path, spltCommand[1]);
                            else strcat(path, spltCommand[1]);
                            if(path[strlen(path) - 1] != '/') strcat(path, "/");
                        }
                        break;
                    case PWD:
                        printf("%s\n", path);
                        break;
                    case CP:
                        break;
                    case MV:
                        break;
                    case RM:
                        break;
                    case MKDIR:
                        mfs_mkdir(fd, &sblock, spltCommand, currentFolder, wordCount);
                        mfs_findInode(fd, sblock, currentFolder.node_id, &currentFolder);
                        break;
                    case TOUCH:
                        mfs_touch(spltCommand, fd, sblock, wordCount, &currentFolder);
                        break;
                    case IMPORT:
                        flag = mfs_import(spltCommand, fd, &sblock, &currentFolder,
                                          wordCount);
                        if(!flag) mfs_findInode(fd, sblock, currentFolder.node_id,
                                                &currentFolder);
                        break;
                    case EXPORT:
                        mfs_export(spltCommand, fd, sblock, &currentFolder, wordCount);
                        break;
                    case CAT:
                        break;
                    case CREATE:
                        mfs_create(spltCommand, wordCount);
                        break;
                    default:
                        continue;
                }
            }
            for(i = 0; i < wordCount; i++){
                free(spltCommand[i]);
            }
            free(spltCommand);
        }
    }

    free(command);
    exit(0);
}