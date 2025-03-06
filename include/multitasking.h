 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#define STACK_SIZE 4096
#define QUANTUM 6
#define IDLE 20

/* We define these the same for all machines.
   Changes from this to the outside world should be done in `_exit'.  */
#define	EXIT_FAILURE	1	/* Failing exit status.  */
#define	EXIT_SUCCESS	0	/* Successful exit status.  */


#include <common/types.h>
#include <gdt.h>

namespace myos
{

	struct CPUState
	{
		// What is the name of the registers? 
		common::uint32_t eax;	// eax: Accumulator register
		common::uint32_t ebx;	// ebx: Base register
		common::uint32_t ecx;	// ecx: Counter register
		common::uint32_t edx;	// edx: Data register

		common::uint32_t esi;	// esi: Source index
		common::uint32_t edi;	// edi: Destination index
		common::uint32_t ebp;	// ebp: Base pointer, stack frame

		/*
		common::uint32_t gs;	// gs: Segment register
		common::uint32_t fs;	// fs: Segment register
		common::uint32_t es;	// es: Destination Segment (used for MOVS, etc.)
		common::uint32_t ds;	// ds: Data Segment (used for MOV)
		*/
		/* This is pushed by the processor only when a exception occures */
		common::uint32_t error;	// error: Error code

		/* These below have been pushed by the processor in the interrupt */
		common::uint32_t eip;	// eip: Instruction pointer
		common::uint32_t cs;	// cs: Code Segment (used for IP)
		common::uint32_t eflags;// eflags: Flags register	
		common::uint32_t esp;	// esp: Stack pointer
		common::uint32_t ss;	// ss: Stack Segment (used for SP)
	} __attribute__((packed));
	
	enum TaskState
	{
		READY,
		RUNNING,
		WAITING,
		STOPPED,
		TERMINATED
	};

	enum Priority
	{
		HIGHEST = 1,
		HIGHER = 2,
		HIGH = 3,
		MEDIUM = 6,
		LOW = 12,
		LOWER = 18,
		LOWEST = 30
	};

	class Task
	{
		friend class TaskManager;
		private:
			common::uint8_t stack[STACK_SIZE]; // 4 KiB
			CPUState* cpustate;
			TaskState taskState;
			Priority priority;
			common::uint32_t passedQuantums;
			bool dynamiclyAdjustedPriority;
			common::pid_t pid;
			common::pid_t ppid;
			common::uint32_t status;
			common::pid_t waitingPid;
			common::pid_t waitedByPid;
			int *waitingStatusLocation;

		public:
			Task(GlobalDescriptorTable *gdt, void entrypoint());
			Task(const Task &other);
			common::pid_t getPid();
			common::pid_t getPPid();
			TaskState getTaskState();
			CPUState* getCPUState();
			~Task();
	};
	
	
	class TaskManager
	{
		private:
			Task* tasks[256];
			int numTasks;
			int currentTask;
			common::pid_t pidCounter;
			GlobalDescriptorTable *gdt;
			Task *idleTask;
			bool prioritiesUpdated;

		public:
			TaskManager(GlobalDescriptorTable *gdt);
			~TaskManager();
			Task *getCurrentTask();
			bool AddTask(Task* task);
			bool RemoveTask(Task* task);
			bool ChangePriority(Task *task, Priority priority);
			Task *getTaskByPid(common::pid_t pid);
			CPUState *Schedule(CPUState *cpustate);
			void printProcessTable();
			void printPriority(Priority priority);
			void exitTask(int exit_status);
			common::pid_t forkTask(myos::common::uint32_t esp);
			common::pid_t waitpid(common::pid_t __pid, int *__stat_loc, int __options);
			int execve(void (*entrypoint)());
			int SetDynamicPriority(common::pid_t __who, bool flag);
	};

	common::uint64_t get_performance_counter();
	int rand_r ();
	void srand_r (unsigned int seed);

}

#endif
