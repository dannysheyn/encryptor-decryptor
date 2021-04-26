#include "shared.h"

MqEntry ClientQueue;
MqEntry ServerQueue;
char client_queue_name[255];

bool isPrintable(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (!isprint((int)buffer[i]))
        {
            return false;
        }
    }

    return true;
}

void send_decrypted_pass(char* dencrypted_pass, int dencrypted_pass_len)
{
    Message msg;
    msg.connect = false;
    msg.disconnect = false;
    msg.sender = ClientQueue.decryptorNum;
    memcpy(msg.password, dencrypted_pass, dencrypted_pass_len + 1);
    mq_send(ServerQueue.mqd, (char*)&msg , sizeof(Message), 0);
}

void get_new_password(char *out_encrypted_pass_from_server, int size_encrypted)
{
    Message msg;
    int ret = mq_receive(ClientQueue.mqd, (char *)&msg, sizeof(Message),0);
    memcpy(out_encrypted_pass_from_server, msg.password, size_encrypted);
}

void send_disconnect()
{
    Message msg;
    msg.connect = false;
    msg.disconnect = true;
    msg.sender = ClientQueue.decryptorNum;
    mq_send(ServerQueue.mqd, (char*)&msg , sizeof(Message), 0);
}

void *decryptPassword(DataToDecryptor* decryporParam)
{
    char encrypted_pass_from_server[255] = {0};
    char dycrptedPassword[255] = {0};
    char randomKeyBuffer[255] = {0};
    char last_password[255] = {0};
    int keyLen = decryporParam->keyLength;
    char *encryptedPassword = decryporParam->encrypted_pass;
    int encryptedPasswordLen = decryporParam->sizeOfEncryption;
    int numOfIterations = 0;
    int times_to_run = decryporParam->iterationsNum;
    bool iterationsNum_timer = times_to_run > 0 ? false : true;
    MTA_CRYPT_RET_STATUS cryptRetStatus;
    struct mq_attr mqAttr = {0};
    Message *msg = malloc(sizeof(Message));
    int number_decrypted = 0;
    memcpy(encrypted_pass_from_server, decryporParam->encrypted_pass, encryptedPasswordLen);

    while (iterationsNum_timer || times_to_run > 0)
    {
        

        /* Get new password from encryptor thorugh queue */
        mq_getattr(ClientQueue.mqd, &mqAttr);
        if (mqAttr.mq_curmsgs > 0)
        {
            get_new_password(encrypted_pass_from_server ,encryptedPasswordLen);
            printf("Client #%d got the new password from the server!\n", ClientQueue.decryptorNum);
        }


        MTA_get_rand_data(randomKeyBuffer, keyLen);
        randomKeyBuffer[keyLen] = '\0';

        cryptRetStatus = MTA_decrypt(randomKeyBuffer, keyLen, encrypted_pass_from_server,encryptedPasswordLen, dycrptedPassword, &encryptedPasswordLen);
        dycrptedPassword[encryptedPasswordLen] = '\0';
        if (cryptRetStatus == MTA_CRYPT_RET_OK)
        {
            if (isPrintable(dycrptedPassword, encryptedPasswordLen))
            {
                printf("Client number #%d found the decrypted password: %s , key: %s, iterations: %d\n",
                ClientQueue.decryptorNum, dycrptedPassword, randomKeyBuffer, numOfIterations);
                send_decrypted_pass(dycrptedPassword, encryptedPasswordLen);
                strcpy(encrypted_pass_from_server,"");
                numOfIterations = 0;
                mq_getattr(ClientQueue.mqd, &mqAttr);
                int ret = mq_receive(ClientQueue.mqd, (char *)msg, sizeof(Message),0);
                if (ret == -1)
                {
                    perror("");
                    printf("errno recived : %d\n", errno);
                }
                memcpy(encrypted_pass_from_server ,msg->password, encryptedPasswordLen);
                times_to_run--;
                number_decrypted++;
                continue;
            }
        }

        numOfIterations++;

        if (numOfIterations % 1000000 == 0)
        {
            printf("cannot crack password\n");
        }
    }

    send_disconnect();
    printf("After %d rounds, my job here is done\n", decryporParam->iterationsNum);
}


void send_request(bool isConnect, int sender_id)
{
    char send_buffer[255];
    Message *msg = malloc(sizeof(Message));
    msg->connect = isConnect;
    msg->disconnect = !isConnect;
    msg->sender = sender_id;
    strcpy(msg->password, "");
    mq_send(ServerQueue.mqd, (char*)msg , sizeof(Message), 0);
}


bool isNumber(const char* input)
{
    int i = 0;

    while (input[i] != '\0')
    {
        if(input[i] < '0' || input[i] > '9')
        {
            return false;
        }
        
        i++;
    }
    
    return true;
}

int get_sender_id(int argc, char const *argv[])
{
    const char *num = argv[1];
    if (isNumber(num))
    {
        return atoi(num);
    }
    else
    {
        printf("first param must be a number\n");
        exit(1);
    }
    
}

int get_nums_to_decrypt(int argc, const char** argv)
{
    int ret_num_of_iterations = 0;

    if(argc > 2 && argc!=4)
    {
        // if more then 2 args, number of rounds have to be supplied
        printf("ERROR: Not supplied number of rounds\n");
        exit(-1);
    }

    for (int i = 0; i < argc - 1; i++)
    {
        if (strcmp(argv[i],"-n") == 0)
        {
            if(isNumber(argv[i+1]) == true)
            {
                ret_num_of_iterations = atoi(argv[i+1]);
            }
            else
            {
                printf("ERROR: Pram after -n was not a number\n");
                exit(-1);
            }
        }
        
    }

    

    return ret_num_of_iterations;
}


void init_data(DataToDecryptor *pass_data,int decryptorNum, int iteration_num)
{
    pass_data->keyLength = KEY_LENGTH;
    pass_data->passwordLength = PASSWORD_LENGTH;
    pass_data->sizeOfEncryption = PASSWORD_LENGTH;
    pass_data->decryptorNum = decryptorNum;
    pass_data->timeout = 0;
    pass_data->decrypted_pass = NULL;
    pass_data->encrypted_pass = NULL;
    pass_data->key_used = NULL;
    pass_data->iterationsNum = iteration_num;
}


int main(int argc, char const *argv[])
{
    mqd_t qd_server, qd_client;   // queue descriptors
    // create the client queue for receiving messages from server
    ClientQueue.decryptorNum = get_sender_id(argc, argv);
    int times_to_decrypt = get_nums_to_decrypt(argc, argv);
    sprintf (client_queue_name, CLIENT_QUEUE_NAME(argv[1]));
    ClientQueue.entry_name = client_queue_name;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    mq_unlink(ClientQueue.entry_name);
    ClientQueue.mqd = mq_open(ClientQueue.entry_name, O_RDONLY | O_CREAT , QUEUE_PERMISSIONS, &attr);
    ServerQueue.entry_name = SERVER_QUEUE_NAME;
    ServerQueue.mqd = mq_open (SERVER_QUEUE_NAME, O_WRONLY);
    send_request(true, ClientQueue.decryptorNum); // connect to server
    DataToDecryptor pass_data;
    init_data(&pass_data, ClientQueue.decryptorNum, times_to_decrypt);
    Message *msg = malloc(sizeof(Message));
    mq_receive(ClientQueue.mqd, (char*)msg, sizeof(Message),0);
    pass_data.encrypted_pass = strdup(msg->password);
    decryptPassword(&pass_data);
    mq_close(ClientQueue.mqd);
    mq_unlink(ClientQueue.entry_name);
    printf("Client #%d: bye\n", ClientQueue.decryptorNum);
    exit(0);
}