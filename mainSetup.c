#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct node {
    char* val;
    struct node * next;
} node_t;



#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */

        i,     /* loop index for accessing inputBuffer array */
        start, /* index where beginning of next command parameter is */
        ct;    /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
    However, if this occurs, errno is set to EINTR. We can check this  value
    and disregard the -1 value */
    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }

    printf(">>%s<<", inputBuffer);
    for (i = 0; i < length; i++)
    {
        /* examine every character in the inputBuffer */

        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

    /*for (i = 0; i < ct; i++)
    {
        printf("args %d = %s\n", i, args[i]);

        char path[] = "/bin/";
        strcat(path, args[i]);
        char* arr[] = {"ls", "-l", "-R", "-a", NULL};
        execv(path, arr);
    }*/


} /* end of setup routine */

int isFileExists(const char *path)
{
    // Try to open file
    FILE *fptr = fopen(path, "r");

    // If file does not exists
    if (fptr == NULL)
        return 0;

    // File exists hence close file and return true.

    fclose(fptr);

    return 1;
}



char** findPath(char *args[])
{
    char *env = getenv("PATH");
    char *str = (char*)malloc(sizeof(char)*1000);
    strcpy(str, env); 
    char delim[] = ":";

    char *ptr = strtok(str, delim);
    char **pathsArray;
    pathsArray = (char**)malloc(sizeof(char*)*100);
    int j = 0;

    while(ptr != NULL)
    {
        pathsArray[j] = (char *)malloc(sizeof(char)*100);
        strcpy(pathsArray[j], ptr);
        j++;

        ptr = strtok(NULL, delim);
    }

    int i;
    int k;
    int a = 0;

    char **temp;
    char **realPaths;

    temp = (char **)malloc(sizeof(char *) * 100);
    realPaths = (char **)malloc(sizeof(char *) * 100);
    for (k = 0; k < j; k++)
    {
        temp[k] = (char *)malloc(sizeof(char) * 100);
        realPaths[k] = (char *)malloc(sizeof(char) * 100);

        strcpy(temp[k], pathsArray[k]);
        strcat(temp[k], "/");
        strcat(temp[k], args[0]);
        printf("\n%s", temp[k]);

        if (isFileExists(temp[k]))
        {
            printf("----found ---\n");
            strcpy(realPaths[a], temp[k]);
            a++;
        }
    }
    printf("\n");
    free(temp);

    return realPaths;
}

/*
char** splitByAmpersandOrSemiColumn(char *args[]){

    char** newArgs;
    newArgs = (char**)malloc(sizeof(char*)*32);

    int count = 0;
    while (args[count] != NULL){
        count++;
    }

    int i;
    for(i = 0; i < count; i++ ){
        newArgs[i] = (char *)malloc(sizeof(char)*128);
        newArgs[i] = args[i];
        if(strcmp(args[i], "&"))
            break;
    }
    return newArgs;
}*/

void push(node_t * head, char* val) {
    if(head == NULL){
        head->val = (char*)malloc(sizeof(char)*128);
        strcpy(head->val, val);
        head->next = NULL;
    }

    node_t * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = malloc(sizeof(node_t));
    current->next->val = (char*)malloc(sizeof(char)*128);
    strcpy(current->next->val, val);
    current->next->next = NULL;
}

void print_list(node_t * head) {
    node_t * current = head;

    while (current != NULL) {
        printf("*******%s\n", current->val);
        current = current->next;
    }
}

void pop(node_t ** head) {
    node_t * next_node = NULL;

    if (*head == NULL) {
        return;
    }

    next_node = (*head)->next;
    free(*head);
    *head = next_node;
}

int getLength(node_t * head){
    int length = 0;
    node_t * current = head;
    while(current->next != NULL){
        length++;
        current = current->next;
    }
    return length;
}

int main(void)
{
    char inputBuffer[MAX_LINE];   /*buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    char **paths;
    int status;

    node_t *head = NULL;
    head = malloc(sizeof(node_t));

    while (1)
    {

        background = 0;
        printf("myshell: ");
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);


        char* mergedArgs = (char*)malloc(sizeof(char)* 128);
        strcpy(mergedArgs, "");
        int count = 0;
        while (args[count] != NULL){
            strcat(mergedArgs, args[count]);
            strcat(mergedArgs, " ");
            count++;
        }


        
        push(head, mergedArgs);
        printf("%d", getLength(head));
        if(getLength(head) == 3)
            pop(&head);
        

        print_list(head);

        paths = findPath(args);

        //printf("---%s\n", paths[0]);

        

        if(background == 1)
            args[count-1] = NULL;

        pid_t pid;
        if ((pid = fork()) == -1)
            perror("fork error");
        else if (pid == 0){
            execv(paths[0], args);
            printf("Return not expected. Must be an execv error.n");
        }
        else{
            if(background == 1){
                printf("i am parent and waiting");
                wait(&status);
            }
        }

        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
        				(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
    }
}
