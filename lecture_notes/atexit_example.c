#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
bye(void)
{
    printf("That was all, folks\n");
}

int
main(void)
{
    long a;
    int i;

    a = sysconf(_SC_ATEXIT_MAX);
    printf("ATEXIT_MAX = %ld\n", a);
   
    i = atexit(bye); // i = 0;
    /// this is not allowed. (SIGNAL FAULT)
    // *(int *)NULL = 0; 
    /// and the atexit() won't called
    printf("this line is after the registration of atexit\n"); 
    if (i != 0) {
        fprintf(stderr, "cannot set exit function\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS); // calls the registrator function and prints;
    /*************************************************
    *atexit((void *)func (void)) registates the given function 
    *and calls it when exit() is called 
    *or when main ends  
    **************************************************/
}
