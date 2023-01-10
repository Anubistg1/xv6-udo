#include "types.h"
#include "user.h"
int main(int argc, char *argv[])
{
    int IsChild;
    char *ChildScreen[] = { "sh", 0 };
    IsChild = fork();
    if (IsChild < 0) {        
        exit();
    }
    if (IsChild == 0) { // Child
       // exec("sh", argv);
      //  cprintf("Is Child\n");
        screen();
        exec("sh", ChildScreen);   
       // syscall();
       // exec("sh", argv);
       // consoleinit();        
    }
    if (IsChild > 0) // Parent
    {
        
        exit();
    }

    
   // exec("sh.exe",0);
    exit();
}