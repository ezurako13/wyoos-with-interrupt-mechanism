
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

void printf(char*);
void printfHex(uint8_t);
void printfHex32(uint32_t);


// !Important syscall icin 80e 20 eklemesini cozdum
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber)
{
	// // todo sil bunlari
	// printf("SyscallHandler: ");
	// printfHex(InterruptNumber);
	// printf("\n");

}

SyscallHandler::~SyscallHandler()
{
}



uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
	CPUState* cpu = (CPUState*)esp;
	

	switch(cpu->eax)
	{
		case EXIT:
			this->getTaskManager()->exitTask(cpu->ebx);
			esp = (uint32_t)this->getTaskManager()->Schedule(cpu);
			break;
		case FORK:
			// todo sil bunu
			// printf("Forking...\n");
			cpu->ecx = this->getTaskManager()->forkTask(esp);
			// return (uint32_t)this->getTaskManager()->Schedule((CPUState *)esp);
			break;
		case PRINTF:
			printf((char*)cpu->ebx);
			break;
		case PRINTFHEX:
			printfHex(cpu->ebx);
			break;
		case WAITPID:
			cpu->eax = this->getTaskManager()->waitpid(cpu->ebx, (int*)cpu->ecx, cpu->edx);
			if(this->getTaskManager()->getCurrentTask()->getTaskState() == WAITING)
				esp = (uint32_t)this->getTaskManager()->Schedule(cpu);
			break;
		case EXECVE:
			cpu->ecx = this->getTaskManager()->execve((void (*)())cpu->ebx);
			esp = (uint32_t)this->getTaskManager()->getCurrentTask()->getCPUState();
			break;
		case GETPID:
			cpu->ecx = this->getTaskManager()->getCurrentTask()->getPid();
			break;
		case GETPPID:
			cpu->ecx = this->getTaskManager()->getCurrentTask()->getPPid();
			break;
		case SET_PRIORITY:
			cpu->edx = this->getTaskManager()->ChangePriority(this->getTaskManager()->getTaskByPid((pid_t)cpu->ebx), (Priority)cpu->ecx);
			break;
		case SET_DYNAMIC_PRIORITY:
			cpu->edx = this->getTaskManager()->SetDynamicPriority((pid_t)cpu->ebx, cpu->ecx);
			break;
		
		default:
			break;
	}

	
	return esp;
}

/* Terminate program execution with the low-order 8 bits of STATUS.  */
void myos::exit(int exit_status)
{
	asm("int $0x80" : : "a" (EXIT), "b" (exit_status));
}

/* Clone the calling process, creating an exact copy.
   Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
pid_t myos::fork()
{
	pid_t pid;

	// // todo cok zor durumdaydim
	// // print real stack pointer
	// uint32_t esp;
	// asm volatile("movl %%esp, %0" : "=r" (esp));
	// printf("Current ESP: "); printfHex32(esp); printf("\n");

	asm("int $0x80" : "=c" (pid) : "a" (FORK));
	return pid;
}

/* Wait for a child matching PID to die.
   If PID is greater than 0, match any process whose process ID is PID.
   If PID is (pid_t) -1, match any process.
   If PID is (pid_t) 0, match any process with the
   same process group as the current process.
   If PID is less than -1, match any process whose
   process group is the absolute value of PID.
   If the WNOHANG bit is set in OPTIONS, and that child
   is not already dead, return (pid_t) 0.  If successful,
   return PID and store the dead child's status in STAT_LOC.
   Return (pid_t) -1 for errors.  If the WUNTRACED bit is
   set in OPTIONS, return status for stopped children; otherwise don't.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
pid_t myos::waitpid(pid_t __pid, int *__stat_loc, int __options)
{
	pid_t ret;
	asm volatile (
		"int $0x80"
		: "=a" (ret), "=m" (*__stat_loc)
		: "0" (WAITPID), "b" (__pid), "c" (__stat_loc), "d" (__options)
		: "memory"
	);

	if (ret < 0) {
		return -1;
	}

	return ret;
}

/* Replace the current process with given program. */
int myos::execve(void entrypoint())
{
	int error = 0;
	asm("int $0x80" : "=c" (error) : "a" (EXECVE), "b" (entrypoint));
	return error;
}

/* Get the process ID of the calling process.  */
extern pid_t myos::getpid (void)
{
	pid_t pid;
	asm("int $0x80" : "=c" (pid) : "a" (GETPID));
	return pid;
}

/* Get the process ID of the calling process's parent.  */
extern pid_t myos::getppid (void)
{
	pid_t pid;
	asm("int $0x80" : "=c" (pid) : "a" (GETPID));
	return pid;
}

/* Set the priority of all processes specified by WHICH and WHO (see above)
   to PRIO.  Returns 0 on success, -1 on errors.  */
int myos::setpriority(pid_t __who, Priority __prio)
{
	int result;
	asm("int $0x80" : "=d" (result) : "a" (SET_PRIORITY), "b" (__who), "c" (__prio));
	return result;
}

/* Set the dynamic priority of all processes specified by WHICH and WHO (see above)
   to PRIO.  Returns 0 on success, -1 on errors.  */
int myos::setdynamicpriority(pid_t __who, bool flag)
{
	int result;
	asm("int $0x80" : "=d" (result) : "a" (SET_DYNAMIC_PRIORITY), "b" (__who), "c" (flag));
	return result;
}

