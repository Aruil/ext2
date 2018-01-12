#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

list_root* mfs_listCreate(){
    list_root   *root;

    root = malloc(sizeof(list_root));
    if(root == NULL){
        perror("mfs_listCreate malloc");
    }else{
        root->size = 0;
        root->root = NULL;
    }

    return root;
}

int mfs_listAddNodeAB(list_root *root, inode toAdd, char *filename, int name_len){
    list_node   *node, *cur;

    node = malloc(sizeof(list_node));
    if(node == NULL){
        perror("mfs_listAddNode malloc");
        return -1;
    }

    memcpy(&(node->mds), &toAdd, sizeof(inode));
    node->filename = malloc(name_len + 1);
    if(node->filename == NULL){
        perror("mfs_listAddNode malloc");
        free(node);
        return -1;
    }
    strncpy(node->filename, filename, name_len);
    node->filename[name_len] = '\0';

    if(root->root == NULL){
        node->previous = NULL;
        node->next = NULL;
        root->root = node;
    }else{
        cur = root->root;
        while(cur != NULL){
            if(strcmp(node->filename, cur->filename) < 0){
                root->size++;
                node->previous = cur->previous;
                if(cur->previous == NULL){
                    root->root = node;
                }else{
                    cur->previous->next = node;
                }
                node->next = cur;
                cur->previous = node;
                break;
            }else if(cur->next == NULL){
                node->previous = cur;
                node->next = NULL;
                cur->next = node;
                break;
            }
            cur = cur->next;
        }
    }

    return 0;
}

int mfs_listAddNodeCT(list_root *root, inode toAdd, char *filename, int name_len){
    list_node   *node, *cur;

    node = malloc(sizeof(list_node));
    if(node == NULL){
        perror("mfs_listAddNode malloc");
        return -1;
    }

    memcpy(&(node->mds), &toAdd, sizeof(inode));
    node->filename = malloc(name_len + 1);
    if(node->filename == NULL){
        perror("mfs_listAddNode malloc");
        free(node);
        return -1;
    }
    strncpy(node->filename, filename, name_len);
    node->filename[name_len] = '\0';

    if(root->root == NULL){
        node->previous = NULL;
        node->next = NULL;
        root->root = node;
    }else{
        cur = root->root;
        while(cur != NULL){
            if(node->mds.creation_time < cur->mds.creation_time){
                root->size++;
                node->previous = cur->previous;
                if(cur->previous == NULL){
                    root->root = node;
                }else{
                    cur->previous->next = node;
                }
                node->next = cur;
                cur->previous = node;
                break;
            }else if(cur->next == NULL){
                node->previous = cur;
                node->next = NULL;
                cur->next = node;
                break;
            }
            cur = cur->next;
        }
    }

    return 0;
}

void mfs_listPrint(list_root root, int lFlag){
    list_node   *cur;

    cur = root.root;
    while(cur != NULL){
        if(lFlag){
            printf("%s\n", cur->filename);
        }else{
            printf("%s ct: %u at: %u mt: %u %llu", cur->filename,
                   cur->mds.creation_time, cur->mds.access_time,
                   cur->mds.modification_time, cur->mds.file_size);
        }
        cur = cur->next;
    }
}

void mfs_listDestroy(list_root *root){
    list_node   *cur, *toDelete;

    if(root != NULL){
        cur = root->root;
        while(cur != NULL){
            toDelete = cur;
            cur = cur->next;
            free(toDelete->filename);
            free(toDelete);
        }
        free(root);
        root = NULL;
    }
}