//
// Public API of jtrace for third parties
//  
// License: Absolutely free for NON-COMMERCIAL use. 
// 
// If you want to use this commercially, Please talk to me. 
// 
// You might want to consider reading my books, or going
// to a training at www.technologeeks.com, where we teach 
// this stuff and lots more.
//
//
#include <sys/types.h>

#ifndef JTRACE_API_H
#define JTRACE_API_H


#ifndef _32
typedef unsigned long uint64_t;
#endif

#ifdef _32
#define arch_int uint32_t
#else

#define arch_int uint64_t
#endif


//
// Abstracting the calling convention:
//
// JTrace will automatically translate these register "numbers" into the corresponding
// registers of the underlying architecture, as per the ABI. In the ARM64 case, these
// are straightforward noops - ARG_0...ARG_7 are X0..X7. In ARM32, X0..X3 are noops,
// but jtrace is smart enough to get the other "register" values from the stack. In the
// Intel case, the mapping is between rdi/rsi/rdx/rcx/r8/r9 or the stack.
//
// Likewise, the syscall number - in Intel, that's eax/rax. In ARM32/ARM64 r7/r8 respectively
//
// This enables you to get the arguments in a plugin without bothering to consider how the 
// calling convention works!

#define REG_ARG_0	0
#define REG_ARG_1	1
#define REG_ARG_2	2
#define REG_ARG_3	3
#define REG_ARG_4	4 // ARM32: stack
#define REG_ARG_5	5
#define REG_ARG_6	6
#define REG_ARG_7	7

// these are intentionally values greater than ARM64's registers,
// so we are guaranteed to map them by ABI
#define REG_ARG_RETVAL		33
#define REG_ARG_SYSCALL_NUM	34

// The actual register representation is the per architecture pt_regs,
// but that's made opaque by this here struct
typedef struct jtregs regs_t;

// the opaque structure will be provided for you (by reference) when jtrace
// calls your callback handler, which needs to be implemented as:

typedef enum {
	DO_NOTHING = 0, // You want JTrace to continue as normal
	BLOCK 	   = 1, // You want JTrace to block this system call
} syscall_handler_ret_t;

typedef syscall_handler_ret_t (*syscall_handler_func) (regs_t *, int exit, int pid);

enum syscallHandlerFlags {
	
	HANDLER_ENTRY_ONLY = 1,
	HANDLER_EXIT_ONLY  = 2,
	HANDLER_OVERRIDE_DEFAULT = 1024,
};

// piece de resistance
int registerSyscallHandler (char *SyscallName, // must exist or you'll get -1 
	     syscall_handler_func Handler,     // Your callback, per above
		              int Bitness,     // 32, 64 or 0 (both)
			      int Flags);      // as per flags, above

#define ERR_SYSCALL_NOT_FOUND	(-1)

// The idea is to then leave the regs_t opaque, and use the accessor:
long getRegValue (regs_t *regs, int num, int is32);

// setRegValue - calls GETREGS, changes your value, then calls SETREGS
int setRegValue (regs_t *regs, int num, uint64_t value, int is32);

// This conveniently hides process_vm_readv, /proc/pid/mem, or ptrace
int readProcessMemory (int Pid, arch_int Addr, int Size, char *MyAddr);

//--- 3/30/2017 ----

// And this can set the process memory, writing Size bytes from MyAddr to Addr!
int writeProcessMemory (int Pid, arch_int Addr, int Size, char *MyAddr);

// Get an FD name. This is useful for operating in read/write hooks only
// if the fd is connected to a certain file
char *getFDName(int fd);


//--- 3/26/2018 One year later --

typedef int binderHook_t (char *Parcel , size_t Size, int Code);

int registerBinderIoctlHook(char *interface, // UTF-8 or UTF-16 OK
			    binderHook_t	Hook);
			

// Is JTrace's current config colorful?

int config_color() ;
//--------------------------------------------------------------------

// Other APIs I use for testing
//-----------------------------

// Get a system call number by providing its name. This is especially useful
// for testing, because God only knows why the numbers keep changing across
// various platforms.

int getSyscallNum (char *);


// For testing
void dumpArgumentsForSyscall (char *);
void dumpAllSyscalls(int); 



int get_is32(void);
#endif
