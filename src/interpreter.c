#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
// #include <pthread.h>

int MAX_ARGS_SIZE = 7;

int help();
int quit();
int echo(char *value);
int set(char *var, char *value);
int print(char *var);
int addScriptToReadyQueue(char *script, char* original_script_name);
int FCFS();
int SJF();
int SJFAging();
int RR(int numLines);
int run(char *script);
int exec(char *scripts[], char *policy, int num_scripts, int backgroundBool, int MTBool);
int badcommandFileDoesNotExist();
int CleanupFailPCBIDNotFound(size_t id);
int InvalidPolicy(char *policy);
int BadCommandSameFileName();
int pageTableUpdateError();
int my_ls();
int my_touch(char *filename);
int my_mkdir(char *dirname);
int my_cd(char *dirname);
int one_liner(char *commands[], int size, int numberOfCommands);
int copy_script_file(char* src_file_path, char* dest_file_path);

// counter for files stored to backing store - used to name our files
int backing_store_count = 0;
// array to keep track of pointer of file names in back store. Used to clear heap memory later. 
char *backing_store_files[3];

// size of our page tables and frames
#define PAGE_TABLE_SIZE 100

// Keeps track of the number of programs.
size_t NUM_PROGRAMS_INTERVAL = 100000;
size_t numPrograms = 100000;

// PCB CODE

// struct to model PCB nodes in the ready queue
typedef struct PCBNode
{
    char *fileName; // fileName in backing store for debugging purposes

    char *og_file_name; // original file name passed to exec for debugging purposes

    pid_t pid; // process ID
    // pid_t is a data type that represents Process IDs.
    // get the pid of the process by calling getpid.
    // getpid returns the process ID of the parent of the current process.

    size_t pcb_id; // id of the PCB

    size_t starting_position; // Spot in the Shell Memory where the SCRIPT is loaded.

    size_t script_length; // length of the script

    size_t page_table_index; // last index of page table processed

    size_t pc; // program counter

    size_t job_length_score; // job length score for sjf aging

    int page_table[PAGE_TABLE_SIZE]; // page table as an int array

    struct PCBNode *next; // next pointer
} PCBNode;

void printPCB(PCBNode *node)
{
    printf("The file name is: %s\n", node->fileName);
    printf("The Process ID is: %d\n", node->pid);
    printf("The PCB ID is: %ld\n", node->pcb_id);
    printf("The starting position is: %ld\n", node->starting_position);
    printf("The length of the script is: %ld \n", node->script_length);
    printf("The PC is at : %ld\n", node->pc);
    printf("Page Table:\n");
    int i = 0;
    while(i < 10){
        printf("%d -> %d\n", i, node->page_table[i]);
        i++;
    }
    printf("The last page table index processed is: %ld\n", node->page_table_index);
}

// Head of the ready queue. Null if empty
PCBNode head = {"head", "head", 0, -1, -1, -1, -1, -1, -1, {}, NULL};
PCBNode Jhead = {"head", "head", 0, -1, -1, -1, -1, -1, -1, {}, NULL};

// Creates a PCBNode with given inputs. Note that the pcb_id is used as the starting position as well
PCBNode *create_pcb_node(char *fileName, char* og_file_name, pid_t pid, size_t pcb_id, size_t script_length, size_t pc, int page_table[])
{
    PCBNode *new_node = (PCBNode *)malloc(sizeof(struct PCBNode));
    new_node->fileName = fileName; // for testing
    new_node->og_file_name = og_file_name; // for testing
    new_node->pid = pid;
    new_node->pcb_id = pcb_id;
    new_node->starting_position = pcb_id;
    new_node->script_length = script_length;
    new_node->pc = pc;
    new_node->page_table_index = 0;
    new_node->job_length_score = script_length;
    memcpy(new_node->page_table, page_table, PAGE_TABLE_SIZE*sizeof(int)); // copy passed in page_table to new_node->page_table
    new_node->next = NULL; // null since new node

    return new_node; // return a pointer to the new node
}

// addToEndOfReadyQueue adds a PCB to the end of the ready queue and iterates numPrograms
void addToEndOfReadyQueue(PCBNode *new_node)
{
    if (head.next == NULL) // the linked list is currently empty
    {
        head.next = new_node;
    }
    else // the linked list is not empty
    {
        PCBNode *current_node = &head;
        while (current_node->next != NULL)
        {
            current_node = current_node->next;
        }
        current_node->next = new_node;
    }

    // Incremement number of programs
    numPrograms += NUM_PROGRAMS_INTERVAL;
}

// Moves node to front of ready queue
void moveToFrontOfReadyQueue(PCBNode *node) 
{

    PCBNode *cur = &head;

    // get to the node just before node
    while (cur->next != node) {
        cur = cur->next;
    }

    // remove node, move it to front
    cur->next = node->next;
    node->next = head.next;
    head.next = node;
}

void moveToEndOfReadyQueue(PCBNode *node)
{
    PCBNode *cur = head.next;

    PCBNode *before = NULL;
    PCBNode *next = NULL;


    if(node->next == NULL){
        return;
    } else {
        while(cur->next != NULL){
            
            // finding the script before the node to be moved
            if(cur->next->pcb_id == node->pcb_id){
                before = cur;
            } 

            // finding the script after the node to be moved
            if(cur->pcb_id = node->pcb_id){
                next = cur->next;
            }

            cur = cur->next;
        }

        before->next = next;
        cur->next = node;

    }
}

// finds process with given pid and releases all of it's code from memory
int cleanUp(PCBNode *to_remove)
{
        // Clears all the code associated with this PCB from memory
        size_t lineNumber = 0;

        int page_table[PAGE_TABLE_SIZE];
        int page_table_index = 0;
        for (int i = 0; i < 100; i++) {
            page_table[i] = -1;
        }

        while (page_table_index < PAGE_TABLE_SIZE && page_table[page_table_index] != -1)
        {
            // clears line from memory
            frame_clear_value(page_table[page_table_index]);
            page_table_index++;
        }

        free(to_remove);

        return 0;
}

// END PCB CODE

int badcommand()
{
    printf("%s\n", "Unknown Command");
    return 1;
}

int badcommandTooManyTokens()
{
    printf("%s\n", "Bad command: Too many tokens");
    return 4;
}

// For run command only
int badcommandFileDoesNotExist()
{
    printf("%s\n", "Bad command: File not found");
    return 3;
}

int CleanupFailPCBIDNotFound(size_t id)
{
    printf("%s%ld\n", "Cleanup Failed: PCB ID was not found: ", id);
    return 5;
}

int InvalidPolicy(char* policy) 
{
    printf("%s is an invalid policy\n", policy);
    return 6;
}

int BadCommandSameFileName() 
{
    printf("Bad command: same file name\n");
    return 8;
}

int pageTableUpdateError(){
    printf("Page Table Update of evicted page: Unsuccessful");
    return 1;
}

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size)
{
    int i;

    int numNull = 0;
    // If the input is not one liner then the number of \0 in command_args is 0

    // Checking if the input is to be executed as a one-liner
    for (int j = 0; j < args_size; j++)
    {
        if (strcmp(command_args[j], "\0") == 0)
        {
            numNull++;
        }
    }

    if (numNull > 0)
    {

        // One liner input
        int numCommands = numNull + 1;
        // Number of commands to execute = numNull + 1
        return one_liner(command_args, args_size, numCommands);
    }

    if (args_size < 1)
    {
        return badcommand();
    }

    if ((args_size > MAX_ARGS_SIZE) && (numNull == 0))
    {
        return badcommandTooManyTokens();
    }

    for (i = 0; i < args_size; i++)
    { // strip spaces new line etc
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0)
    {
        // help
        if (args_size != 1)
            return badcommand();
        return help();
    }
    else if (strcmp(command_args[0], "quit") == 0)
    {
        // quit
        if (args_size != 1)
            return badcommand();
        return quit();
    }
    else if (strcmp(command_args[0], "set") == 0)
    {
        // set
        if (args_size < 3)
            return badcommand();
        if (args_size > 7)
            return badcommandTooManyTokens();

        // initializes buffer with first arg
        char buffer[505];
        strcpy(buffer, command_args[2]);

        // copies over args 3-4, adding spaces between them
        for (int i = 3; i < args_size; i++)
        {
            strcat(buffer, " ");
            strcat(buffer, command_args[i]);
        }

        return set(command_args[1], buffer);
    }
    else if (strcmp(command_args[0], "echo") == 0)
    {
        // echo
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);
    }
    else if (strcmp(command_args[0], "print") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);
    }
    else if (strcmp(command_args[0], "run") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return run(command_args[1]);
    }
    else if (strcmp(command_args[0], "exec") == 0)
    {
        if (args_size < 3 || args_size > 6)
            return badcommand();

            // copy over scripts into scripts array
            char **scripts = (char **)malloc((args_size - 2) * sizeof(char *));
            for (int i = 1; i < args_size - 1; i++)
            {
                scripts[i - 1] = command_args[i];
            }

            int errCode = 0;
        
            // background = 1 : execute without #
            errCode = exec(scripts, command_args[args_size - 1], args_size - 2, 1, 1);
            free(scripts);
            return errCode;
    }
    else if (strcmp(command_args[0], "my_ls") == 0)
    {
        if (args_size != 1)
            return badcommand();
        return my_ls();
    }
    else if (strcmp(command_args[0], "my_touch") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_mkdir") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_cd") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);
    }
    else
        return badcommand();
}

int help()
{

    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit()
{
    printf("%s\n", "Bye!");

    mem_clear_backing_store(backing_store_files, backing_store_count);
    clear_LRU();
    exit(0);
}

int echo(char *value)
{

    if (value[0] == '$')
    {
        char *buffer = mem_get_value(value + 1);

        if (strcmp(buffer, "Variable does not exist") != 0)
        {
            printf("%s\n", buffer);
        }
        else
        {
            printf("\n");
        }
    }
    else
    {
        printf("%s\n", value);
    }

    return 0;
}

int set(char *var, char *value)
{
    char *link = "=";
    char buffer[1000];
    strcpy(buffer, var);
    strcat(buffer, link);
    strcat(buffer, value);

    mem_set_value(var, value);

    return 0;
}

int print(char *var)
{
    printf("%s\n", mem_get_value(var));
    return 0;
}

// loads script into memory, adds to end of ready queue
int addScriptToReadyQueue(char *script, char* original_script_name) 
{
    int errCode = 0;
    char line[1000];

    // calculating the number of lines in the script
    FILE *p1 = fopen(script, "rt");

    int lineCount = 0; // number of lines in the script
    while(fgets(line, 99, p1) != NULL){
        lineCount++;
    }

    fclose(p1);

    FILE *p = fopen(script, "rt"); // the program is in a file

    if (p == NULL)
    {
        return badcommandFileDoesNotExist();
    }

    size_t lineNumber = 0;

    // make pageTable -> -1 means no page exists
    int page_table[PAGE_TABLE_SIZE];
    
    int page_table_index = 0;

    // init page table with -1 indicating that no page exists
    for (int i = 0; i < 100; i++) {
        page_table[i] = -1;
    }

    // number of frames to be loaded in the beginning
    int num;

    if(lineCount > 3){
        num = 1;
    } else{
        num = 2;
    }

    // saves each line of the script into memory
    while (num != -1)
    {
        char *lines[FRAME_SIZE];

        // gets FRAME_SIZE lines from file at a time
        for (int i = 0; i < FRAME_SIZE; i++) {
            if (fgets(line, 999, p) == NULL) {
                // if already eof, then this line is "none"
                lines[i] = "none";
            }
            else { 
                lines[i] =  strdup(line);
                lineNumber++;
            }

        }

        // frame_set_value returns frame index or error code of INT_MAX
        page_table[page_table_index] = frame_set_value(lines[0], lines[1], lines[2]);
        
        // move the frame to the end of the LRU queue
        moveFrameToEndOfLRU(page_table[page_table_index]);
        
        // if error code, return error code and stop
        if (page_table[page_table_index] == INT_MAX) {
            fclose(p);
            return page_table[page_table_index];
        }

        page_table_index++;
       
        if (feof(p)) {
            break;
        }
    
        num--;
    }
    
    addToEndOfReadyQueue(create_pcb_node(script, original_script_name, getpid(), numPrograms, lineCount, 0, page_table));

    fclose(p);

    return errCode;
}

int updatePageTableOfEvictedPage(int frameID)
{
    int page_table[PAGE_TABLE_SIZE]; 
    PCBNode *temp = head.next;

    while(temp != NULL){
        memcpy(page_table, temp->page_table, sizeof(int) * PAGE_TABLE_SIZE);
        
        int index = 0;
        while(index < 10){
            if(page_table[index] == frameID){
                page_table[index] = -1;
                temp->page_table[index] = -1;
                return temp->pcb_id;
            }
            index++;
        } 
        temp = temp->next;
    }

    return -1; 
}

int handlePageFaults(PCBNode *cur)
{
    int errCode;

    int page_table[PAGE_TABLE_SIZE]; 
    int page_table_index = cur->page_table_index;

    printf("Page fault! Victim page contents:\n");
    moveToEndOfReadyQueue(cur);

    // Find the first empty frame in Physical Memory
    int frameID = findFirstAvailableHole();
    
    //printf("First Available hole: %d\n", frameID);
    getFrameLines(frameID);
    frame_clear_value(frameID);

    // update the page table of the victed page
    int i = 0;
    while(i < 10){
        if(cur->page_table[i] == frameID){
            cur->page_table[i] = -1;
        }
        i++;
    }
    
    
    // load new lines into frameID
    printf("End of victim page contents.\n");

    // load code into frame in memory
    char line[1000];
    int lineNumber = 1;

    FILE *p = fopen(cur->fileName, "rt"); // the program is in a file

    // get to the line to be loaded
    while(lineNumber <= (cur->page_table_index)*3){
        fgets(line, 999, p);
        lineNumber++;
    }

    char *lines[FRAME_SIZE];

    // get lines to be loaded
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (fgets(line, 999, p) == NULL) {
            // if already eof, then this line is "none"
            lines[i] = "none";
        }
        else {
            lines[i] =  strdup(line);
            lineNumber++;
        }
    }

    

    // frame_set_value returns frame index or error code of INT_MAX
    cur->page_table[page_table_index] = frame_set_value(lines[0], lines[1], lines[2]);
    //moveFrameToEndOfLRU(page_table[page_table_index]);
    //printLRU();
    // if error code, return error code and stop
        if (page_table[page_table_index] == INT_MAX) {
        fclose(p);
        return page_table[page_table_index];
    }

    fclose(p);            

    int frameNum = cur->page_table[cur->page_table_index];
    moveFrameToEndOfLRU(frameNum);

    char **result = malloc(3 * sizeof(char *));
    frame_get_value(frameNum, result);
    memcpy(lines, result, sizeof(char *) * FRAME_SIZE);
    free(result);

    page_table_index++;
    cur->page_table_index = page_table_index;
    
    // parse all the non-empty frames
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (strcmp(lines[i], "none") != 0) {
            cur->pc = (cur->pc) + 1;
            errCode = parseInput(lines[i]);
        } 
    }
    return errCode;
}

// executes 1-3 processes with given policy. Currently policy is ignored and FCFS is used instead
int exec(char *scripts[], char *policy, int num_scripts, int backgroundBool, int MTBool)
{
    int errCode = 0;

    if (!(strcmp(policy, "FCFS") == 0 || strcmp(policy, "SJF") == 0 ||
    strcmp(policy, "RR") == 0 || strcmp(policy, "AGING") == 0 || strcmp(policy,"RR30") == 0)){
        return InvalidPolicy(policy);
    }

    // adds all scripts to backing store
    char* new_scripts[num_scripts];
    for (int i = 0; i < num_scripts; i++) {
        char* new_script_name = (char*) malloc(256);
        sprintf(new_script_name, "./backing_store/%d.txt", backing_store_count);
        backing_store_files[backing_store_count] = new_script_name;
        backing_store_count++;
        new_scripts[i] = new_script_name;
       
        // copies scripts[i] to new file in backing store with name new_scripts[i];
        errCode = copy_script_file(scripts[i], new_scripts[i]);
        if (errCode != 0) {
            return errCode;
        }
    }

    // adds all scripts to ready queue, stops if memory is full
    for (int i = 0; i < num_scripts; i++) {
        errCode = addScriptToReadyQueue(new_scripts[i], scripts[i]);
        if (errCode != 0) {
            return errCode;
        }
    }

   // print_memory();

    if(MTBool == 0){
        return errCode;
    }
    else {
        // Handle different policies here
        if (strcmp(policy, "FCFS") == 0) {
            errCode = FCFS();
        } else if(strcmp(policy, "SJF") == 0){
            // errCode = SJF(backgroundBool);
            errCode = SJF();
        } else if(strcmp(policy, "RR") == 0){
            // errCode = RR(backgroundBool,2);
            errCode = RR(2);
        } else if(strcmp(policy, "RR30") == 0){
            errCode = RR(30);
        }else if (strcmp(policy, "AGING") == 0) {
            errCode = SJFAging();
        } return errCode;
    }
   
    
}

// Runs a singular script using FCFS policy
int run(char *script)
{
    // copy script into backing store
    char* new_script_name = (char*) malloc(256);
    sprintf(new_script_name, "./backing_store/%d.txt", backing_store_count);
    backing_store_files[backing_store_count] = new_script_name;
    backing_store_count++;

    int errCode = copy_script_file(script, new_script_name);

    if (errCode != 0) {
        return errCode;
    }

    // Loads script into memory, adds to ready queue
    errCode = addScriptToReadyQueue(new_script_name, script);

    // runs scheduler in FCFS mode if script was added without error
    if (errCode == 0) {
        FCFS();
    }

    return errCode;
}

// FCFS scheduler
int FCFS()
{
    int errCode = 0;

    
    // While there's a program
    while (head.next != NULL)
    {

        // We remove the node from the ready queue while operating on it to avoid issues with nested run/exec
        struct PCBNode *cur = head.next;
        head.next = head.next->next;

        // get the page table from cur
        int page_table[PAGE_TABLE_SIZE]; 
        memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE); 

        int page_table_index = 0;

        // while there are frames in memory
        while (page_table[page_table_index] != -1)
        {
            // copy over the current frame from memory
            char *lines[FRAME_SIZE];

            char **result = malloc(3 * sizeof(char *));
            frame_get_value(page_table[page_table_index], result);
            memcpy(lines, result, sizeof(char *) * FRAME_SIZE);
            free(result);

           //memcpy(lines, frameGetValue(page_table[page_table_index]), sizeof(char *) * FRAME_SIZE);
            moveFrameToEndOfLRU(page_table[page_table_index]);
            page_table_index++;
            cur->page_table_index = page_table_index;

            // parse all the non-empty frames
            for (int i = 0; i < FRAME_SIZE; i++) {
                if (strcmp(lines[i], "none") != 0) {
                    cur->pc = (cur->pc)+1;
                    errCode = parseInput(lines[i]);
                }
            }
        }
       
        while(cur->pc < cur->script_length){
            errCode = handlePageFaults(cur);
        }
        
        errCode = cleanUp(cur);
        
        if (errCode != 0) {
            return errCode;
        }
    }

    return errCode;
}

// SJF Scheduler
int SJF()
{
    // sort the ready queue based on the length of the script. 
    // then execute as FCFS
    PCBNode *i;
    PCBNode *j; 

        for(i = head.next; i != NULL; i=i->next){
            for(j = i->next; j != NULL; j=j->next){
                if(i->script_length > j->script_length){
                    pid_t tempPID = i->pid;
                    size_t tempStarting_position = i->starting_position;
                    size_t tempScript_length = i->script_length;
                    size_t tempPC = i->pc;
                    
                    i->pid = j->pid;
                    i->starting_position = j->starting_position;
                    i->script_length = j->script_length;
                    i->pc = j->pc;
                    
                    j->pid = tempPID;
                    j->starting_position = tempStarting_position;
                    j->script_length = tempScript_length;
                    j->pc = tempPC;
                }
            }
        }

    int errCode = FCFS();
    return errCode;
}

// Sorts lists by job length score
int sortListByJobLengthScore() 
{
    if (head.next->next == NULL || head.next == NULL) {
        return 0;
    }

    int test = 1;

    // make a temp list, empty our queue
    PCBNode *tempHead = (PCBNode *) malloc(sizeof(PCBNode));
    tempHead->next = head.next;

    head.next = NULL;

    while (tempHead->next != NULL) {

        PCBNode *cur = tempHead->next;
        int min = cur->job_length_score;
        PCBNode *lowest = cur;

        // find lowest node in temp list
        while (cur != NULL) {
            if (cur->job_length_score < min) {
                min = cur->job_length_score;
                lowest = cur;
            }
            cur = cur->next;
        }

        // remove lowest from temp list
        cur = tempHead;
        while (cur->next != lowest) {
            cur = cur->next;
        }

        cur->next = lowest->next;
        lowest->next = NULL;

        // add lowest to end of readyQueue
        addToEndOfReadyQueue(lowest);
    }

    free(tempHead);

    return 0;
}


int SJFAging() 
{
    int errCode = 0;

    PCBNode *temp;
    PCBNode *cur;

    while (head.next != NULL) {

        sortListByJobLengthScore();

        // We remove the node from the ready queue while operating on it to avoid issues with nested run/exec
        cur = head.next;
        head.next = head.next->next;

        size_t lineNumber = cur->pc;

        // gets page table and index
        int page_table[PAGE_TABLE_SIZE]; 
        memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE); 

        int page_table_index = cur->page_table_index;

        // gets frames from page_table
        char *lines[FRAME_SIZE];

        char **result = malloc(3 * sizeof(char *));
        frame_get_value(page_table[page_table_index], result);
        memcpy(lines, result, sizeof(char *) * FRAME_SIZE);
        free(result);

        //memcpy(lines, frameGetValue(page_table[page_table_index]), sizeof(char *) * FRAME_SIZE);

        // parses relevant frame
        int index = lineNumber % FRAME_SIZE;
        if (strcmp(lines[index], "none") != 0) {
            errCode = parseInput(lines[index]);
        }

        // if at last line inside of frame, go to next frame
        if (lineNumber % FRAME_SIZE == FRAME_SIZE - 1) {
            cur->page_table_index = cur->page_table_index + 1;
        }

        lineNumber++;

        cur->pc = lineNumber;

        // decrement job length score of all other jobs
        temp = head.next;
        while (temp != NULL) {
            int new_score = temp->job_length_score - 1;

            temp->job_length_score = new_score;
            if (new_score < 0) {
                temp->job_length_score = 0;
            }

            temp = temp->next;
        }

        // if done cleanup, else add back to ready queue
        if (lineNumber >= cur->script_length) {
            errCode = cleanUp(cur);
        }
        else {
            head.next = cur;
        }

    }

    return errCode;
}

// prints the processes in the ready queue
void printReadyQueue()
{
    PCBNode *temp = head.next;
    
    while(temp != NULL){
        printf("%ld -> ", temp->pcb_id);
        temp = temp->next;
    }
    printf("NULL\n");

}

int handleRRPageFault(PCBNode *cur)
{
    int page_table[PAGE_TABLE_SIZE]; 
    memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE); 
    int page_table_index = cur->page_table_index;

    // Check if there is already an empty frame in the memory
    if(!isEmptyFrameAvailable()){
        printf("Page fault! Victim page contents:\n");
        int frameID = findFirstAvailableHole();
        //printf("Frame returned by LRU is: %d\n", frameID);
        getFrameLines(frameID);
        printf("End of victim page contents.\n");
        frame_clear_value(frameID);

        // update page table of the evicted page
        if(updatePageTableOfEvictedPage(frameID) == -1){
            // page evicted its own page
            int i = 0;
            while(i < 10){
                if(cur->page_table[page_table_index] == frameID) {
                    cur->page_table[page_table_index] = -1;
                    break;
                }
                i++;
            }
        }
    }
    
    // move the process that caused a paging fault to the end of the ready queue
    if (head.next == NULL){
        head.next = cur;
    } else{
        PCBNode *temp = head.next;
        while(temp->next != NULL){
            temp = temp->next; 
        }
        temp->next = cur;
    }
    

    // load code into frame in memory
    char line[1000];
    int lineNumber = 1;

    FILE *p = fopen(cur->fileName, "rt"); // the program is in a file

    // get to the line to be loaded
    while(lineNumber <= (cur->page_table_index)*3){
        fgets(line, 999, p);
        lineNumber++;
    }

    char *lines[FRAME_SIZE];

    // get lines to be loaded
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (fgets(line, 999, p) == NULL) {
            // if already eof, then this line is "none"
            lines[i] = "none";
        }
        else {
            lines[i] =  strdup(line);
            lineNumber++;
        }
    }

 

    // update page table of the process
    // frame_set_value returns frame index or error code of INT_MAX
    cur->page_table[page_table_index] = frame_set_value(lines[0], lines[1], lines[2]);
    moveFrameToEndOfLRU(cur->page_table[page_table_index]);
    //printf("Frame at the head of the LRU is: %d\n", cur->page_table[page_table_index]);
    
    // if error code, return error code and stop
        if (page_table[page_table_index] == INT_MAX) {
        fclose(p);
        return page_table[page_table_index];
    }

    fclose(p);
    return 0;
}

// RR Scheduler
int RR(int numLines)
{
    int errCode = 0;

    // while there is still  script in the ready queue
    while(head.next != NULL){

        // cur points to the first process
        PCBNode *cur = head.next;
        
        // remove cur from the ready queue
        head.next = cur->next;
        cur->next = NULL;

        int counter = 0;
        int value = 0;
        
        if((cur->script_length - cur->pc) < numLines){
            value = (cur->script_length - cur->pc);
        } else {
            value = numLines;
        }

        int page_table[PAGE_TABLE_SIZE]; 
        memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE); 
        int page_table_index = cur->page_table_index;

        // checking for page fault
        if(page_table[page_table_index] == -1){
            // handle page fault
            int errCode = handleRRPageFault(cur);
            continue;
        }
        
        memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE);
        page_table_index = cur->page_table_index;

        char *lines[FRAME_SIZE];
        moveFrameToEndOfLRU(page_table[page_table_index]);

        char **result = malloc(3 * sizeof(char *));
        frame_get_value(page_table[page_table_index], result);
        memcpy(lines, result, sizeof(char *) * FRAME_SIZE);
        free(result);

       // memcpy(lines, frameGetValue(page_table[page_table_index]), sizeof(char *) * FRAME_SIZE);

        // execute upto 2 commands
        while(counter < value){

            int lineNumber = cur->pc;
           
            int index = lineNumber % FRAME_SIZE;
            
            if (strcmp(lines[index], "none") != 0) {
                errCode = parseInput(lines[index]);
            } else {
                //print_memory();
                if(cur->pc < cur->script_length){
                    if(!isEmptyFrameAvailable()){
                        printf("Page fault! Victim page contents:\n");
                        int frameID = findFirstAvailableHole();
                        getFrameLines(frameID);
                        printf("End of victim page contents.\n");

                        // clear the frame to be able to laod new page into the frame
                        frame_clear_value(frameID);

                        // update the page table of the evicted page
                        updatePageTableOfEvictedPage(frameID);
                        
                    }

                    int page_table[PAGE_TABLE_SIZE]; 
                    memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE); 
                    int page_table_index = cur->page_table_index + 1;
                    
                    char line[1000];
                    int lineNumber = 1;

                    FILE *p = fopen(cur->fileName, "rt"); // the program is in a file

                    // get to the line to be loaded
                    while(lineNumber <= (cur->page_table_index + 1)*3){
                        fgets(line, 999, p);
                        lineNumber++;
                    }

                    char *lines[FRAME_SIZE];

                    // get lines to be loaded
                    for (int i = 0; i < FRAME_SIZE; i++) {
                        if (fgets(line, 999, p) == NULL) {
                            // if already eof, then this line is "none"
                            lines[i] = "none";
                        }
                        else {
                            lines[i] =  strdup(line);
                            lineNumber++;
                        }
                    }

                   

                    // update page table of the process
                    // frame_set_value returns frame index or error code of INT_MAX
                    cur->page_table[page_table_index] = frame_set_value(lines[0], lines[1], lines[2]);
                    memcpy(page_table, cur->page_table, sizeof(int) * PAGE_TABLE_SIZE);
                    moveFrameToEndOfLRU(page_table[page_table_index]);
                    // if error code, return error code and stop
                        if (page_table[page_table_index] == INT_MAX) {
                        fclose(p);
                        return page_table[page_table_index];
                    }

                    fclose(p);
                    moveToEndOfReadyQueue(cur);
                    break;
                }
            }

            if (lineNumber % FRAME_SIZE == FRAME_SIZE - 1) {
                page_table_index = cur->page_table_index + 1;

                char **result = malloc(3 * sizeof(char *));
                frame_get_value(page_table[page_table_index], result);
                memcpy(lines, result, sizeof(char *) * FRAME_SIZE);
                free(result);

                // memcpy(lines, frameGetValue(page_table[page_table_index]), sizeof(char *) * FRAME_SIZE);
            }

            cur->pc = cur->pc + 1;
            counter++;
        }

        cur->page_table_index = page_table_index;
       
        
        // add current process to end of ready queue if not implemented fully
        if(cur->pc < cur->script_length){

            // add cur to the end of the ready queue
            if(head.next == NULL){
                head.next = cur;
            }else{
                PCBNode *temp = head.next;
                while(temp->next != NULL){
                    temp = temp->next; 
                }
                temp->next = cur;
            }
            moveToEndOfReadyQueue(cur);
        } else {
            cleanUp(cur);
        }
    }
    return errCode;
}


int my_ls()
{
    int n = 0;
    struct dirent *d;

    DIR *dir1 = opendir(".");

    // credits for checking empty directory
    // https://stackoverflow.com/questions/6383584/check-if-a-directory-is-empty-using-c-on-linux
    while ((d = readdir(dir1)) != NULL)
    {
        if (++n > 2)
            break;
    }
    if (n <= 2)
    {
        // Empty directory
        closedir(dir1);
        return 1;
    }

    // credits for looping through directory
    // https://www.youtube.com/watch?v=j9yL30R6npk&t=565s

    DIR *dir = opendir(".");
    struct dirent *object;
    object = readdir(dir);

    typedef struct node
    {
        char *name;
        struct node *next;
    } node;

    node *head = NULL;

    // Create Linked List
    while (object != NULL)
    {
        if (!strcmp(object->d_name, "."))
        {
            // don't include "."
            object = readdir(dir);
        }

        if (!strcmp(object->d_name, ".."))
        {
            // don't include ".."
            object = readdir(dir);
        }

        // Create Node
        node *new;
        new = (node *)malloc(sizeof(node));
        new->name = object->d_name;
        new->next = NULL;

        if (head == NULL)
        {
            head = new;
        }
        else
        {
            node *temp;
            temp = head;
            while (temp->next != NULL)
            {
                temp = temp->next;
            }
            temp->next = new;
        }

        free(new);

        object = readdir(dir);
    }

    // Sort Linked List
    node *i;
    node *j;

    for (i = head; i != NULL; i = i->next)
    {
        for (j = i->next; j != NULL; j = j->next)
        {
            if (tolower(i->name[0]) > tolower(j->name[0]))
            {
                // swap
                char *prevName = i->name;
                i->name = j->name;
                j->name = prevName;
            }
            else if (tolower(i->name[0] < tolower(j->name[0])))
            {
                continue;
                // nothing to do
            }
            else
            {
                // They start with the same letter - upper case or lower case
                if (strncmp((i->name), (j->name), 1) == 32)
                {
                    // i is lower-case and j is upper-case
                    char *prevName = i->name;
                    i->name = j->name;
                    j->name = prevName;
                }
                else
                {
                    // They're both lower-case or upper-case so can be sorted normally
                    if (strcmp(i->name, j->name) > 0)
                    {
                        char *prevName = i->name;
                        i->name = j->name;
                        j->name = prevName;
                    }
                }
            }
        }
    }

    // Printing the linked list
    node *temp;
    temp = head;
    while (temp != NULL)
    {
        printf("%s\n", temp->name);
        temp = temp->next;
    }

   
    node *ptr;
    temp = head;
    while (temp != NULL)
    {
        ptr = temp;
        temp = temp->next;
        free(ptr);
    }

    closedir(dir);
}

int my_touch(char *filename)
{
    FILE *f;
    f = fopen(filename, "w");
    fclose(f);
    return 0;
}

int my_mkdir(char *dirname)
{
    // Checking if dirname is a shell variable
    if (dirname[0] == '$')
    {
        // dirname is a shell variable

        char *newDirname = mem_get_value(dirname + 1);

        if (strcmp("Variable does not exist", newDirname) == 0)
        {
            // credits for mkdir(dirname, 0777)
            // https://www.geeksforgeeks.org/create-directoryfolder-cc-program/
            printf("Bad command: my_mkdir\n");
            return 1;
        }
        else
        {
            mkdir(newDirname, 0777);
        }
    }
    else
    {
        // dirname is not a shell variable
        mkdir(dirname, 0777);
    }
    return 0;
}

int my_cd(char *dirname)
{

    // credits for checking directory does not exist
    // https://stackoverflow.com/questions/6383584/check-if-a-directory-is-empty-using-c-on-linux
    struct dirent *d;
    DIR *dir = opendir(dirname);
    if (dir == NULL)
    {
        // If directory does not exit
        printf("Bad command: my_cd\n");
        return 1;
    }
    chdir(dirname);
}

int one_liner(char *commands[], int size, int numberOfCommands)
{
    int starting_index = 0;

    int command_size = 0;

    for (int i = 0; i < size; i++)
    {
        if ((strcmp(commands[i], "\0") == 0) || ((i + 1) == size))
        {

            // reached the end of a command
            if ((i + 1) == size)
            {
                command_size++;
            }

            char *Newcommand[command_size];

            // form the individual command
            for (int j = 0; j < command_size; j++)
            {
                Newcommand[j] = commands[starting_index + j];
            }

            // Run the command
            interpreter(Newcommand, command_size);

            starting_index = i + 1;

            command_size = 0;
        }
        else
        {
            command_size++;
        }
    }
    return 0;
}

// copies script to another script
int copy_script_file(char* src_file_path, char* dest_file_path) 
{
  FILE* src_file = fopen(src_file_path, "r");

  if (src_file == NULL) {
    return badcommandFileDoesNotExist();
  }


  FILE* dest_file = fopen(dest_file_path, "w");

  if (dest_file == NULL) {
    printf("Error opening dest in copy_script_file\n");
    return 8;
  }

  int c;
  while ((c = fgetc(src_file)) != EOF) {
    // checking for one-liners
    if(c == ';'){
        fputc('\n', dest_file);
    } else{
        fputc(c, dest_file);
    }
    
  }

  fclose(src_file);
  fclose(dest_file);
  return 0;
}