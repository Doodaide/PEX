#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

int main(void)
{
	char str[] = "123;";
	if(str == NULL)
    {
		printf("Null\n");
        return 0;
    }
    char * end; 
    strtol(str, &end, 10);
    if( *end != '\0' || end == NULL || str == end)
    {
        printf("Bruh\n");
    }
	printf("No issues: \n");
    
    return 0;
}
