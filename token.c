#include <stdlib.h>
#include <string.h>

//parses an command into strings
extern int parseString(char *inputStr, char ***parsedCmd) { 
    char *str, *token, *saveptr;
    char **parsedStr = (char**) calloc(15, sizeof(char*));
    int idx;

    for (idx = 0, str = inputStr; ; idx++, str = NULL) {
        token = strtok_r(str, " ", &saveptr);
        if (token == NULL)
            break;
        parsedStr[idx] = token;
    }
    parsedStr[idx] = NULL; // NULL terminated
    *parsedCmd = parsedStr;

    return idx;
}
