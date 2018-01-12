#ifndef _LOGIN_H_
#define _LOGIN_H_

#define BUFFER_SIZE 256
#define BACKSPACE 0x7f
#define ACCOUNTS_FILE "accounts.bin"

int getcharSilent();

char* readUsername();

char* readPassword();

int createAccount(int accountsFile);

int findAccount(char *username, char *password, int accountsFile);

int deleteAccount(int accountsFile);

int loginMenu(char** str, int *userID);

void encryptDecrypt(char* string);

#endif