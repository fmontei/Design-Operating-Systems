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

    char entry[200];
    int uid = 54321, pid = 12345;
    long unsigned int regs_ax = 1312, nr_mkdir = 5243;
    char *regs_di = "regs_di";

    sprintf(entry, "my sysmon_intercept_before: uid = %d, " 
		"pid = %d, regs->ax = %lu, regs->di = %s, __NR_mkdir: %lu\n",
                uid, pid, regs_ax, regs_di, nr_mkdir); 
    printf("length = %d\n", (int) strlen(entry));
    add_entry_to_log(entry);

    printf("log = %s", log);

    
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

