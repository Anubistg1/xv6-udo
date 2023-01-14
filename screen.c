#include "types.h"
#include "user.h"
int main(int argc, char *argv[])
{
    int IsChild;
    IsChild = fork();
    if (IsChild < 0) {        
        exit();
    }
    if (IsChild == 0) { // Child
       // exec("sh", argv);
      //  cprintf("Is Child\n");
        char *argv2[] = { "sh", 0 }; // only needed here
        screen();
        exec("sh", argv2);
           
       // syscall();
       // exec("sh", argv);
       // consoleinit();        
    }
    if (IsChild > 0) // Parent
    {
        wait(); // waits for forked children to keep running
        exit();
    }

    
   // exec("sh.exe",0);
    exit();
}