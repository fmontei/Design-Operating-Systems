#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fwrite;
    fwrite = fopen("sysmon.backup", "a");

    if (fwrite == NULL)
    {
        fprintf(stderr, "Error writing file.\n");
        return EXIT_FAILURE;
    }
    
    fprintf(fwrite, "\n%s", argv[1]);

    fclose(fwrite);

    return EXIT_SUCCESS;
}
