#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "interpreter.h"
#include "shellmemory.h"

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[])
{
    printf("%s\n\n", "Shell version 1.2 Created January 2023");

    // adding extra printout in shell for frame size and var mem size
    printf("Frame Store Size = %d; Variable Store Size = %d", FRAME_STORE_SIZE, VAR_STORE_SIZE);
    
    char prompt = '$';              // Shell prompt
    char userInput[MAX_USER_INPUT]; // user's input stored here
    int errorCode = 0;              // zero means no error, default

    // init user input
    for (int i = 0; i < MAX_USER_INPUT; i++)
        userInput[i] = '\0';

    // init shell memory
    mem_init();
    
    // 0 = false, 1 = true
    int eof = 0;
    int batchMode = 0;

    if (isatty(fileno(stdin)) == 0)
    {
        // batch mode
        batchMode = 1;
        printf("\n");
    }
    else {
        help();
    }

    while (eof == 0)
        {
            if (batchMode == 0) {
                printf("%c ", prompt);
            }

            eof = feof(stdin);

            // skip if eof
            if (eof == 0) {
                fgets(userInput, MAX_USER_INPUT - 1, stdin);

                errorCode = parseInput(userInput);
                if (errorCode == -1)
                    exit(99); // ignore all other errors
            }
            memset(userInput, 0, sizeof(userInput));
        }

    return 0;
}

/*int parseInput(char ui[])
{
    char tmp[200];
    char *words[100];
    int a = 0;
    int b;
    int w = 0; // wordID
    int errorCode;
    for (a = 0; ui[a] == ' ' && a < 1000; a++)
        ; // skip white spaces
    while (ui[a] != '\n' && ui[a] != '\0' && a < 1000)
    {
        for (b = 0; ui[a] != ';' && ui[a] != '\0' && ui[a] != '\n' && ui[a] != ' ' && a < 1000; a++, b++)
        {
            tmp[b] = ui[a];
            // extract a word
        }
        tmp[b] = '\0';
        words[w] = strdup(tmp);
        w++;
        if (ui[a] == '\0')
            break;
        a++;
    }
    errorCode = interpreter(words, w);
    return errorCode;
}
*/

int parseInput(char *ui) {
    char tmp[200];
    char *words[100];                            
    int a = 0;
    int b;                            
    int w=0; // wordID    
    int errorCode;
    for(a=0; ui[a]==' ' && a<1000; a++);        // skip white spaces
    for(int i=0; i<100; i++) words[i] = NULL;
    
    while(a<1000 && a<strlen(ui) && ui[a] != '\n' && ui[a] != '\0') {
        while(ui[a]==' ') a++;
        if(ui[a] == '\0') break;
        for(b=0; ui[a]!=';' && ui[a]!='\0' && ui[a]!='\n' && ui[a]!=' ' && a<1000; a++, b++) tmp[b] = ui[a];
        tmp[b] = '\0';
        if(strlen(tmp)==0) continue;
        if(words[w]!=NULL){
            free(words[w]);
        }
        words[w] = strdup(tmp);
        if(ui[a]==';'){
            w++;
            errorCode = interpreter(words, w);
            if(errorCode == -1) return errorCode;
            a++;
            w = 0;
            for(; ui[a]==' ' && a<1000; a++);        // skip white spaces
            continue;
        }
        w++;
        a++; 
    }
    errorCode = interpreter(words, w);
    for(int i=0; i<100; i++) {
        if(words[i]!=NULL) free(words[i]);
    }
    return errorCode;
}