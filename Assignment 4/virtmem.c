/**
 * virtmem.c 
 * Written by Michael Ballantyne 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
    unsigned char logical;
    unsigned char physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];

// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

//We check if our logical page is in the TLB, if so, we return the physical address, else, we return -1.
int checkIfPageIsInTLB(int logicalPage){
    //we pass all over the tlb array, although its circular, this is much clearer approach (if the page is there, we will get to it).
    int i;
    for(i=0;i< TLB_SIZE;i++){
        if(tlb[i].logical==logicalPage){
            return tlb[i].physical;
        }
    }
    //We didn't find it in the tlb, thus, returning -1;
    return -1; //Not found
}

int main(int argc, const char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage ./virtmem backingstore input\n");
        exit(1);
    }

    const char *backing_filename = argv[1]; 
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++) 
    {
        pagetable[i] = -1;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    // Number of the next unallocated physical page in main memory
    unsigned char free_page = 0;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) 
    {
        int logical_address = atoi(buffer);

        //For the printing (the calculation of the address later on).
        int physicalAddress;

        //Our address values, and the relative offsets (and value), declaring first value for everything
        int physicalPage=0;
        int logicalPage=0;
        int offset=0;
        int value=0;

        //Setting the value separately (since c99 didn't like it when we first initialize)
        logicalPage= (logical_address >> OFFSET_BITS) & PAGE_MASK;
        offset = logical_address & OFFSET_MASK;

        //Advancing the total Address. (the number of addresses we need to map)
        total_addresses++;

        physicalPage= checkIfPageIsInTLB(logicalPage); //First value to the physical page - from the TLB, in case it's not in the TLB, it will be "overwritten".
        if(physicalPage!=-1){
            //In the TLB
            tlb_hits++; //We advanced it since we just hit.

        }else{
            //Not in the TLB!
            // We now check if it is not in the page table (value that is -1).
            //Second value to the physical page - from the pageTable, in case it's not in the pageTable, it will be "overwritten".
            physicalPage= pagetable[logicalPage];
            if(physicalPage==-1){
                //Doesn't exist in the page table - we got page-fault exception! (in case a logical page isn't in the table yet, value will be -1).
                page_faults++;
                physicalPage = free_page;

                memcpy(main_memory + (free_page * PAGE_SIZE), backing + (logicalPage * PAGE_SIZE), PAGE_SIZE);

                pagetable[logicalPage] = free_page;
                //Advancing the free page (we start from 0).
                free_page++;

            }
            tlbindex++;
            //Overwriting/Adding to the tlb Cell.
            tlb[tlbindex % TLB_SIZE].logical = logicalPage;
            tlb[tlbindex % TLB_SIZE].physical = physicalPage;
        }

        physicalAddress= (physicalPage << OFFSET_BITS) | offset;
        value = main_memory[physicalPage* PAGE_SIZE + offset];


        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physicalAddress, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
