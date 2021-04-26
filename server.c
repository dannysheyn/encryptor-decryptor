#include "shared.h"


MqEntry Active_Connections[255];
int AC_Size = 0; // Active_Connections size
mqd_t Server;
int GOT_CONNECTION = 0;
int GOT_PASSWORD = 1;
int GOT_DISCONNECT = 2;

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


void remove_from_client_list(int sender)
{
    for (int i = 0; i < AC_Size; i++)
        if (Active_Connections[i].decryptorNum == sender)
            for (int j = i; j < AC_Size -1; j++)
                Active_Connections[j] = Active_Connections[j + 1];
    printf("Server: bye bye client #%d\n", sender);
    AC_Size--;
}

int extract_info_from_buffer(Message *msg)
{
   if(msg->connect == true)
   { // new connection
        char buff[255];
        sprintf(buff , "/decryptor-%d", msg->sender);
        Active_Connections[AC_Size].entry_name = strdup(buff);
        Active_Connections[AC_Size].mqd = mq_open(Active_Connections[AC_Size].entry_name, O_WRONLY);
        Active_Connections[AC_Size].decryptorNum = msg->sender;
        AC_Size++;
        return GOT_CONNECTION;
   }
    else if (msg->disconnect == true)
    {
        remove_from_client_list(msg->sender);
        return GOT_DISCONNECT;
    }

   else
   { // decryptor sent a decrypted password
       return GOT_PASSWORD;
   }
}

void getPrintablePassword(char* passwordBuffer, int bufferSize)
{
    char c;

    for (int i = 0; i < bufferSize; i++)
    {
        do
        {
            c = MTA_get_rand_char();

        } while (!isprint((int)c));

        passwordBuffer[i] = c;
    }
}

int get_client_number(char* queue_name)
{
    for (int i = 0; i < AC_Size; i++)
    {
        if (strcmp(queue_name, Active_Connections[i].entry_name) == 0)
        {
            return i + 1;
        }
    }
    
    return -1;
}


bool valitade_recevied_password(char* decryptedPass, char* orginal_decrypted, int sender)
{
    if (strcmp(decryptedPass, orginal_decrypted) == 0)
    {
        printf("Password decrypted successfully by client #%d, recieved %s, is %s \n", sender, decryptedPass, orginal_decrypted);
        return true;
    }
    else
    {
        printf("Wrong password recieved from client #%d %s , should be %s \n", sender, decryptedPass, orginal_decrypted);  
        return false;
    }
    
}


void send_encrypted_msg(char* encrypted_pass, int encrypted_pas_len)
{
    mqd_t client;
    Message msg;
    int ret_value;
    memcpy(msg.password, encrypted_pass, encrypted_pas_len);
    for (int i = 0; i < AC_Size; i++)
    {
        ret_value = mq_send(Active_Connections[i].mqd, (char*)&msg, sizeof(Message), 0);
        if (ret_value == -1)
        {
           remove_from_client_list(Active_Connections[i].decryptorNum);
           printf("Failed to send a msg to client #%d and threfore was removed from the client list\n", Active_Connections[i].decryptorNum); 
        }
    }
}

void getRandomPassword(char *password, int passwordLength, int keyLen)
{
    if (keyLen > 1)
    {
            getPrintablePassword(password, passwordLength);
    }
    else
    {
            do
            {
                MTA_get_rand_data(password, passwordLength);

            } while (!isPrintable(password, passwordLength));
    }
}


void encryptPassword(DataToDecryptor* data)
{
    unsigned int passwordLength = data->passwordLength;
    unsigned int keyLength = passwordLength/8;
    int timeout = data->timeout;
    char encryptedPassword[255] = {0};
    char originalPassword[255] = {0};
    char encriptionKey[255] = {0};
    unsigned int encryptedPasswordLength = PASSWORD_LENGTH;
    MTA_CRYPT_RET_STATUS decryptRetType;
    int num_password_decrypted = 0;
    int ret_status;
    bool recived_password;
    int ret_value;
    Message *msg = malloc(sizeof(Message));

    while (true)
    {
        recived_password = false;
        getRandomPassword(originalPassword, passwordLength, keyLength);
        MTA_get_rand_data(encriptionKey, keyLength);

        decryptRetType = MTA_encrypt(encriptionKey, keyLength, originalPassword, passwordLength, encryptedPassword, &encryptedPasswordLength);
        if (decryptRetType == MTA_CRYPT_RET_OK)
        {
            fflush(stdout);
            printf("New password generated: %s , key: %s, After encryption: %s\n",originalPassword, encriptionKey, encryptedPassword);            
            /* send pass in mq */
            send_encrypted_msg(encryptedPassword, encryptedPasswordLength);

            do
            {
                ret_value = mq_receive(Server, (char*)msg, sizeof(Message), NULL);
                if(ret_value == -1)
                {
                    perror("");
                    printf("errno: %d\n", errno);
                }
                ret_status = extract_info_from_buffer(msg);

                if (ret_status == GOT_CONNECTION)
                {
                    // send current encrypted password to the new connection
                    Message msg;
                    memcpy(msg.password, encryptedPassword, passwordLength);
                    mq_send(Active_Connections[AC_Size -1].mqd, (char*)&msg, sizeof(Message), 0);
                    printf("sent connection to: %s\n", Active_Connections[AC_Size -1].entry_name);
                }

                else if (ret_status != GOT_DISCONNECT)
                {
                    // validate new password
                    recived_password = valitade_recevied_password(msg->password, originalPassword, msg->sender);
                    if (recived_password == false)
                    {
                        // in case the password is wrong we are sending it again
                        // because decryptor is awaiting for it
                        // the reason behind this, is to not let the decryptor
                        // decrypt old passwords 
                        send_encrypted_msg(encryptedPassword, encryptedPasswordLength);
                    }
                }
                
            } while (!recived_password);

        }
        else
        {
            printf("Error while trying to encrypt password %s, status is %d\n",originalPassword, decryptRetType);
            continue;
        }
    }
  
}


bool isNumber(char* input)
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



int main(int argc, char*argv[])
{
    int rc = mq_unlink(SERVER_QUEUE_NAME);
    int max_buffer = 255;
    srand(time(NULL));
    mqd_t qd_server, qd_client;   // queue descriptors
    
    /* initialize the queue attributes */

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10; //MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;


    DataToDecryptor pass_data;
    pass_data.keyLength = KEY_LENGTH;
    pass_data.passwordLength = PASSWORD_LENGTH;
    pass_data.sizeOfEncryption = PASSWORD_LENGTH;
    pass_data.decryptorNum = -1;
    pass_data.timeout = 0;
    pass_data.decrypted_pass = NULL;
    pass_data.encrypted_pass = NULL;
    pass_data.key_used = NULL;
    unlink(SERVER_QUEUE_NAME);
    Server = mq_open(SERVER_QUEUE_NAME, O_RDWR | O_CREAT, 0666 , &attr);
    if (Server == -1)
    {
        perror("");
        printf("problem while opening server mq code: %d\n", errno);
    }
    encryptPassword(&pass_data);
}