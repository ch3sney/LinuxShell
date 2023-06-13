#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMANDS 10 /* number of commands to display */
char buffer[MAX_COMMANDS * MAX_LINE + MAX_COMMANDS + 64]; /* buffer to pass to signal handler */
char history_file[64] = "chesney18.history";
int counter = 0; /*global program counter */

/* node structure for storing command history */
struct Node {
    int index;
    char **args;
    int len;
    int background;
    struct Node *next;
    struct Node *previous;
};

/* global linked list head */
struct Node *head = NULL;

/* function to handle SIGINT event, reprints 'COMMAND' for clarity's sake */
void handle_SIGINT() {
    write(STDOUT_FILENO, buffer, strlen(buffer));
    write(STDOUT_FILENO, "COMMAND->", strlen("COMMAND->"));
}

void update_buffer() {
	int i;

        /* create traverse node starting at head */
        struct Node *traverse = head;

        /* clear buffer to avoid possible residual data */
        memset(buffer, '\0', sizeof(buffer));

        /* begin with new line */
        strcat(buffer, "\n");

        /* traverse through linked list */
        while (traverse != NULL) {
            /* only proceed with 10 most recent commands */
            if (traverse->index >= counter - 10) {
                /* placeholder index variable */
                char currIndex[12];
                sprintf(currIndex, "%d", traverse->index + 1);

                /* populate 'row' in buffer with command */
                strcat(buffer, currIndex);
                strcat(buffer, ".\t");
                for (i = 0; i < traverse->len; i++) {
                    strcat(buffer, traverse->args[i]);
                    strcat(buffer, " ");
                }

                /* append a '&' if background = 1 */
                if (traverse->background == 1) {
                    strcat(buffer, "&");
                }

                strcat(buffer, "\n");
            }
            traverse = traverse->next;
        }
}

char *get_line(int fd) {
    char* lineBuffer;
    char c;
    int read_ret;
    int index = 0;

    lineBuffer = (char*) malloc(MAX_LINE * sizeof(char));

    while(1) {
        read_ret = read(fd, &c, 1);
        if (read_ret <= 0) {
                break;
        }

        if (c == '\n' && index != 0) {
                lineBuffer[index] = '\0';
                return lineBuffer;
        }

        lineBuffer[index] = c;
        index++;
    }
    return NULL;
}

void printStringToFile(const char *string) {
    int fd = open(history_file, O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    size_t len = strlen(string);
    ssize_t bytes_written = write(fd, string, len);
    if (bytes_written != (ssize_t)len) {
        perror("Error writing to file");
        close(fd);
        return;
    }

    close(fd);
}

void update_file() {
        /* create traverse node starting at head */
        struct Node *traverse = head;
	int fd;
	int i;

	/* clear file */
	fd = open(history_file, O_WRONLY | O_TRUNC);
	close(fd);

	while (traverse->next != NULL) {
		traverse = traverse->next;
	}
	
        /* traverse through linked list */
        while (traverse != NULL) {
            /* only proceed with 10 most recent commands */
            if (traverse->index >= counter - 10) {
                /* populate 'row' in buffer with command */
		char currBuffer[8];
                sprintf(currBuffer, "%d", traverse->len);
                printStringToFile(currBuffer);
		printStringToFile(" ");

		memset(currBuffer, 0, 8);
                sprintf(currBuffer, "%d", traverse->background);
                printStringToFile(currBuffer);
                printStringToFile(" ");

                for (i = 0; i < traverse->len; i++) {
                    printStringToFile(traverse->args[i]);
                    if (i != traverse->len - 1) {
			printStringToFile(" ");
		    }
                }

                printStringToFile("\n");
            }
            traverse = traverse->previous;
        }
}

void populate_list(int fd) {
    char *lineBuffer;
    char *tokenBuffer;
    int i;

    lineBuffer = get_line(fd);
    while(lineBuffer != NULL) {
	/* declare new node */
        struct Node *newNode = NULL;
        newNode = (struct Node *) malloc(sizeof(struct Node));

        /* populate data fields */
        newNode->index = counter;
        newNode->len = atoi(strtok(lineBuffer, " "));
	newNode->background = atoi(strtok(NULL, " "));
        newNode->args = (char **) malloc((newNode->len) * sizeof(char *));
        for (i = 0; i < newNode->len; i++) {
            newNode->args[i] = strtok(NULL, " ");
        }

        /* insert at front of linked list */
	if (head != NULL) {
		head->previous = newNode;
	}
        newNode->next = head;
        head = newNode;
	
        /* update program counter */
        counter++;
	
        lineBuffer = get_line(fd);
    }
    update_buffer();
}

int open_file() {
    int fd;
    int close_ret;

    fd = open(history_file, O_RDONLY);
    if(fd == -1) {
	printf("Creating history file, '%s'...\n", history_file);
	fd = creat(history_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
            printf("Failed to create file.\n");
            return -1;
        } else {
            printf("File created successfully!\n");
	    close_ret = close(fd);
        }
    } else {
	printf("Found history file, '%s'...\n", history_file);
	populate_list(fd);
	close_ret = close(fd);
    }
}

void setup(char inputBuffer[], char *args[], int *background, int *failedSearch, int *resultArgs) {
    int length; /* # of characters in the command line */
    int i;      /* loop index for various uses */
    int start;  /* index where beginning of next command parameter is */
    int ct;    /* index of where to place the next parameter into args[] */
    int found; /* boolean value to mark successful search */

    ct = 0;
    found = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* special case for 'r x' commands */
    if (inputBuffer[0] == 'r' && inputBuffer[1] == ' ') {
        /* traverse linked list searching for corresponding command */
        struct Node *search = head;
        while (search != NULL && !found) {
            if (search->index > counter - 10) {
                if (inputBuffer[2] == search->args[0][0]) {
                    /* transfer linked list data */
                    for (i = 0; i < search->len; i++) {
			args[i] = (char*)malloc(sizeof(search->args[i]));
                        strcpy(args[i], search->args[i]);
                    }
                    *background = search->background;
                    ct = search->len;
                    *resultArgs = ct;
                    found = 1;
                }
            }
            search = search->next;
        }

        /* case if search fails */
        if (search == NULL && !found) {
            *failedSearch = 1;
            printf("ERROR: command not found.\n");
        }
    /* case for 'r' command */
    } else if (inputBuffer[0] == 'r' && inputBuffer[1] == '\n') {
        /* transfer linked list data */
        for (i = 0; i < head->len; i++) {
            strcpy(args[i], head->args[i]);
        }
        *background = head->background;
        ct = head->len;
        *resultArgs = ct;
    } else {
        /* standard input case */
        for (i = 0; i < length; i++) {
            switch (inputBuffer[i]) {
                case ' ':
                case '\t' :               /* argument separators */
                    if (start != -1) {
                        args[ct] = &inputBuffer[start];    /* set up pointer */
                        ct++;
                    }
                    inputBuffer[i] = '\0'; /* add a null char; make a C string */
                    start = -1;
                    break;

                case '\n':                 /* should be the final char examined */
                    if (start != -1) {
                        args[ct] = &inputBuffer[start];
                        ct++;
                    }
                    inputBuffer[i] = '\0';
                    args[ct] = NULL; /* no more arguments to this command */
                    break;

                case '&':
                    *background = 1;
                    inputBuffer[i] = '\0';
                    break;

                default :             /* some other character */
                    if (start == -1)
                        start = i;
            }
        }
        args[ct] = NULL; /* just in case the input line was > 80 */
    }
    /* update linked list model (skip in case of failed search) */
    if (*failedSearch == 0) {
        /* declare new node */
        struct Node *newNode = NULL;
        newNode = (struct Node *) malloc(sizeof(struct Node));

        /* populate data fields */
        newNode->index = counter;
        newNode->len = ct;
        newNode->args = (char **) malloc((newNode->len) * sizeof(char *));
        for (i = 0; i < newNode->len; i++) {
            newNode->args[i] = (char *) malloc(sizeof(args[i]));
            strcpy(newNode->args[i], args[i]);
        }
        newNode->background = *background;

        /* insert at front of linked list */
        if (head != NULL) {
                head->previous = newNode;
        }
        newNode->next = head;
        head = newNode;

        /* update program counter */
        counter++;
	
	update_buffer();
	update_file();
    }
}

int main(void) {
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    int failedSearch;        /* equals 1 if search failed to find a matching command */
    int resultsArgs;        /* number of arguments in search results */
    int i;                  /* various for loop uses */
    pid_t pid, child_pid;

    open_file();

    while (1) {        /* Program terminates normally inside setup */
        background = 0;
        resultsArgs = -1;
        failedSearch = 0;
        char *args[MAX_LINE / 2 + 1];    /* command line (of 80) has max of 40 arguments */
        printf("COMMAND->");
        fflush(0);

	/* create a sigaction handler for when program is first initialized */
	struct sigaction handler;
        handler.sa_handler = handle_SIGINT;
        handler.sa_flags = SA_RESTART;
        sigaction(SIGINT, &handler, NULL);
	
        setup(inputBuffer, args, &background, &failedSearch, &resultsArgs);        /* get next command */

	/* remove this handler */
	handler.sa_handler = SIG_DFL;
        handler.sa_flags = SA_RESTART;
        sigaction(SIGINT, &handler, NULL);
	
        /* only create fork if a failed search hasn't occurred */
        if (failedSearch == 0) {
            pid = fork();
        }                /* fork another process */
        if (pid < 0) {
            fprintf(stderr, "Fork failed!");    /* fork error handling */
            exit(-1);
        } else if (pid == 0 && failedSearch == 0) {
            /* special case created for when a search command is rerun...
             * somewhat of a workaround, but it works!
             */
            if (resultsArgs == -1) {
                int ret = execvp(args[0], args);
            } else {
                char *searchArgs[MAX_LINE / 2 + 1];
                for (i = 0; i < resultsArgs; i++) {
                    searchArgs[i] = args[i];
                }
                int ret = execvp(searchArgs[0], searchArgs);
            }
            int ret = execvp(args[0], args);            /* call command in child process */
            if (ret < 0) {
                exit(-1);
            }
            child_pid = wait(NULL);
        } else if (failedSearch == 0) {
            /* only create handler for 'non-child' process */
            struct sigaction handler;
            handler.sa_handler = handle_SIGINT;
            handler.sa_flags = SA_RESTART;
            sigaction(SIGINT, &handler, NULL);
        }
        if (background == 0) {
            child_pid = wait(NULL);            /* wait for process to complete */
        }
        /* else return to set up */
    }
}

