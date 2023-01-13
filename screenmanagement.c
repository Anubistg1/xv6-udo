#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"


//struct screenmanagement
//{
//    int ScreenNum;
//    uint ScreenBuffer[(SCREENWIDTH*SCREEENHEIGHT*2) - 1];
//    int CursorPos;
//};

//struct 
//{
//    struct spinlock ScreenLock;
//    struct screenmanagement ScreenNumber[MAXSCREENS];
    
//} ConsoleTable;
//struct screenmanagement *Screen;
//struct screenmanagement* ScreenAllocation(void) 
//{
    

  //  acquire(&ConsoleTable.ScreenLock);
  //  for (Screen = ConsoleTable.ScreenNumber; Screen < ConsoleTable.ScreenNumber + MAXSCREENS; Screen++)
  //  {
    //    if (Screen->ScreenNum == 0)
    //    {
           // Screen->ScreenNum = 1;
         //   release(&ConsoleTable.ScreenLock);
       //     return Screen;
     //   }
        
   // }
 //   release(&ConsoleTable.ScreenLock);
//    return 0;
//};

//void CreateScreen(void)
//{

 //   ScreenAllocation(); 

    
// };
