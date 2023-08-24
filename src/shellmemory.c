#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#ifndef FRAME_STORE_SIZE
#error "frame store size not defined"
#endif

#ifndef VAR_STORE_SIZE
#error "var mem size not defined"
#endif


struct memory_struct{
	char *var;
	char *value;
};

// Queue to keep track of the least recently used frame
typedef struct LRUNode{
	int frameID; 
	struct LRUNode *next;
} LRUNode;

LRUNode *LRUhead = NULL;

// also defined in shellmemory.h
// Each frame will store 3 lines of code
#define FRAME_SIZE 3

struct frame{
	int frame_number; // index of frame
	int active; // whether or not the frame is empty -> 0 for empty, 1 for being used

	// when programmatically defining frame size, might need to convert this to an array instead
	char *line1;
	char *line2;
	char *line3;
};

// Using two different data structures to store frames and variables
// For storing variables
struct memory_struct var_store[VAR_STORE_SIZE];

// For storing frames
// note this means 1000 frames, not 333 frames. Each frame stores 3 lines
struct frame frame_store[FRAME_STORE_SIZE/3];

int ShellMemoryOutOfSpace(char *var) {
	printf("Shell memory has run out of space and variable %s could not be added to memory\n", var);
	return INT_MAX;
}

// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extracted value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get to =

	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);

	value[j]='\0';
	
	return strdup(value);
}

void delete_directory(char* dir_path) {

	// Remove all files and subdirectories within the directory
    char *cmd = malloc(strlen(dir_path) + 10); 
    sprintf(cmd, "rm -rf %s/*", dir_path); 
    system(cmd);
    free(cmd);

	// delete current directory
    rmdir(dir_path);
}

void printLRU(){
	LRUNode *cur = LRUhead;
	while(cur != NULL){
		printf("%d -> ", cur->frameID);
		cur = cur->next;
	}
	printf("NULL\n");
}

void moveFrameToEndOfLRU(int frame){

	// loop to the frame we want and also the frame before it in the LRU
	
	LRUNode *temp = LRUhead;
	LRUNode *cur = NULL;
	
	if(LRUhead->frameID == frame){
		cur = LRUhead;
		cur->next == NULL;
		LRUhead = LRUhead->next;
	}
	else{
		LRUNode *before = NULL;
		while(temp->next->frameID != frame && temp->next != NULL){
			temp = temp->next;
		}
		cur = temp->next;
		before = temp;
		before->next = cur->next;
		cur->next = NULL;
	
	}

	temp = LRUhead;
	while(temp->next != NULL){
		temp = temp->next;
	}
	temp->next = cur;
	

	// now temp points to the last LRUNode
	
	cur->next = NULL;
	temp->next = cur;
	//printLRU();
}
// Shell memory functions

void mem_init(){

	int i;

	// init var memory
	for (i=0; i<VAR_STORE_SIZE; i++){	
		var_store[i].var = "none";
		var_store[i].value = "none";
	}

	// init frame memory
	for (i=0; i<(FRAME_STORE_SIZE/3); i++){	
		frame_store[i].frame_number = i;
		frame_store[i].active = 0;
		frame_store[i].line1 = "none";
		frame_store[i].line2 = "none";
		frame_store[i].line3 = "none";
	}

	// init LRU queue
	for( i=0; i<(FRAME_STORE_SIZE/3); i++){
		LRUNode *new = (LRUNode *)malloc(sizeof(LRUNode));
		new->frameID = i;
		new->next = NULL;
		if(LRUhead == NULL){
			LRUhead = new;
		} else {
			LRUNode *temp = LRUhead;
			while(temp->next != NULL){
				temp = temp->next;
			}
			temp->next = new;
		}
	}
	
	delete_directory("backing_store");
	mkdir("backing_store", 0777);
}

void mem_clear_backing_store(char *files[], int backing_store_count) {
	for(int i = 0; i < backing_store_count; i++){
		free(files[i]);
	}
	delete_directory("backing_store");
}


// Clears a key/value pair in memory
void mem_clear_value(char *var_in) {
	int i;

	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(var_store[i].var, var_in) == 0){
			var_store[i].var = "none";
			var_store[i].value = "none";
			return;
		} 
	}
	return;

}

// clears a frame
void frame_clear_value(int frame_number) {

	int i;

	for (i=0; i < (FRAME_STORE_SIZE/3); i++){
		if (frame_number == frame_store[i].frame_number){
			frame_store[i].active = 0;
			frame_store[i].line1 = "none\n";
			frame_store[i].line2 = "none\n";
			frame_store[i].line3 = "none\n";
			return;
		} 
	}
	return;

}

// Set key value pair
int mem_set_value(char *var_in, char *value_in) {
	
	int i;

	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(var_store[i].var, var_in) == 0){
			var_store[i].value = strdup(value_in);
			return 0;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(var_store[i].var, "none") == 0){
			var_store[i].var = strdup(var_in);
			var_store[i].value = strdup(value_in);
			return 0;
		} 
	}

	return ShellMemoryOutOfSpace(var_in);
}

// stores a frame. Returns INT_MAX if error code, else returns frame_number
int frame_set_value(char *line1, char *line2, char *line3) {
	
	int i;

	// need to find a free spot.
	for (i=0; i<(FRAME_STORE_SIZE/3); i++){
		if (frame_store[i].active == 0){
			frame_store[i].active = 1;
			frame_store[i].line1 = line1;
			frame_store[i].line2 = line2;
			frame_store[i].line3 = line3;
			
			return frame_store[i].frame_number;
		} 
	}

	return ShellMemoryOutOfSpace("frame");
}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;

	for (i=0; i<1000; i++){
		if (strcmp(var_store[i].var, var_in) == 0){

			return strdup(var_store[i].value);
		} 
	}
	return "Variable does not exist";

}

// gets frame values - returns with index 0 "none" if no such value
// if a frame is not full, the remaining lines are "none"
void frame_get_value(int frame_number, char **result) {
	int i;

	result[0] = "none";

	for (i=0; i<(FRAME_STORE_SIZE/3); i++){
		if (frame_store[i].frame_number == frame_number){
            result[0] = frame_store[i].line1;
            result[1] = frame_store[i].line2;
            result[2] = frame_store[i].line3;
		} 
	}


}

// NOTE: Also defined in shellemory.h
void print_memory(){
	printf("FRAME MEMORY: \n");

	for(int i = 0; i < (FRAME_STORE_SIZE/3); i++){
		printf("Frame #%d\n", i);
		printf("%s\n", frame_store[i].line1);
		printf("%s\n", frame_store[i].line2);
		printf("%s\n", frame_store[i].line3);
	}
}

// Finds the first available hold in Physical Memory
int findFirstAvailableHole(){
	return LRUhead->frameID;
}

// prints the lines in the evicted frame
void getFrameLines(int frameID){
	printf("\n");
	printf("%s", frame_store[frameID].line1);
	printf("%s", frame_store[frameID].line2);
	printf("%s", frame_store[frameID].line3);
	printf("\n");
}

// returns 1 if an empty frame already exists in the memory else returns 0
int isEmptyFrameAvailable(){

	if(frame_store[LRUhead->frameID].active == 0 && !(strcmp(frame_store[LRUhead->frameID].line1, "none") && strcmp(frame_store[LRUhead->frameID].line2, "none") && strcmp(frame_store[LRUhead->frameID].line3, "none"))){
		return 1;
	}

	return 0;
}

// clears the malloc memory created by LRU
int clear_LRU(){
	LRUNode *ptr;
    LRUNode *temp = LRUhead;
    while (temp != NULL)
    {
        ptr = temp;
        temp = temp->next;
        free(ptr);
    }
	return 1;
}
