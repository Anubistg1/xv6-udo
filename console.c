// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define INPUT_BUF 128
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory need to grab this for 
struct kbdbuffer {
    char buf[INPUT_BUF];
    uint r;  // Read index
    uint w;  // Write index
    uint e;  // Edit index
};

struct kbdbuffer inputBuffer;

struct kbdbuffer * input = 0;

// Extra stuff for multiple screens
// static int CurrentScreen;
struct 
{
    struct spinlock ScreenLock;
    struct screenmanagement Screen[MAXSCREENS - 1];
    
} ConsoleTable;

#define C(x)  ((x) - '@')  // Control-x



static void consputc(int);

static int panicked = 0;

static struct {
    struct spinlock lock;
    int locking;
} cons;

static void printint(int xx, int base, int sign) {
    static char digits[] = "0123456789abcdef";
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0)) {
        x = -xx;
    }
    else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % base];
    }
    while ((x /= base) != 0);

    if (sign) {
        buf[i++] = '-';
    }

    while (--i >= 0) {
        consputc(buf[i]);
    }
}

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...) {
    int i, c, locking;
    uint *argp;
    char *s;

    locking = cons.locking;
    if (locking) {
        acquire(&cons.lock);
    }

    if (fmt == 0) {
        panic("null fmt");
    }

    argp = (uint*)(void*)(&fmt + 1);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            consputc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0) {
            break;
        }
        switch (c) {
            case 'd':
                printint(*argp++, 10, 1);
                break;
            case 'x':
            case 'p':
                printint(*argp++, 16, 0);
                break;
            case 's':
                if ((s = (char*)*argp++) == 0) {
                    s = "(null)";
                }
                for (; *s; s++) {
                    consputc(*s);
                }
                break;
            case '%':
                consputc('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                consputc('%');
                consputc(c);
                break;
        }
    }

    if (locking) {
        release(&cons.lock);
    }
}

void panic(char *s) {
    int i;
    uint pcs[10];

    cli();
    cons.locking = 0;
    // use lapiccpunum so that we can call panic from mycpu()
    cprintf("lapicid %d: panic: ", lapicid());
    cprintf(s);
    cprintf("\n");
    getcallerpcs(&s, pcs);
    for (i = 0; i < 10; i++) {
        cprintf(" %p", pcs[i]);
    }
    panicked = 1; // freeze other CPU
    for (;;) {
        ;
    }
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4


static void cgaputc(int c) {
    int pos;
   // int CurrentScreen;
    
    if ((ConsoleTable.Screen->ScreenCurrentUse == 1) || myproc() == 0) //Checks if the screen is the one that isn't and is a process
    {
        
    // Cursor position: col + 80*row.
    outb(CRTPORT, 14);
    pos = inb(CRTPORT + 1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT + 1);

    if (c == '\n') {
        pos += 80 - pos % 80;
    }
    else if (c == BACKSPACE) {
        if (pos > 0) {
            --pos;
        }
    }
    else {
        crt[pos++] = (c & 0xff) | 0x0700;  // black on white

    }
    if (pos < 0 || pos > 25 * 80) {
        panic("pos under/overflow");
    }

    if ((pos / 80) >= 24) { // Scroll up.
        memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
        pos -= 80;
        memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
    }

    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    crt[pos] = ' ' | 0x0700;

    }

    // Virtual buffer
    // Cursor position: col + 80*row.
    // If define is wrong
    if ((ConsoleTable.Screen->ScreenCurrentUse == 1) && myproc() != 0) //Writes when is a process
    {
    
    int NewPos = ConsoleTable.Screen->CursorPos;
    outb(CRTPORT, 14);
    NewPos = inb(CRTPORT + 1) << 8;
    outb(CRTPORT, 15);
    NewPos |= inb(CRTPORT + 1);

    if (c == '\n') {
        NewPos += 80 - NewPos % 80;
    }
    else if (c == BACKSPACE) {
        if (NewPos > 0) {
            --NewPos;
        }
    }
    else {
        ConsoleTable.Screen->ScreenBuffer[NewPos++] = (c & 0xff) | 0x0700;  // black on white

    }
    if (NewPos < 0 || NewPos > 25 * 80) {
        panic("pos under/overflow");
    }

    if ((NewPos / 80) >= 24) { // Scroll up.
        memmove(ConsoleTable.Screen->ScreenBuffer, ConsoleTable.Screen->ScreenBuffer + 80, sizeof(ConsoleTable.Screen->ScreenBuffer[0]) * 23 * 80);
        NewPos -= 80;
        memset(ConsoleTable.Screen->ScreenBuffer + NewPos, 0, sizeof(ConsoleTable.Screen->ScreenBuffer[0]) * (24 * 80 - NewPos));
        ConsoleTable.Screen->CursorPos = NewPos;
    }
    // This has caused double spacing so has been commented out
   // outb(CRTPORT, 14);
   // outb(CRTPORT + 1, NewPos >> 8);
   // outb(CRTPORT, 15);
    // outb(CRTPORT + 1, NewPos);
   // ConsoleTable.ScreenNumber->ScreenBuffer[NewPos] = ' ' | 0x0700;
   
    }


}

void consputc(int c) {
    if (panicked) {
        cli();
        for (;;) {
            ;
        }
    }

    if (c == BACKSPACE) {
        uartputc('\b');
        uartputc(' ');
        uartputc('\b');
    }
    else {
        uartputc(c);
    }
    cgaputc(c);
}

int consoleget(void) {
    int c;

    acquire(&cons.lock);

    while ((c = kbdgetc()) <= 0) {
        if (c == 0) {
            c = kbdgetc();
        }
    }

    release(&cons.lock);

    return c;
}

void consoleintr(int (*getc)(void)) {
    int c, doprocdump = 0;

    acquire(&cons.lock);
    while ((c = getc()) >= 0) {
        switch (c) {
            case C('P'):  // Process listing.
                // procdump() locks cons.lock indirectly; invoke later
                doprocdump = 1;
                break;
            case C('U'):  // Kill line.
                while (input->e != input->w &&
                       input->buf[(input->e - 1) % INPUT_BUF] != '\n') {
                    input->e--;
                    consputc(BACKSPACE);
                }
                break;
            case C('H'):
            case '\x7f':  // Backspace
                if (input->e != input->w) {
                    input->e--;
                    consputc(BACKSPACE);
                }
                break;
            default:
                if (c != 0 && input->e - input->r < INPUT_BUF) {
                    c = (c == '\r') ? '\n' : c;
                    input->buf[input->e++ % INPUT_BUF] = c;
                    consputc(c);
                    if (c == '\n' || c == C('D') || input->e == input->r + INPUT_BUF) {
                        input->w = input->e;
                        wakeup(&(input->r));
                    }
                }
                break;
        }
    }
    release(&cons.lock);
    if (doprocdump) {
        procdump();  // now call procdump() wo. cons.lock held
    }
}

int consoleread(struct inode *ip, char *dst, int n) {
    uint target;
    int c;

    iunlock(ip);
    target = n;
    acquire(&cons.lock);
    while (n > 0) {
        while (input->r == input->w) {
            if (myproc()->killed) {
                release(&cons.lock);
                ilock(ip);
                return -1;
            }
            sleep(&(input->r), &cons.lock);
        }
        c = input->buf[input->r++ % INPUT_BUF];
        if (c == C('D')) { // EOF
            if (n < target) {
                // Save ^D for next time, to make sure
                // caller gets a 0-byte result.
                input->r--;
            }
            break;
        }
        *dst++ = c;
        --n;
        if (c == '\n') {
            break;
        }
    }
    release(&cons.lock);
    ilock(ip);

    return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n) {
    int i;

    iunlock(ip);
    acquire(&cons.lock);
    for (i = 0; i < n; i++) {
        consputc(buf[i] & 0xff);
    }
    release(&cons.lock);
    ilock(ip);

    return n;
}

void consoleinit(void) {
    initlock(&cons.lock, "console");

    // Initialise pointer to point to our console input buffer
    input = &inputBuffer;

    devsw[CONSOLE].write = consolewrite;
    devsw[CONSOLE].read = consoleread;
    cons.locking = 1;

    ioapicenable(IRQ_KBD, 0);
}


int ScreenAvailablity(void)
{
    int UsedScreens = 0;
    acquire(&ConsoleTable.ScreenLock);
    for (int i = 0; i < (MAXSCREENS - 1); i++)
    {
        if (ConsoleTable.Screen[i].ScreenInUse == 1)
        {
            UsedScreens = UsedScreens + 1;
        }
        if (ConsoleTable.Screen[i].ScreenInUse == 0)
        {
            release(&ConsoleTable.ScreenLock);
            cprintf("%d", i);
            return 1;
        }
        
    }
    release(&ConsoleTable.ScreenLock);
    
    return 0;
}

struct screenmanagement* ScreenAllocation(void) 
{
    struct screenmanagement  *TempScreen;
    
    int SetID = 0;

  
    acquire(&ConsoleTable.ScreenLock);
    for (TempScreen = ConsoleTable.Screen; TempScreen < ConsoleTable.Screen + (MAXSCREENS - 1); TempScreen++)
    {
        if (TempScreen->ScreenInUse == 0)
        {
            TempScreen->ScreenInUse = 1;
            TempScreen->ScreenCurrentUse = 1;
            TempScreen->ScreenNum = SetID;
            
            // int CurrentScreen = SetID;
           //  struct proc* CurrentProc = myproc();
           //  CurrentProc->ConsoleID = CurrentScreen;
            // cprintf("Child Process\n");
           //  int id = myproc()->ConsoleID;
           //  cprintf("%d", id);
           //  cprintf("%d", CurrentScreen);
            // ConsoleTable.ScreenNumber[id].ScreenInUse = 1;
            // ConsoleTable.ScreenNumber[id].ScreenCurrentUse = 1;
            // got child ID
            release(&ConsoleTable.ScreenLock);
            return TempScreen;
        }
        SetID++;
    }
        
    release(&ConsoleTable.ScreenLock);
    return 0;
};

void CreateScreen(void)
{
    int pos = 0;
    memset(ConsoleTable.Screen->ScreenBuffer, 0x00, sizeof(crt[0]) * 23 * 80);
    memmove(&crt, ConsoleTable.Screen->ScreenBuffer, sizeof(crt[0]) * 23 * 80); // moves crt to screen buffer
    
    // sorts cursor position
    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    crt[pos] = ' ' | 0x0700;
    ConsoleTable.Screen->CursorPos = pos;

    return;
};

// deallocate
void ScreenDeallocation(int id)
{
    struct screenmanagement  *TempScreen;
    TempScreen = ConsoleTable.Screen;
    acquire(&ConsoleTable.ScreenLock);
    if (TempScreen->ScreenNum != 0)
    {

        if (TempScreen->ScreenCurrentUse == 1)
        {
            TempScreen->ScreenInUse = 0;
            TempScreen->ScreenCurrentUse = 0;
            TempScreen->ScreenNum = id;
            

            release(&ConsoleTable.ScreenLock);
            return;
        } 
    }
    release(&ConsoleTable.ScreenLock);
    return;
    // Code that over complicated what I was doing.
    // acquire(&ConsoleTable.ScreenLock);
   // ConsoleTable.Screen[id].ScreenInUse = 0;
    // ConsoleTable.Screen[id].ScreenCurrentUse = 0;
   // ConsoleTable.Screen[id].CursorPos = 0;
   // release(&ConsoleTable.ScreenLock);
   // ConsoleTable.ScreenNumber[id].ScreenBuffer = 0;
   //return 0;
    
}

// previous attempt at allocation
   // for (int i = 0; i < MAXSCREENS - 1; i++)
   // {
   //     if ((ConsoleTable.Screen[i].ScreenCurrentUse = 1))
   //     {
   //         memmove(ConsoleTable.Screen[i].ScreenBuffer, crt, sizeof(crt[0]) * 23 * 80);
   //         cprintf("%d", ConsoleTable.Screen[i].ScreenBuffer);
   //     }
        
   // }
    
   // for (int i = 0; i < MAXSCREENS - 1; i++)
   // {
        
   //     if ((ConsoleTable.ScreenNumber[i].ScreenNum = 0) && (ConsoleTable.ScreenNumber[i].ScreenInUse = 0)) // Checks to see if the first screen is alreadly assigned
   //     {
   //         cprintf("made it here checked first screen");
            // If the first screen isn't assigned then assign it
            // ConsoleTable.ScreenNumber[i].VideoBuffer = (ushort*)P2V(0xb8000);
   //         memmove(ConsoleTable.ScreenNumber[i].ScreenBuffer, crt, sizeof(crt[0]) * 23 * 80);
   //         ConsoleTable.ScreenNumber[i].CursorPos = 0; // cgaputc(int c)
   //         ConsoleTable.ScreenNumber[i].ScreenInUse = 1;
   //         ConsoleTable.ScreenNumber[i].ScreenCurrentUse = 1;
   //    }
   //    if ((ConsoleTable.ScreenNumber[i].ScreenInUse = 0))
   //     {
   //         cprintf("set new screen");
            // Sets old screen data            
           // ConsoleTable.ScreenNumber[i - 1].VideoBuffer = (ushort*)P2V(0xb8000);
   //         memmove(ConsoleTable.ScreenNumber[i - 1].ScreenBuffer, crt, sizeof(crt[0]) * 23 * 80);
   //         ConsoleTable.ScreenNumber[i - 1].CursorPos = 0; // cgaputc(int c)
   //         ConsoleTable.ScreenNumber[i - 1].ScreenInUse = 1;
   //         ConsoleTable.ScreenNumber[i - 1].ScreenCurrentUse = 0;

            // Sets new parameters
    //        memmove(ConsoleTable.ScreenNumber[i].ScreenBuffer, crt, sizeof(crt[0]) * 23 * 80);
     //       ConsoleTable.ScreenNumber[i].CursorPos = 0; // cgaputc(int c)
       //     ConsoleTable.ScreenNumber[i].ScreenInUse = 1;
       //     ConsoleTable.ScreenNumber[i].ScreenCurrentUse = 1;
       //     cprintf("%d", i);
       // }
        
   // }
    


    // Initial attempt at handing the buffers


  //  for (int i = 0; i < (MAXSCREENS - 1); i++)
  //  {
  //      if (ConsoleTable.Screen[i].ScreenCurrentUse == 1)
  //      {
  //          if (i != 0)
  //          {
  //              memmove(crt, ConsoleTable.Screen[i - 1].ScreenBuffer, sizeof(crt[0]) * 23 * 80);
  //              memset(ConsoleTable.Screen[i].ScreenBuffer, 0, sizeof(crt[0]) * 23 * 80);
  //          }
  //          else
  //          {
  //              memmove(crt, ConsoleTable.Screen[i].ScreenBuffer, sizeof(crt[0]) * 23 * 80);
  //          }
           
  //      }
        
  //  }
    

    