#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_SIZE 100

typedef struct Node {
    char data[BUFFER_SIZE];
    struct Node *next;
} Node;
typedef struct Node *nodePtr;

//Node *create_node(const char *str) {
//    Node *node = malloc(sizeof *node);
//    if (!node) {
////        perror("Memory allocation failed\n");
//        perror("error");
//        exit(1);
//    }
//    strncpy(node->data, str, BUFFER_SIZE);
//    node->next = NULL;
//    return node;
//}

Node *create_node(const char *str) {
    Node *node = malloc(sizeof *node);
    if (!node) {
        perror("error");
        exit(1);
    }
    strncpy(node->data, str, BUFFER_SIZE-1);
    node->data[BUFFER_SIZE - 1] = '\0';
    node->next = NULL;
    return node;
}


void print_list(const Node *head, int counter) {
    const Node *cur = head;
    while (cur) {
        fprintf(stdout, "%d %s\r\n", counter--, cur->data);
        cur = cur->next;
    }
}

void free_list(Node *head) {
    Node *cur = head;
    while (cur) {
        Node *next = cur->next;
//        free(cur->data);
        free(cur);
        cur = next;
    }
}

/**
 * Checks if commands ends with &, also removes it so it won't be in args once we run it.
 * @param string - the raw command string.
 * @return true or false based if & exists.
 */
bool ends_with_ampersand(char *buf) {
    size_t len = strlen(buf);
    if (len && buf[len - 1] == '\n') buf[--len] = '\0';
    if (len && buf[len - 1] == '&') {
        buf[--len] = '\0';   // actually remove it
        return true;
    }
    return false;
}


int main(void) {
    close(2);
    dup(1);
    char command[BUFFER_SIZE];

    int counter = 0; //Counting the amount of commands till clear history. (so we can track how many commands we "used" till now.)
    nodePtr history_list = NULL;
    bool isBackground; //flag for is background command.

    while (1) {
        isBackground = false;//flag reset

        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

        size_t len = strlen(command);
        if (len && command[len-1] == '\n')
            command[len-1] = '\0';          /* remove the LF */

        //Commands adding to the history list (in case its history_list/exit , it will be removed, so it's an okay behavior)
        nodePtr newCommand = create_node(command);
        //Attaching the commands to the list (supporting both case if it's empty or not empty).
        if (!history_list) {
            history_list = newCommand;
        } else {
            newCommand->next = history_list;
            history_list = newCommand;
        }
        counter++; //commands history counter

        //Checking if commands needs to run in the background.
        if (ends_with_ampersand(command)) {
            isBackground = true;
        }

        //Tokenizing & Splitting the command
        char *tokenCommand = strtok(command, " \t\n");
        char *argv[100];
        int argc = 0;
        while (tokenCommand) {
            argv[argc++] = tokenCommand;
            tokenCommand = strtok(NULL, " \t\n");
        }
        argv[argc] = NULL;
        //Empty command
        if (!argv[0]) {
            continue;
        }
        //note - tokenizing before checking for commands, since we can have cases such as "   history12" , etc... which is ok.

        if (strncmp(argv[0], "exit", 4) == 0) { //Exit called
            free_list(history_list);
            history_list = NULL;
            break;
        }
        if (strncmp(argv[0], "clear_history", 13) == 0) { //Clear_History
            free_list(history_list);
            history_list = NULL;
            counter = 0;
            continue;
        } else if (strncmp(argv[0], "history", 7) == 0) { //History - it also should prints itself.
            print_list(history_list, counter);
            continue;
        }


        pid_t pid = fork();
        //We get -1 - which means the fork failed
        if (pid < 0) {
            perror("error");
//            exit(1)
        } else if (pid == 0) {
            //Son process
            int status = 0;
            status = execvp(argv[0], argv);
            if (status < 0) { //execvp failed, error...
                perror("error");
                exit(1);
            }
        } else {
            //Father process (pid >0)
            if (!isBackground) {
                int status;
                //we wait for the son process to complete
                waitpid(pid, &status, 0);
            }
        }

    }

    return 0;
}
