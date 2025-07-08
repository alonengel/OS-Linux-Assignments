#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node {
    int value; //Current node value.
    node *next; //pointer to the next node.
    pthread_mutex_t mutex; //the mutex lock for the node.
};

struct list {
    node *nodeList; //The head pointer node of the list, will be null in case the list is empty.
    pthread_mutex_t mutex; //the mutex lock for the list.
};

void print_node(node *node) {
    // DO NOT DELETE
    if (node) {
        printf("%d ", node->value);
    }
}

list *create_list() {
    list *newList = (list *) malloc(sizeof(list));
    if (newList) {
        newList->nodeList = NULL;
    } else {
        //Error with memory allocation (malloc).
        perror("error");
        exit(1);
    }
    int result = pthread_mutex_init(&(newList->mutex), NULL);
    if (result != 0) { //Unsuccessful mutex initialize (if we get 0, it means we succeeded).
        free(newList);
        return NULL;
    }
    return newList;//The mutex and the list were successful
}

void delete_list(list *list) {
    //We will divide this function into 2 sections:
    //1. Delete the list itself, but we keep the node list.
    //2. Delete the inner node list.

    node *nodeList;
    node *deleteNode;

    if (!list) { //List is already NULL, nothing to delete.
        return;
    }

    pthread_mutex_lock(&list->mutex);
    nodeList = list->nodeList;
    list->nodeList = NULL;
    pthread_mutex_unlock(&list->mutex);
    pthread_mutex_destroy(&(list->mutex));
    free(list);

    //In case its null, the while wouldn't work, in case list has values, we will loop throw the while.
    while (nodeList) {
        pthread_mutex_lock(&(nodeList->mutex));
        deleteNode = nodeList; //The node we want to delete.
        nodeList = nodeList->next; //Advance NodeList
        pthread_mutex_unlock(&deleteNode->mutex);
        pthread_mutex_destroy(&(deleteNode->mutex));
        free(deleteNode);
    }
}

void insert_value(list *list, int value) {
    if (!list) { //In case list is empty, we can't add anything.
        return;
    }
    //First, we create the new value node:
    node *newNode = (node *) malloc(sizeof(node));
    if (newNode) {
        newNode->value = value;
        newNode->next = NULL;
        int result = pthread_mutex_init(&(newNode->mutex), NULL);
        if (result != 0) { //Unsuccessful mutex initialize (if we get 0, it means we succeeded).
            free(newNode);
            return;
        }
    } else { //Error with memory allocation (malloc)
        perror("error");
        exit(1);
    }

    pthread_mutex_lock(&(list->mutex));
    node *currNode = list->nodeList;
    node *prevNode = NULL;

    if (currNode) {
        pthread_mutex_lock(&(currNode->mutex));

    } else {
        //NodeList is empty, now we create a new head node for it.
        list->nodeList = newNode;
        pthread_mutex_unlock(&(list->mutex));
        return;
    }
    pthread_mutex_unlock(&(list->mutex));


    //We need to traverse till we get to the right value position. (or the end, whatever comes first).
    // if we got here, 3 cases:
    // 1. we might need to replace the list head. (our new value is smaller than the head value).
    // 2. we might need to add it in between
    // 3. we might need to add it in the end.
    //Self note: the same value can repeat several times!
    while (currNode) { //Travel over the nodeList
        if (value < currNode->value) {
            if (!prevNode) { //we are in the head pointer, and we need to replace the list head.
                pthread_mutex_lock(&(list->mutex));
                newNode->next = currNode;
                list->nodeList = newNode;
                pthread_mutex_unlock(&(list->mutex));
                pthread_mutex_unlock(&(currNode->mutex));
                return;
            } else { //We need to insert it between prev and current node.
                newNode->next = currNode;
                prevNode->next = newNode;
                pthread_mutex_unlock(&(prevNode->mutex));
                pthread_mutex_unlock(&(currNode->mutex));
                return;
            }
        } else {
            //We check if we got to the end, if so, we need to add it in the end.
            if (!currNode->next) {
                currNode->next = newNode;
                if(prevNode){
                    pthread_mutex_unlock(&(prevNode->mutex));
                }
                pthread_mutex_unlock(&(currNode->mutex));
                return;
            }
            //Advance in the list (if we haven't found the right spot yet).
            if(prevNode){
                pthread_mutex_unlock(&(prevNode->mutex));
            }
            prevNode = currNode;
            currNode = currNode->next;
            if (currNode) {
                pthread_mutex_lock(&(currNode->mutex));
            }
        }
    }
}

void remove_value(list *list, int value) {
    /*
     * 1. The remove is in the start - not hard.
     * 2. The remove is in the middle - the "hardest"
     * 3. The remove is in the end - not hard.
     */
    if (!list) { //The list is empty.
        return;
    }

    pthread_mutex_lock(&(list->mutex));
    node *prevNode = NULL;
    node *currNode = list->nodeList;

    if (currNode) { //we have nodelist
        //locking the first node, it must exist if we got here.
        pthread_mutex_lock(&(currNode->mutex));
    }
    pthread_mutex_unlock(&(list->mutex));


    while (currNode) {
        //Using return so if we have several with the same value, it will delete only 1 per function call.
        if (currNode->value == value) {
            if (!prevNode) { //No previous -> we delete the head!
                pthread_mutex_lock(&(list->mutex));
                list->nodeList = currNode->next;
                pthread_mutex_unlock(&(list->mutex));

                pthread_mutex_unlock(&(currNode->mutex));
                pthread_mutex_destroy(&(currNode->mutex));
                free(currNode);
                return;
            } else {
                prevNode->next = currNode->next;
                pthread_mutex_unlock(&(prevNode->mutex));

                pthread_mutex_unlock(&(currNode->mutex));
                pthread_mutex_destroy(&(currNode->mutex));
                free(currNode);
                return;
            }
        }

        node *next = currNode->next;
        if (next) {
            pthread_mutex_lock(&(next->mutex));  //we lock the next node first.
        }
        if (prevNode) {
            //unlocking the old prev.
            pthread_mutex_unlock(&prevNode->mutex);
        }
        prevNode = currNode; //current nodes becomes the prev in the next run
        currNode = next;
    }
    if (prevNode) { //in the last run it might remain locked, unlocking.
        pthread_mutex_unlock(&prevNode->mutex);
    }

}

void print_list(list *list) {
    //Empty list
    if (!list) {
        printf("\n");
        return;
    }

    pthread_mutex_lock(&(list->mutex));
    node *currNode = list->nodeList;
    if (currNode) {
        pthread_mutex_lock(&(currNode->mutex));
    }
    node *prevNode = NULL;
    pthread_mutex_unlock(&(list->mutex));

    while (currNode) {
        print_node(currNode);
        prevNode = currNode;
        currNode = currNode->next;
        if (currNode) { //making sure it's not null, it can be in the end.
            pthread_mutex_lock(&(currNode->mutex));
        }
        pthread_mutex_unlock(&(prevNode->mutex));
    }

    printf("\n"); // DO NOT DELETE
}

void count_list(list *list, int (*predicate)(int)) {
    int count = 0; // DO NOT DELETE

    if (!list) {
        printf("%d items were counted\n", 0);
        return;
    }

    pthread_mutex_lock(&(list->mutex));
    node *currNode = list->nodeList;
    if (currNode) {
        pthread_mutex_lock(&(currNode->mutex));
    }
    node *prevNode;
    pthread_mutex_unlock(&(list->mutex));

    while (currNode) {
        if (predicate(currNode->value)) {
            count++;
        }
        prevNode = currNode;
        currNode = currNode->next;
        if (currNode) {
            //in case we have next,we lock it
            pthread_mutex_lock(&(currNode->mutex));
        }
        pthread_mutex_unlock(&(prevNode->mutex));
    }
    printf("%d items were counted\n", count); // DO NOT DELETE
}

void swap_values(list *list, int val1, int val2) {

    //we know that val1 and val2 are different (even if we change them, it doesn't harm anything, just wasteful).
    if (!list || val1 == val2) {
        //Empty List or empty nodeList (Empty Nodelist just to not go deeper in the function).
        return;
    }

    //Pointer to the ptr we will use to run over the list node with.
    pthread_mutex_lock(&(list->mutex));
    node *currNode = list->nodeList;
    if (currNode) {
        pthread_mutex_lock(&(currNode->mutex));
    }
    pthread_mutex_unlock(&(list->mutex));

    //Pointers for the values nodes
    node *firstNodePtr = NULL;
    node *secondNodePtr = NULL;
    node *prevNodePtr = NULL; //I want to advance to next node before I unlock the node mutex (better behavior), so using this as a helper pointer.

    //Finding the values locations
    while (currNode != NULL) {
        if (currNode->value == val1 && !firstNodePtr) {
            //the value is equal to the first value, and first value is null (not set up already).
            firstNodePtr = currNode;
        } else if (currNode->value == val2 && !secondNodePtr) {
            //second val, else, so we don't lock both (in case they are equal, even with the if statement beforehand).
            secondNodePtr = currNode;
        }
        prevNodePtr = currNode;
        currNode = currNode->next;
        if (currNode) { //locking the next node
            pthread_mutex_lock(&(currNode->mutex));
        }
        if (prevNodePtr != firstNodePtr && prevNodePtr != secondNodePtr) {
            //unlocking the current node (not the next), in case it's not equal to first Node or the second Node
            pthread_mutex_unlock(&(prevNodePtr->mutex));
        }
    }

    //We found both nodes, if not, we can't really switch.
    if (firstNodePtr && secondNodePtr) {
        //Switching values - we are locked from earlier.
        int tempVal = firstNodePtr->value;
        firstNodePtr->value = secondNodePtr->value;
        secondNodePtr->value = tempVal;
        //Releasing by order
        if (firstNodePtr < secondNodePtr) {
            pthread_mutex_unlock(&(firstNodePtr->mutex));
            pthread_mutex_unlock(&(secondNodePtr->mutex));
        } else {
            pthread_mutex_unlock(&(secondNodePtr->mutex));
            pthread_mutex_unlock(&(firstNodePtr->mutex));
        }
    } else {
        //Unlock the pointers in case we found only 1 pointer and can not complete the switch.
        if (firstNodePtr) {
            pthread_mutex_unlock(&(firstNodePtr->mutex));
        }
        if (secondNodePtr) {
            pthread_mutex_unlock(&(secondNodePtr->mutex));
        }
    }
}
