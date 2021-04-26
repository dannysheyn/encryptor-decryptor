#include "shared.h"

typedef struct inputParams{
    int num_of_rounds;
    int num_of_decryptors;
}InputParams;


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


InputParams* parseInputParams(int argc, char*argv[])
{

    if (argc < 2)
    {
        printf("Missing arguments: num-of-decrypters, -n|--number_of_rounds(optional)\n");
        exit(1);
    }

    InputParams *inputPrams = malloc(sizeof(InputParams));
    inputPrams->num_of_rounds = 0;
    inputPrams->num_of_decryptors = atoi(argv[1]);

    if (argc > 2)
    {

        if(argc != 4)
        {
            // if more then 2 args, number of rounds have to be supplied
            printf("ERROR: Didn't supply with a number of rounds\n");
            exit(-1);
        }
        else if(strcmp(argv[2],"-n") != 0)
        {
            printf("ERROR: incorrect flag supplied, should be -n\n");
            exit(-1);
        }
        else
        {
            if(isNumber(argv[3]) == true)
            {
                inputPrams->num_of_rounds = atoi(argv[3]);
            }
            else
            {
                printf("ERROR: Number of rounds is not a number\n");
                exit(-1);
            }
        }
        
        
    }
    
    return inputPrams;
}
int main(int argc, char*argv[])
{
    
    InputParams* input = parseInputParams(argc,argv);
    pid_t pid = vfork();


    if (pid == 0)
    {
        char* args[2];
        args[0] = "./server.out";
        args[1] = NULL;
        execve(args[0], args, NULL);
    }

    for(int i=0; i< input->num_of_decryptors; i++)
    {
        if(vfork()==0)
        {
            char* args[5];
            char buff1[50] = {0};
            char buff2[50] = {0};
            args[0] = "./decryptor.out";
            sprintf(buff1,"%d", i+1);
            args[1] = buff1;
            args[2] = "-n";
            sprintf(buff2,"%d", input->num_of_rounds);
            args[3] = buff2;
            args[4] = NULL;
            for (int j = 0; j < 4; j++)
            {
                printf("%s ", args[j]);
            }
            printf("\n");            
            execve(args[0], args, NULL);
        }
    }
    
}
