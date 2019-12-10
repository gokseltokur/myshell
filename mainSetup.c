#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

//creates commands linked list
typedef struct node {
    char* val;
    struct node * next;
} node_t;

typedef struct background_queue {
    pid_t pid;
    char* command;
    struct background_queue* next;
} backgroundQueue;

backgroundQueue* backgroundQ = NULL;
int isThereAnyForegroundProcess = 0;
int currentForegroundProcess;


/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background) {
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
    if (length == 0){
        printf("^d was entered, end of user command stream");
        exit(0); /* ^d was entered, end of user command stream */
    }
    
    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
    However, if this occurs, errno is set to EINTR. We can check this  value
    and disregard the -1 value */
    if ((length < 0) && (errno != EINTR)) {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }
    printf(">>%s<<\n", inputBuffer);
    for (i = 0; i < length; i++) {
        /* examine every character in the inputBuffer */

        switch (inputBuffer[i]) {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1) {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1) {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&') {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

} /* end of setup routine */


//returns 1 if there is such a file
int isFileExists(const char *path) {
    // Try to open file
    FILE *fptr = fopen(path, "r");

    // If file does not exists
    if (fptr == NULL)
        return 0;

    // File exists hence close file and return true.
    fclose(fptr);

    return 1;
}

//finds the path of the executable
char** findPath(char *args[]) {
    //splits path variable by : and copies into a paths array
    char *env = getenv("PATH");
    char *str = (char*)malloc(sizeof(char)*1000);
    strcpy(str, env);
    char delim[] = ":";

    char *ptr = strtok(str, delim);
    char **pathsArray;
    pathsArray = (char**)malloc(sizeof(char*)*100);
    int j = 0;

    while(ptr != NULL) {
        pathsArray[j] = (char *)malloc(sizeof(char)*100);
        strcpy(pathsArray[j], ptr);
        j++;

        ptr = strtok(NULL, delim);
    }

    int a;

    if(!strcmp(args[0], "path")) {
        if (args[1] == NULL){
            for (a = 0; a < j; a++)
              printf("\n%s", pathsArray[a]);
        }
        else if (!strcmp(args[1], "-")){
            for (a = 0; a < j ; a++){
              if(!strcmp(args[2], pathsArray[a])){
                pathsArray[a] = NULL;
                j--;
              }
              printf("\n%s", pathsArray[a]);
            }
        }
        else if (!strcmp(args[1], "+")){
            pathsArray[j++] = args[2];
            for (a = 0; a < j; a++)
              printf("\n%s", pathsArray[a]);
        }
        return pathsArray;
    }
    else {

    int k;
    int a = 0;

    char **temp;
    char **realPaths;

    temp = (char **)malloc(sizeof(char *) * 100);
    realPaths = (char **)malloc(sizeof(char *) * 100);
    for (k = 0; k < j; k++) {
        temp[k] = (char *)malloc(sizeof(char) * 100);
        realPaths[k] = (char *)malloc(sizeof(char) * 100);

        strcpy(temp[k], pathsArray[k]);
        strcat(temp[k], "/");
        strcat(temp[k], args[0]);
        //printf("\n%s", temp[k]);

        if (isFileExists(temp[k])) {
            //printf("----found ----");
            strcpy(realPaths[a], temp[k]);
            a++;
        }
    }
    //printf("\n");
    free(temp);
    return realPaths;
    }
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

//adds a new command to the list
void push(node_t * head, char* val) {
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

//prints the linked list from the last added
void reverse_display(node_t * head) {
    if(head) {
        reverse_display(head->next);
        printf("%s\n ", head->val);
    }
}

//deletes the first executed command from the list
void pop(node_t ** head) {
    node_t * next_node;

    if (*head == NULL) {
        return;
    }

    next_node = (*head)->next;
    free(*head);
    *head = next_node;
}

//returns the command from the list with the given index
char* GetNth(node_t* head, int index) {
    node_t* current = head;

    int count = 0;
    while (current != NULL) {
        if (count == index)
            return current->val;
        count++;
        current = current->next;
    }
    return NULL;
}

//calculates the length of the linked list
int getLength(node_t * head) {
    int length = 0;
    node_t * current = head;
    while(current->next != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

// ^Z - Stop the currently running foreground process, as well as any descendants of that
// process (e.g., any child processes that it forked). If there is no foreground process, then the
// signal should have no effect.
static void signalHandler() {
    int status;
    // If there is a foreground process
    if(isThereAnyForegroundProcess) {
        // kill current process
        kill(currentForegroundProcess, 0);
        if(errno == ESRCH) {
            fprintf(stderr, "\nProcess %d not found\n", currentForegroundProcess);
            isThereAnyForegroundProcess = 0;
            printf("myshell: ");
            fflush(stdout);
        }
        else{ // if there is still foreground process
            kill(currentForegroundProcess, SIGKILL);
            waitpid(-currentForegroundProcess, &status, WNOHANG);
            printf("\n");
            isThereAnyForegroundProcess = 0;
        }
    }
    // If there is no foreground process
    else{
        printf("\nmyshell: ");
        fflush(stdout);
    }
}

void childHandler() {
    int status;
    //union wait wst;
    pid_t pid;

    while(1) {
        // Get pid of terminating process
        pid = wait3(&status, WNOHANG, NULL);
        if (pid == 0){
            return;
        }
        else if (pid == -1){
            return;
        }
        // With child process pid
        else {
            // Kick that process from the background process queue because it's not running anymore
            // Search for terminated process in the queue
            // If it's found in the head of queue
            if(backgroundQ != NULL && backgroundQ->pid == pid) {
                // Remove process from the queue
                backgroundQueue *killedProcess = backgroundQ;
                backgroundQ = killedProcess->next;
                killedProcess->next = NULL;
                free(killedProcess);
                return;
            }
            // If it's not on the head of queue
            else if(backgroundQ != NULL) {
                backgroundQueue* iter = backgroundQ;
                // Find process in the queue
                while(iter->next != NULL) {
                    if(iter->next->pid == pid) {
                        // after finding, kick it from the queue
                        backgroundQueue *killedProcess = iter->next;
                        killedProcess->next = iter->next;
                        killedProcess->next = NULL;
                        free(killedProcess);
                        return;
                    }
                }
            }
        }
    }
}

void enqueueBackgroundQ(pid_t pid, char* command){
    if(backgroundQ == NULL){
        backgroundQ = (backgroundQueue*)malloc(sizeof(backgroundQueue));
        backgroundQ->command = (char*)malloc(sizeof(char)*strlen(command));
        strcpy(backgroundQ->command, command);
        backgroundQ->pid = pid;
        backgroundQ->next = NULL;
        return;
    }

    backgroundQueue* last = backgroundQ;
    while(last->next != NULL){
        last = last->next;
    }
    backgroundQueue* new = (backgroundQueue*)malloc(sizeof(backgroundQueue));
        new->command = (char*)malloc(sizeof(char)*strlen(command));
        strcpy(new->command, command);
        new->pid = pid;
        new->next = NULL;
        last->next = new;
}

void deleteByPid(int pid){
    backgroundQueue *prev, *cur;

    while(backgroundQ != NULL && backgroundQ->pid == pid){
        prev = backgroundQ;
        backgroundQ = backgroundQ->next;
        free(prev);
    }

    prev = NULL;
    cur = backgroundQ;

    while(cur != NULL){
        if(cur->pid == pid){
            if(prev != NULL){
                prev ->next = cur->next;
            }

            free(cur);
            cur = prev->next;
        }
        else{
            prev = cur;
            cur = cur->next;
        }
    }
}

void execute(char** paths, char* args[], int* background){
    if(*background != 0){
        signal(SIGCHLD, childHandler);
    }
    pid_t childPid;
    childPid = fork();

    // Fork error
    if(childPid == -1) {
        fprintf(stderr, "Failed to fork.\n");
        return;
    }
    // Child
    if(childPid == 0) {
        execv(paths[0], args);
        printf("Return not expected. Must be an execv error.n");
        return;
    }
    // Parent
    int status;
    // foreground
    if(*background == 0) {
        isThereAnyForegroundProcess = 1;
        currentForegroundProcess = childPid;
        // wait child terminates
        waitpid(childPid, &status, 0);
        isThereAnyForegroundProcess = 0;
    }
    // background
    else {
        // Enqueue background process and print
        enqueueBackgroundQ(childPid, args[0]);
        
        backgroundQueue* temp = backgroundQ;
        if(temp == NULL)
            printf("There is no background process.\n");
        int i = 0;
        for(i = 0; temp != NULL; i++){
            printf("%d. Background Process' Pid: %d Command: %s\n", i , temp->pid, temp->command);
            temp = temp->next;
        }
    }
    *background = 0;

}

void removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

int main(void) {
    char inputBuffer[MAX_LINE];   /*buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    char **paths;
    int status;
    
    // sigaction init
    struct sigaction signalAction;
    signalAction.sa_handler = signalHandler;
    signalAction.sa_flags = SA_RESTART;
    // sigaction error check
    if ((sigemptyset( &signalAction.sa_mask ) == -1) || ( sigaction(SIGTSTP, &signalAction, NULL) == -1 ) ) {
        fprintf(stderr, "Couldn't set SIGTSTP handler\n");
        return 1;
    }

    node_t *head;
    head = malloc(sizeof(node_t));
    while (1) {
        background = 0;
        printf("myshell: ");
        fflush(NULL);
        // seperates the command by spaces, adds to the args array and check if & character entered
        setup(inputBuffer, args, &background);
        // Null argument handler
        if (args[0] == NULL)
            continue;
        

        char* mergedArgs = (char*)malloc(sizeof(char)* 128);
        //char* mergedArgs[MAX_LINE/2 +1 ];
        strcpy(mergedArgs, "");

        //concatenate the commands
        int count = 0;
        while (args[count] != NULL) {
                strcat(mergedArgs, args[count]);
                strcat(mergedArgs, " ");
                count++;
        }
        //add the commands to the list if the command is not history
        if(strcmp(args[0], "history")){
                push(head, mergedArgs);
        }

        //if there are more than 10 commands delete the first executed command
        if(getLength(head) == 10)
            pop(&head);
        int fd0,fd1,in=0,out=0,err=0,app=0;
        char input[64],output[64];
        
        
        // finds where '<' or '>' occurs and make that argv[i] = NULL , to ensure that command wont't read that
        if(count > 2 && args[count-2] != NULL){
            pid_t cPid = fork();

            // fork error
            if (cPid < 0)
            {
                printf("Failed to fork\n");
                return -1;
            }
            if (cPid == 0){
                if(strcmp(args[count-2],"<")==0){  
                    args[count-2]=NULL;
                    strcpy(input,args[count-1]);  
                    in=1;
                }else if(strcmp(args[count-2],">")==0){
                    args[count-2]=NULL;
                    strcpy(output,args[count-1]);
                    out=1;
                }else if(strcmp(args[count-2],"2>") == 0){
                    args[count-2]=NULL;
                    strcpy(output,args[count-1]);
                    err=1;
                }else if(strcmp(args[count-2],">>") == 0){
                    args[count-2]=NULL;
                    strcpy(output,args[count-1]);
                    app=1;
                }
            
                //if '<' char was found in string inputted by user
                if(in == 1){
                    // fdo is file-descriptor
                    int fd0;
                    
                    if ((fd0 = open(input, O_RDONLY, 0)) < 0){
                        perror("Couldn't open input file");
                        exit(0);
                    }
                    // dup2() copies content of fdo in input of preceeding file
                    dup2(fd0, STDIN_FILENO); // STDIN_FILENO here can be replaced by 0
                    close(fd0); // necessary
                }else if (out == 1){
                    int fd1 ;
                    if ((fd1 = fd1 = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){
                        perror("Couldn't open the output file");
                        exit(0);
                    }
                    dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
                    close(fd1);
                }else if(err == 1){
                    int fd2;
                    if ((fd2 = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){
                        perror("Couldn't open the output file");
                        exit(0);
                    }
                    dup2(fd2, STDERR_FILENO);
                    close(fd2);
                }else if(app == 1){
                    int fd3;
                    if ((fd3 = open(output, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) {
                        perror("Couldn't open the output file");
                        exit(0);
                    }
                    dup2(fd3, STDOUT_FILENO);
                    close(fd3);
                }
                execv(paths[0], args);
            }

            if(cPid > 0){
                int status;
                if(background == 0){
                    isThereAnyForegroundProcess = 1;
                    currentForegroundProcess = cPid;
                    waitpid(cPid, &status, 0);
                    isThereAnyForegroundProcess = 0;
                    background = 0;
                }else{
                    enqueueBackgroundQ(cPid, args[0]);
                    backgroundQueue *temp = backgroundQ;
                    int i = 0;
                    for (i = 0; temp != NULL; i++)
                    {
                        printf("%d. Background Process' Pid: %d Command: %s\n", i, temp->pid, temp->command);
                        temp = temp->next;
                    }
                    background = 0 ;
                }
            }


        }

        //if the command is history, print the list
        if(!strcmp(args[0], "history") && args[1] == NULL) {
            reverse_display(head);
        }
        if(!strcmp(args[0], "exit")) {
            if(backgroundQ != NULL){
                printf("There is some background processes. You need to kill them before exit.");
            }else{
                exit(0);
            }
        }
        if(!strcmp(args[0], "fg")){
            if(backgroundQ == NULL)
                printf("There is no background process.");
            else{
                removeChar(args[1], '%');
                deleteByPid(atoi(args[1]));
                isThereAnyForegroundProcess = 1;
                currentForegroundProcess = atoi(args[1]);
                kill(atoi(args[1]), SIGCONT);
                waitpid(atoi(args[1]), &status, WUNTRACED);
                backgroundQueue* temp = backgroundQ;
                int i = 0;
                for (i = 0; temp != NULL; i++)
                {
                    printf("%d. Background Process' Pid: %d Command: %s\n", i, temp->pid, temp->command);
                    temp = temp->next;
                }
                continue;
            }
        }
        

        else {
            //if the command is history with and index value, execute the command on that index
            char *newArgs[128];
            int a  = 0;
                while(args[a] != NULL){
            a++;
            }
            if(!strcmp(args[0], "history") && args[1] != NULL){
                newArgs[0] = GetNth(head, getLength(head) - atoi(args[2]));
                newArgs[0] = strtok(newArgs[0], " ");
                newArgs[1] = NULL;
                paths = findPath(newArgs);
                execute(paths, newArgs, &background);
            }
            else {	
                paths = findPath(args);

                if(background == 1)
                    args[count-1] = NULL;

                execute(paths, args, &background);
            }
        }
        *args = NULL;
}
        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
        				(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
        }
