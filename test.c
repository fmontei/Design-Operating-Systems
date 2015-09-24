#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LENGTH 128

static int num_omitted = 0;

int main(void)
{
    char *str = (char  *) malloc(sizeof(char) * MAX_LENGTH);
    char msg[] = "String omitted\n";
    strcpy(str, "Hello\nWorld\nString\nLast\n");
    
    int i, j;
    for (i = strlen(str) - 2; i >= 0; i--) {
        if (strlen(msg) == MAX_LENGTH - 1)
        {
        }
    }
    
    for (i = i + 1, j = 0; i < MAX_LENGTH; i++, j++)
    {
        if (j < strlen(msg)) 
        {
            str[i] = msg[j];
        }
        else
        {
            str[i] = ' ';
        }
    }
    printf("str = %s\n", str);
}
