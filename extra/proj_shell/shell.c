#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LEN 2097152
#define MAX_TOK 50

void runcmd(char *str);
void getcmd(char *res);

int main(int argc, char *argv[])
{
    char buf[MAX_LEN], *result;
    switch (argc)
    {
    case 1:
    {
        while (1)
        {
            printf("prompt> ");
            if ((result = fgets(buf, MAX_LEN, stdin)) != NULL)
            {
                if (strlen(result) == 5 && result[0] == 'q' && result[1] == 'u' && result[2] == 'i' && result[3] == 't') break;
                else getcmd(result);
            }
            else exit(0);
        }
        break;
    }
    case 2:
    {
        FILE *fp = NULL;
        fp = fopen(argv[1], "r");
        if(fp==NULL)
        {
            printf("File not found\n");
            exit(0);
        }
        while ((result = fgets(buf, MAX_LEN, fp)) != NULL){
            printf("%s",result);
            if (strlen(result) == 5 && result[0] == 'q' && result[1] == 'u' && result[2] == 'i' && result[3] == 't') break;
            getcmd(result);
        } 
	fclose(fp);
	break;
    }
    default:
    {
        printf("This program can only run one batch file\n");
        break;
    }
    }
}
void runcmd(char *str)
{
    char *token[MAX_TOK];
    int i = -1;
    token[++i] = strtok(str, " ");
    while (token[i] != NULL)
    {
        token[++i] = strtok(NULL, " ");
    }
    token[i] = NULL;
    if(execvp(token[0], token)<0){
        for(int j = 0 ; j < i ; j++) printf("%s ",token[j]);
    }
    printf(": command not found\n");
    exit(0);
}
void getcmd(char *res)
{
    res[strlen(res) - 1] = '\0';
    char *token = strtok(res, ";");
    int n = 0;
    while (token != NULL)
    {
        int pid = fork();
        n++;
        if (pid == 0)
        {
            runcmd(token);
            exit(0);
        }
        else if (pid < 0) printf("fork error\n");
        else token = strtok(NULL, ";");
    }
    int status;
    while (n > 0)
    {
        wait(&status);
        --n;
    }
}
