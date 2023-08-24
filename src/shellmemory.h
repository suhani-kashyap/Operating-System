#define FRAME_SIZE 3


void mem_init();
char *mem_get_value(char *var);
int mem_set_value(char *var, char *value);
void mem_clear_value(char *var);
void mem_clear_backing_store(char *files[], int backing_store_count);
void frame_clear_value(int frame_number);
int frame_set_value(char *line1, char *line2, char *line3);
void frame_get_value(int frame_number, char **result);
void print_memory();
int findFirstAvailableHole();
void getFrameLines(int frameID);
int isEmptyFrameAvailable();
void moveFrameToEndOfLRU(int frame);
void printLRU();
int clear_LRU();