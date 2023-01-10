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
        screen();
       // syscall();
       // exec("sh", argv);
       // consoleinit();        
        exit();
    }
    if (IsChild > 0) // Parent
    {
        exit();
    }

    
   // exec("sh.exe",0);
    exit();
}