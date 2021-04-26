#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <mqueue.h>
#include "mta_crypt.h"
#include "mta_rand.h"

#define bool int
#define true 1
#define false 0
#define SERVER_QUEUE_NAME "/server_mq"
#define CLIENT_QUEUE_NAME(a) "/decryptor-%s",a
#define QUEUE_PERMISSIONS 0777
#define PASSWORD_LENGTH 16
#define KEY_LENGTH PASSWORD_LENGTH/8
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE sizeof(Message)
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10
#define NUMBER_OF_CLIENTS 3


typedef struct DataToDecryptor
{
    int passwordLength;
    int keyLength;
    int decryptorNum;
    int sizeOfEncryption;
    int timeout;
    int iterationsNum;
    char* encrypted_pass;
    char* decrypted_pass;
    char* key_used;
}DataToDecryptor;

typedef struct MqEntry
{
    char* entry_name;
    int decryptorNum;
    mqd_t mqd;
}MqEntry;


typedef struct Message{
bool connect;
bool disconnect;
char password[PASSWORD_LENGTH + 1];
int sender;
}Message;