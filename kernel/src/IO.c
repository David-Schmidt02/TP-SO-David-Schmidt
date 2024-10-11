#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
/
void interfaz() {
    
    while(milisec)
    {
        char *linea;
        while (1) {
            linea = readline(">");
            if (!linea) {
                break;
            }
            if (linea) {
                add_history(linea);
            }
            if (!strncmp(linea, "exit", 4)) {
                free(linea);
                break;
            }
            printf("%s\n", linea);
            free(linea);
        }
    return 0;    
    }
    
}
