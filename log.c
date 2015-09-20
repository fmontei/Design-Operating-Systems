#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LENGTH 1024

static char *log;
static char *temp;

void add_entry_to_log(char *entry);
void substring(char s[], char sub[], int p, int l);

int main(void) 
{
    log = (char *) malloc(sizeof(char) * MAX_LENGTH);

    int i = 0;
    for (; i < 1000000; i++) { 
        char *entry = "Hello world!\n";   
        add_entry_to_log(entry);

        entry = "Hello world again!\n";
        add_entry_to_log(entry);

        entry = "Felipe Monteiro\n";
        add_entry_to_log(entry);

        entry = "Blah Blah\n";
        add_entry_to_log(entry);

        entry = "random characters random characters\n";
        add_entry_to_log(entry);

        entry = "random!!!!\n";
        add_entry_to_log(entry);

        printf("log =\n%s", log);
    }

    return 0;
}

void add_entry_to_log(char *entry)
{
    if (strlen(log) + strlen(entry) < MAX_LENGTH)
    {
        strcat(log, entry);
    }
    else if (strlen(log) + strlen(entry) >= MAX_LENGTH)
    {
        char token = '\n';
        int i = 0; 
        while (token != log[i])
        {
            i++;
        }
        i += 2; // Include \n character
        
        temp = (char *) malloc(sizeof(char) * MAX_LENGTH);
        substring(log, temp, i, MAX_LENGTH - i);
        
        free(log);
        log = temp;
        
        if (strlen(log) + strlen(entry) < MAX_LENGTH)
            strcat(log, entry);
        else if (strlen(log) + strlen(entry) >= MAX_LENGTH)
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

