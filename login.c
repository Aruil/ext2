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
#include "login.h"

int getcharSilent(){
    int ch;
    struct termios oldt, newt;

    /* Retrieve old terminal settings */
    tcgetattr(STDIN_FILENO, &oldt);

    /* Disable canonical input mode, and input character echoing. */
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );

    /* Set new terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    /* Read next character, and then switch to old terminal settings. */
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

char* readUsername(){
    int     c, i = 0;
    char    *username;

    username = malloc(BUFFER_SIZE * sizeof(char));
    if(username == NULL){
        perror("readUsername malloc");
        return NULL;
    }

    while((c = getcharSilent()) != EOF){
        if(i == BUFFER_SIZE - 1){
            fprintf(stderr, "\nMaximum length reached.");
        }else{
            if(isalpha(c)){
                username[i] = c;
                i++;
                putchar(c);
            }else if(c == '\n' || i == BUFFER_SIZE - 1){
                username[i] = '\0';
                if(c == '\n'){
                    putchar(c);
                    return username;
                }
            }else if(c == BACKSPACE){
                if(i){
                    i--;
                    printf("\b \b");
                }
            }else if(c == VEOF){
                free(username);
                return NULL;
            }
        }
    }

    return NULL;
}

char* readPassword(){
    int     c, i = 0;
    char    *password;

    password = malloc(BUFFER_SIZE * sizeof(char));
    if(password == NULL){
        perror("readPassword malloc");
        return NULL;
    }

    while((c = getcharSilent()) != EOF){
        if(i == BUFFER_SIZE - 1){
            fprintf(stderr, "\nMaximum length reached.");
        }else{
            if(c == BACKSPACE){
                if(i) i--;
            }else if(c == '\n' || i == BUFFER_SIZE - 1){
                password[i] = '\0';
                if(c == '\n'){
                    putchar('\n');
                    return password;
                }
            }else if(c == VEOF){
                free(password);
                return NULL;
            }else{
                password[i] = c;
                i++;
            }
        }
    }

    return NULL;
}

int createAccount(int accountsFile){
    int     pos;
    char    *username, *password;
    off_t   seek;
    ssize_t wr = -1;

    printf("Please enter the desired username(only alphanumeric characters"
           " allowed, maximum length %d):\n", BUFFER_SIZE - 1);
    username = readUsername();
    if(username == NULL) return -2;
    if(findAccount(username, NULL, accountsFile)) return -1;
    printf("Please enter the desired password(maximum length %d):\n",
           BUFFER_SIZE - 1);
    password = readPassword();
    if(password != NULL){
        pos = findAccount("", NULL, accountsFile);
        if(pos){
            seek = lseek(accountsFile, (pos - 1) * 2 * BUFFER_SIZE, SEEK_SET);
        }else{
            seek = lseek(accountsFile, 0, SEEK_END);
        }
        if(seek == -1){
            perror("createAccount seek");
        }else{
            encryptDecrypt(username);
            wr = write(accountsFile, username, BUFFER_SIZE);
            if(wr == -1){
                perror("createAccount write");
            }else{
                encryptDecrypt(password);
                wr = write(accountsFile, password, BUFFER_SIZE);
                if(wr == -1){
                    perror("createAccount write");
                }
            }
        }
    }
    free(username);
    if(password != NULL) free(password);
    if(password != NULL && wr != -1){
        return 0;
    }else if(password != NULL){
        return -1;
    }else{
        return -2;
    }
}

int findAccount(char *username, char *password, int accountsFile){
    int     pos = 1;
    char    temp[BUFFER_SIZE];
    off_t   seek;
    ssize_t rd;

    seek = lseek(accountsFile, 0, SEEK_SET);
    if(seek == -1){
        perror("findAccount seek");
        return 0;
    }
    while(read(accountsFile, temp, BUFFER_SIZE) > 0){
        encryptDecrypt(temp);
        if(!strcmp(temp, username)){
            if(password == NULL){
                fprintf(stderr, "Account already exists.\n");
                return pos;
            }else{
                rd = read(accountsFile, temp, BUFFER_SIZE);
                if(rd == -1){
                    perror("findAccount read");
                    return 0;
                }
                encryptDecrypt(temp);
                if(!strcmp(temp, password)){
                    return pos;
                }else{
                    fprintf(stderr, "Wrong password.\n");
                    return 0;
                }
            }
        }
        seek = lseek(accountsFile, BUFFER_SIZE, SEEK_CUR);
        if(seek == -1){
            perror("findAccount seek");
            return 0;
        }
        pos++;
    }
    if(password != NULL) fprintf(stderr, "Account not found.\n");
    return 0;
}

int deleteAccount(int accountsFile){
    int     pos;
    char    *username, *password, temp[BUFFER_SIZE] = "";
    off_t   seek;
    ssize_t wr = -1;

    printf("Please enter the username of the account to be deleted:\n");
    username = readUsername();
    if(username == NULL) return -2;
    printf("Please enter the password:\n");
    password = readPassword();
    if(password != NULL){
        pos = findAccount(username, password, accountsFile);
        if(pos){
            seek = lseek(accountsFile, (pos - 1) * 2 * BUFFER_SIZE, SEEK_SET);
            if(seek == -1){
                perror("deleteAccount seek");
            }else{
                wr = write(accountsFile, temp, BUFFER_SIZE);
                if(wr == -1){
                    perror("deleteAccount write");
                }
            }
        }
    }
    free(username);
    if(password != NULL) free(password);
    else return -2;
    return (int) wr;
}

int loginMenu(char** str, int *userID){
    int     choice;
    int     usernameFlag = -1, flag = -1;
    int     accountsFile;
    char    *password = NULL, *username = NULL;

    accountsFile = open(ACCOUNTS_FILE, O_RDWR | O_CREAT, 0666);
    if(accountsFile == -1){
        perror("Accountsfile open");
        return -1;
    }

    while(1){
        printf("Please enter your choice ((l)ogin, (n)ew account, (e)xit, "
               "(d)elete account):\n");
        choice = getcharSilent();
        putchar(choice);
        putchar('\n');
        switch(choice){
            case 68:
            case 100:
                flag = deleteAccount(accountsFile);
                if(flag == -1){
                    fprintf(stderr, "Failed to delete account.\n");
                }else if(flag > 0){
                    fprintf(stderr, "Account successfully deleted.\n");
                }
                flag = -1;
                break;
            case 69:
            case 101:
                printf("Exiting...\n");
                close(accountsFile);
                return -1;
                break;
            case 76:
            case 108:
                while(flag){
                    if(usernameFlag) printf("Please enter your username:\n");
                    else printf("Please enter your password:\n");
                    if(usernameFlag){
                        username = readUsername();
                        if(username == NULL) break;
                        usernameFlag = 0;
                    }else{
                        password = readPassword();
                        usernameFlag = -1;
                        if(password == NULL){
                            free(username);
                            username = NULL;
                            break;
                        }
                        flag = findAccount(username, password, accountsFile);
                        if(flag){
                            printf("Welcome back %s.\n", username);
                            close(accountsFile);
                            free(password);
                            *str = username;
                            *userID = flag;
                            return 0;
                        }else{
                            flag = -1;
                            free(password);
                            free(username);
                        }
                    }
                }
                break;
            case 78:
            case 110:
                while(flag){
                    flag = createAccount(accountsFile);
                    if(flag == -1){
                        fprintf(stderr, "Failed to create new account.\n");
                    }else if(flag == -2){
                        break;
                    }else{
                        fprintf(stderr, "Account successfully created.\n");
                    }
                }
                flag = -1;
                break;
            default:
                continue;
        }
        choice = 0;
    }
}

void encryptDecrypt(char* string){
    char    key[3] = {'Z', 'K', 'C'};
    int     i;

    for(i = 0; i < BUFFER_SIZE; i++){
        string[i] = string[i] ^ key[i % (sizeof(key) / sizeof(char))];
    }
}