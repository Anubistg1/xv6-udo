#include "types.h"
#include "user.h"
int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (strcmp("-r", argv[1]) == 0)
        {
            shutdown(0);
            exit();
        }       
    }
    else
    {
    shutdown(1);
    exit();
    }
    

}