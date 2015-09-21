#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOG_MAX_LENGTH 1024

static char *log; 
static char *temp;

void substring(char s[], char sub[], int p, int l);

void add_entry_to_log(char *entry)
{
    if (strlen(log) + strlen(entry) < LOG_MAX_LENGTH)
    {
        strcat(log, entry);
    }
    else if (strlen(log) + strlen(entry) >= LOG_MAX_LENGTH)
    {
        char token = '\n';
        int i = 0; 
        while (token != log[i])
        {
            i++;
        }
        i += 2; // Include \n character
        
        temp = (char *) malloc(sizeof(char) * LOG_MAX_LENGTH);
        substring(log, temp, i, LOG_MAX_LENGTH - i);
        
        free(log);
        log = temp;
        
        if (strlen(log) + strlen(entry) < LOG_MAX_LENGTH)
            strcat(log, entry);
        else if (strlen(log) + strlen(entry) >= LOG_MAX_LENGTH)
            add_entry_to_log(entry);
    }
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
 
   while (c < l) {
      sub[c] = s[p + c - 1];
      c++;
   }
}

int main(void) 
{
    log = (char *) malloc(sizeof(char) * LOG_MAX_LENGTH);
    char array[2] = "0\0";
    printf("array length = %d\n", (int) strlen(array));
    if (array[0] == '0' || array[0] == '1') 
    {
        printf("found a number!\n");
    }
    return 0;
}

