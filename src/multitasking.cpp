
#include <multitasking.h>

using namespace myos;
using namespace myos::common;


void printf(char*);
void printfHex(uint8_t key);
void printfHex32(uint32_t key);
// todo gereksizleri kaldir
void taskC();
void idle();

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
	cpustate = (CPUState*)(stack + STACK_SIZE - sizeof(CPUState));
	
	cpustate -> eax = 0;
	cpustate -> ebx = 0;
	cpustate -> ecx = 0;
	cpustate -> edx = 0;

	cpustate -> esi = 0;
	cpustate -> edi = 0;
	cpustate -> ebp = 0;
	
	/*
	cpustate -> gs = 0;
	cpustate -> fs = 0;
	cpustate -> es = 0;
	cpustate -> ds = 0;
	*/
	
	// cpustate -> error = 0;    
   
	// cpustate -> esp = ;
	cpustate -> eip = (uint32_t)entrypoint;
	cpustate -> cs = gdt->CodeSegmentSelector();
	// cpustate -> ss = ;
	cpustate -> eflags = 0x202;

	taskState = STOPPED;
	priority = MEDIUM;
	pid = 0;
	ppid = 0;
	waitingPid = 0;
	waitedByPid = 0;
	status = 0;
}

Task::Task(const Task &other)
{
	for(uint32_t i = 0; i < STACK_SIZE; i++)
		this->stack[i] = other.stack[i];

	uint32_t cpustateOffset = (uint32_t)other.cpustate - (uint32_t)(other.stack);
	this->cpustate = (CPUState*)(((uint32_t)(this->stack)) + cpustateOffset);

	this->ppid = other.pid;
	this->waitedByPid = other.waitedByPid;
	this->waitingPid = other.waitingPid;
}

Task::~Task()
{
}

pid_t Task::getPid()
{
	return pid;
}

pid_t Task::getPPid()
{
	return ppid;
}

TaskState Task::getTaskState()
{
	return taskState;
}

CPUState* Task::getCPUState()
{
	return cpustate;
}


TaskManager::TaskManager(GlobalDescriptorTable *gdt)
{
	numTasks = 0;
	currentTask = numTasks - 1;
	this->gdt = gdt;
	pidCounter = numTasks + 1;
	prioritiesUpdated = false;

	idleTask = new Task(gdt, idle);
	idleTask->waitedByPid = -1;
	idleTask->pid = -1;
	idleTask->taskState = READY;


	// todo kernelin da calismasini istiyosan numTasksi bi arttir ve bunlari ac
	// tasks[0] = new Task(gdt, NULL);
	// tasks[0]->pid = 1;
	// tasks[0]->taskState = RUNNING;
}

TaskManager::~TaskManager()
{
	// for(int i = 0; i < numTasks; i++)
	//     delete tasks[i];
	// todo sil bunu
	printf("TaskManager deleted\n");
}

Task *TaskManager::getCurrentTask()
{
	return tasks[currentTask];
}

/**
 * Add a task to the task list with sorting by priority
*/
bool TaskManager::AddTask(Task* task)
{
	// // todo sil bunu
	// printf("AddTask:\n");
	if(numTasks >= 256)
		return false;
		
	task->pid = pidCounter++;
	task->taskState = READY;
	tasks[numTasks++] = task;


	if(currentTask > 0 && tasks[currentTask]->priority > task->priority)
	{
		++currentTask;
	}

	// Now shift the task to the correct position in the task list 
	// based on priority. Except for the first task. (In ascending order)
	if(numTasks > 1)
	{
		int i = numTasks - 1;

		// priority farkliligindan dolayi islem sirasi degisecekse bunu isaretle
		if(i > 0 && tasks[i]->priority < tasks[i - 1]->priority)
			this->prioritiesUpdated = true;

		while(i > 0 && tasks[i]->priority < tasks[i - 1]->priority)
		{
			Task* temp = tasks[i];
			tasks[i] = tasks[i - 1];
			tasks[i - 1] = temp;
			i--;
		}
	}


	// // todo sil bunlari
	// printf("TaskManager: ");
	// printfHex32(numTasks);
	// printf("\n");

	return true;
}

/**
 * Remove a task from the task list
*/
bool TaskManager::RemoveTask(Task* task)
{
	if(numTasks <= 0)
		return false;

	uint16_t i = 0;
	
	while(i < numTasks && tasks[i] != task)
		i++;

	if(i == numTasks)
		return false;
	
	// todo bunu duzelt
	// delete tasks[i];

	while(i < numTasks - 1)
	{
		tasks[i] = tasks[i + 1];
		i++;
	}

	numTasks--;

	return true;	
}

/**
 * Change the priority of a task and sort the task list by priority
*/
bool TaskManager::ChangePriority(Task *task, Priority yeniPriority)
{
	if(numTasks <= 0)
		return false;

	if(task->priority == yeniPriority)
		return true;

	int i = 0;
	
	while(i < numTasks && tasks[i] != task)
		i++;

	printf((char *)(i == numTasks ? "Task not found\n" : "Task found\n"));

	if(i == numTasks)
		return false;

	bool isCurrentTask = tasks[currentTask] == task;
	Priority eskiPriority = task->priority;
	task->priority = yeniPriority;

	task->passedQuantums = 0;
	this->prioritiesUpdated = true;

	// printf("Eski: "); printPriority(eskiPriority); printf(" Yeni: "); printPriority(yeniPriority); printf("\n");
	// printf((char *)(isCurrentTask ? "current task" : "not current task"));
	// printf("\n");
	
	// printf("basta currentTask: "); printfHex(currentTask); printf(" "); printfHex(tasks[currentTask]->pid); printf("\n");

	// If it will change position of current task, then update it
	if(i > currentTask && tasks[currentTask]->priority > yeniPriority)
		++currentTask;
		
	else if(i < currentTask && tasks[currentTask]->priority < yeniPriority)
		--currentTask;

	// printf("ortada currentTask: "); printfHex(currentTask); printf(" "); printfHex(tasks[currentTask]->pid); printf("\n");


	// Now shift the task to the correct position in the task list
	// based on priority. Except for the first task. (In ascending order)
	if(numTasks > 1)
	{
		// prioritysi yukselmisse
		if(eskiPriority > yeniPriority)
		{
			while(i > 0 && tasks[i]->priority < tasks[i - 1]->priority)
			{
				Task* temp = tasks[i];
				tasks[i] = tasks[i - 1];
				tasks[i - 1] = temp;
				i--;
			}
		}
		// prioritysi dusmusse
		else if(eskiPriority < yeniPriority)
		{
			while(i < numTasks - 1 && tasks[i]->priority > tasks[i + 1]->priority)
			{
				Task* temp = tasks[i];
				tasks[i] = tasks[i + 1];
				tasks[i + 1] = temp;
				i++;
			}
		}
		if(isCurrentTask)
			currentTask = i;
	}

	// printf("sonda currentTask: "); printfHex(currentTask); printf(" "); printfHex(tasks[currentTask]->pid); printf("\n");


	return true;
}

/**
 * search a task by pid and return it
*/
Task *TaskManager::getTaskByPid(common::pid_t pid)
{
	for(uint32_t i = 0; i < numTasks; i++)
	{
		if(tasks[i]->pid == pid)
			return tasks[i];
	}

	return NULL;
}

/**
 * My schedulle function
*/
CPUState* TaskManager::Schedule(CPUState* cpustate)
{
	if(numTasks <= 0)
		return cpustate;

	// todo bunu yok et ya da printleri temizle

	/**
	 * Context switchler arasi zaman cok az oldugundan 
	 * taskleri bekletebilmek adina idle task ekledim
	*/
	static int idleTime = 0;
	// printf("idleTime: "); printfHex(idleTime); printf("\n");

	if(idleTask->taskState == RUNNING)
		idleTask->cpustate = cpustate;
	else if(idleTask->taskState == READY)
	{
		idleTask->taskState = RUNNING;
		if(currentTask >= 0)
			tasks[currentTask]->cpustate = cpustate;
	}

	if(++idleTime % IDLE != 0)
		return idleTask->cpustate;
	else
	{
		idleTime = 0;
		idleTask->taskState = READY;
	}
	
	// todo idle i yok ettiysen bunu ac
	// if(currentTask >= 0)
	// 	tasks[currentTask]->cpustate = cpustate;


	
	// static int ticks = 0;

	// // Only perform a context switch if the quantum has been reached
    // if (++ticks < QUANTUM && currentTask >= 0 && tasks[currentTask]->taskState == RUNNING)
	// {

    //     return cpustate;
	// }

	// ticks = 0;	// Reset the tick count

	// bool taskBulundu = false;

	// quantum time is used for priortize high priority tasks
	static int ticks = QUANTUM;

	// if the priorities are updated, then reset the ticks and current process counter
	if(currentTask >= 0 && prioritiesUpdated)
	{
		prioritiesUpdated = false;
		ticks = QUANTUM;

		if(tasks[currentTask]->taskState == RUNNING)
			tasks[currentTask]->taskState = READY;
		currentTask = -1;
	}

	// Only perform a context switch if the quantum has been reached
    if (currentTask >= 0 && (ticks -= tasks[currentTask]->priority) > 0 && 
			tasks[currentTask]->taskState == RUNNING)
	{
		printProcessTable();
        return cpustate;
	}

	ticks = QUANTUM;	// Reset the tick count
	
	int thisSchedulleStartedWith = currentTask < 0 ? 0 : currentTask;
	int thisSchedulleStartedWithPid = currentTask < 0 ? 0 : tasks[currentTask]->pid;
	bool tamTurAtildi = false;
	
	while(currentTask == -1 || tasks[currentTask]->taskState != READY)
	{
		if(++currentTask >= numTasks)
		{
			currentTask %= numTasks;
			tamTurAtildi = true;
		}

		// Low priority tasks are only executed if they are passed by certain number of quantums according to their priority
		while(tasks[currentTask]->priority != MEDIUM && tasks[currentTask]->passedQuantums < tasks[currentTask]->priority / QUANTUM)
		{
			tasks[currentTask]->passedQuantums++;

			if(++currentTask >= numTasks)
				currentTask %= numTasks;
		}


		tasks[currentTask]->passedQuantums = 0;
		
		// if(tasks[currentTask]->taskState == WAITING)
		// {
		// }

		// when a task appears to be TERMINATED, remove it from the task list and make the waiting task ready
		if(tasks[currentTask]->taskState == TERMINATED)
		{
			Task* awaitingTask = getTaskByPid(tasks[currentTask]->waitedByPid);
			if(awaitingTask != NULL)
			{
				if(awaitingTask->taskState == WAITING)
				{
					*(awaitingTask->waitingStatusLocation) = tasks[currentTask]->status;
					awaitingTask->taskState = READY;
					awaitingTask->waitingPid = 0;
				}
				RemoveTask(tasks[currentTask]);
				currentTask--;

				if(numTasks == 1)
					printProcessTable();

				// currentTask %= numTasks;
			}
		}

		// printfHex(currentTask); printf(" "); 
		// printf("curr: "); printfHex(tasks[currentTask]->pid); printf(" ");
		// printf("start: "); printfHex(thisSchedulleStartedWithPid); printf("\n"); 
		// printProcessTable();
		// printf("\r");
		
		/**
		 * Deadlock detection explanation: (Biraz Copilot biraz ben:) 
		 * If the current task is the same as the task that started the scheduler 
		 * and all the other task have been checked, then the current task is in deadlock. 
		 * This is because the current task is the only task that is running and
		 * it is not in the READY or RUNNING state. This means that the current task is 
		 * waiting for another task to be released.
		 * Therefore, the current task is in deadlock.
		*/
		if(tasks[currentTask]->pid == thisSchedulleStartedWithPid && tamTurAtildi)
		{
			if(tasks[currentTask]->taskState != RUNNING)
			{
				printfHex(currentTask);
				printf("Deadlock\n");
				return NULL;
			}
			else
			{
				// printProcessTable();
				// printf("Break ");
				break;
				// printf("atar misin\n");
			}
		}
	}

	// ticks -= tasks[currentTask]->priority;

	// only make previous task ready if it is running when schedulle started
	if(tasks[thisSchedulleStartedWith]->taskState == RUNNING)
		tasks[thisSchedulleStartedWith]->taskState = READY;
	tasks[currentTask]->taskState = RUNNING;

	// Only print the process table if the current task is not only task in the task list
	if(tasks[currentTask]->pid != thisSchedulleStartedWithPid || 
			tasks[currentTask]->taskState != RUNNING ||
			!tamTurAtildi)
		printProcessTable();

	// printfHex(currentTask); printf(" "); 
	// printf("x");
	// /// todo silmeyi unutma
	// printf("Schedulle: "); printfHex32((uint32_t)cpustate); printf(" -> "); printfHex32((uint32_t)tasks[currentTask]->cpustate); printf("\n");
	
	return tasks[currentTask]->cpustate;
}


void TaskManager::printProcessTable()
{
	// Print the process table
	printf("\rProcess Table:\n");
	printf("PID PPID    State   WDby WIng Priority\n");
	for (uint32_t i = 0; i < numTasks; i++)
	{
		printf(" ");
		printfHex(tasks[i]->pid);
		printf("  ");
		printfHex(tasks[i]->ppid);
		printf("  ");
		if (tasks[i]->taskState == READY)
			printf("   READY  ");
		else if (tasks[i]->taskState == RUNNING)
			printf("  RUNNING ");
		else if (tasks[i]->taskState == WAITING)
			printf("  WAITING ");
		else if (tasks[i]->taskState == STOPPED)
			printf("  STOPPED ");
		else if (tasks[i]->taskState == TERMINATED)
			printf("TERMINATED");
		printf("  ");
		printfHex(tasks[i]->waitedByPid);
		printf("   ");
		printfHex(tasks[i]->waitingPid);
		printf("  ");
		if(tasks[i]->priority == HIGHEST)
			printf(" HIGHEST");
		else if(tasks[i]->priority == HIGHER)
			printf(" HIGHER ");
		else if(tasks[i]->priority == HIGH)
			printf("  HIGH  ");
		else if(tasks[i]->priority == MEDIUM)
			printf(" MEDIUM ");
		else if(tasks[i]->priority == LOW)
			printf("  LOW   ");
		else if(tasks[i]->priority == LOWER)
			printf(" LOWER  ");
		else if(tasks[i]->priority == LOWEST)
			printf(" LOWEST ");

		printf("\n");
	}
}

void TaskManager::printPriority(Priority priority)
{
	if(priority == HIGHEST)
		printf("HIGHEST");
	else if(priority == HIGHER)
		printf("HIGHER");
	else if(priority == HIGH)
		printf("HIGH");
	else if(priority == MEDIUM)
		printf("MEDIUM");
	else if(priority == LOW)
		printf("LOW");
	else if(priority == LOWER)
		printf("LOWER");
	else if(priority == LOWEST)
		printf("LOWEST");
}

/* Terminate program execution with the low-order 8 bits of STATUS.  */
void TaskManager::exitTask(int exit_status)
{
	if(numTasks <= 0 || currentTask < 0)
		return;

	tasks[currentTask]->status = exit_status;
	tasks[currentTask]->taskState = TERMINATED;

	// todo bunu zaten schedulleda yapiyom
	// if(tasks[currentTask]->waitedByPid > 0)
	// {
	// 	Task* awaitingTask = getTaskByPid(tasks[currentTask]->waitedByPid);
	// 	if(awaitingTask != NULL)
	// 	{
	// 		*(awaitingTask->waitingStatusLocation) = exit_status;
	// 		awaitingTask->taskState = READY;
	// 		awaitingTask->waitingPid = 0;
	// 	}
	// }

	// todo sil bunu
	// printf("exitTask: "); printfHex32(tasks[currentTask]->pid); printf("\n");
}


/* Clone the calling process, creating an exact copy.
   Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
pid_t TaskManager::forkTask(uint32_t esp)
{
	CPUState *cpu = (CPUState*)esp;
	Task *yeniTask = new Task(this->gdt, NULL);
	Task *parent = tasks[currentTask];
	yeniTask->priority = parent->priority;

	if(!AddTask(yeniTask))
		return -1;

	// copy the stack of the parent process to the child process
	for(uint32_t i = 0; i < STACK_SIZE; i++)
		yeniTask->stack[i] = tasks[currentTask]->stack[i];
	

	// uint32_t cpustateOffset = esp - (uint32_t)(tasks[currentTask]->stack);
	// printf("cpustateOffset: "); printfHex32(cpustateOffset); printf("\n");


	// todo bu bir denemedir. VALLAHA OLDU. LAAAAAN
	uint32_t stackDifference = (uint32_t)yeniTask->stack - (uint32_t)parent->stack;
	// update the stack pointer in the cpu state of the child process
	yeniTask->cpustate = (CPUState*)(esp + stackDifference);
	// update the stack frame in order for correct restore of esp after the end of the fork syscall
	yeniTask->cpustate->ebp += stackDifference;


	// for(uint32_t i = 0; i < 40; i++)
	// {
	// 	if(yeniStackCurrentTop[i] > pStack && yeniStackCurrentTop[i] < pStackEnd)
	// 		yeniStackCurrentTop[i] = yeniStackCurrentTop[i] + stackDifference;
	// }


	// yeniTask->cpustate->esp = (uint32_t)yeniTask->stack + (cpu->esp - (uint32_t)tasks[currentTask]->stack);
	
	// *(yeniTask->cpustate) = *cpu;
	// assign the parent process id to the child process ppid
	yeniTask->ppid = tasks[currentTask]->pid;
	// update return value of fork of the child as 0 
	yeniTask->cpustate->ecx = 0;

	// yeniTask->cpustate->esp = (uint32_t)yeniTask->cpustate;


	// // todo bunu da sil
	// {
	// printf("      Parent:  Child:\n");
	// printf("eax:  "); printfHex32(cpu->eax); printf(" "); printfHex32(yeniTask->cpustate->eax); printf("\n");
	// printf("ebx:  "); printfHex32(cpu->ebx); printf(" "); printfHex32(yeniTask->cpustate->ebx); printf("\n");
	// printf("ecx:  "); printfHex32(cpu->ecx); printf(" "); printfHex32(yeniTask->cpustate->ecx); printf("\n");
	// printf("edx:  "); printfHex32(cpu->edx); printf(" "); printfHex32(yeniTask->cpustate->edx); printf("\n");
	// // printf("\n");
	// printf("esi:  "); printfHex32(cpu->esi); printf(" "); printfHex32(yeniTask->cpustate->esi); printf("\n");
	// printf("edi:  "); printfHex32(cpu->edi); printf(" "); printfHex32(yeniTask->cpustate->edi); printf("\n");
	// printf("ebp:  "); printfHex32(cpu->ebp); printf(" "); printfHex32(yeniTask->cpustate->ebp); printf("\n");
	// // printf("\n");
	// printf("error "); printfHex32(cpu->error); printf(" "); printfHex32(yeniTask->cpustate->error); printf("\n");
	// // printf("\n");
	// printf("eip:  "); printfHex32(cpu->eip); printf(" "); printfHex32(yeniTask->cpustate->eip); printf("\n");
	// printf("cs:   "); printfHex32(cpu->cs); printf(" "); printfHex32(yeniTask->cpustate->cs); printf("\n");
	// printf("eflag "); printfHex32(cpu->eflags); printf(" "); printfHex32(yeniTask->cpustate->eflags); printf("\n");
	// printf("esp:  "); printfHex32(cpu->esp); printf(" "); printfHex32(yeniTask->cpustate->esp); printf("\n");
	// printf("ss:   "); printfHex32(cpu->ss); printf(" "); printfHex32(yeniTask->cpustate->ss); printf("\n");
	// }

	// // todo yeni silinecekler
	// // printf("\r");
	// printf("child stack: "); 
	// // printf("\n"); 
	// printfHex32((uint32_t)yeniTask->stack); printf(" "); printfHex32((uint32_t)yeniTask->cpustate); printf(" "); printfHex32((uint32_t)(yeniTask->stack + 4096)); printf("\n");
	// printf("parent stack:"); 
	// // printf("\n");
	// printfHex32((uint32_t)tasks[currentTask]->stack); printf(" "); printfHex32((uint32_t)esp); printf(" "); printfHex32((uint32_t)(tasks[currentTask]->stack + 4096)); printf("\n");

	// // todo sil. Bu da ise yaramadi
	// printf("stack (last 40 element) "); printfHex32((uint32_t)tasks[currentTask]->stack); printf(" :\n");
	// for(uint32_t i = 0; i < 40; i++)
	// {
	// 	printfHex32(*((uint32_t *)yeniTask->cpustate + i)); 
	// 	printf("\n");
	// }

	
	// // todo sil bunlari
	// // forklananmis processe asla gecmemesini sagliyorum
	// // aman! sunu degistireyim de beni de dava etmesinler
	// yeniTask->pid = 13;
	// numTasks--;

	// // todo sil bunlari
	// // forklananmis processi parent processin yerine koyuyorum
	// yeniTask->pid = yeniTask->ppid;
	// tasks[currentTask] = yeniTask;
	// numTasks--;

	// // todo bunu da silmeyi unutma. Nalet olsun bu da yemedi
	// // bu odev imkansiz
	// // childdan sonra hemen parent a context switch yapmasin diye araya yeni bi task eklicem
	// Task* yeniTask2 = new Task(this->gdt, &taskC);
	// if(!AddTask(yeniTask2))
	// 	return -1;



	return yeniTask->pid;
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
pid_t TaskManager::waitpid(pid_t __pid, int *__stat_loc, int __options)
{
	if(numTasks <= 0 || currentTask < 0)
		return -1;

	if(__options != 0 && __options != WNOHANG && __options != WUNTRACED)
		return -1;
	
	if(__options == WUNTRACED)
	{
		printf("WUNTRACED is not supported yet\n");
		return -1;
	}
	
	// If PID is greater than 0, match any process whose process ID is PID.
	if(__pid > 0)
	{
		for(uint32_t i = 0; i < numTasks; i++)
		{
			if(tasks[i]->pid == __pid)
			{
				// If the child process is already dead, remove it from the task list and return its PID.
				if(tasks[i]->taskState == TERMINATED)
				{
					*__stat_loc = tasks[i]->status;
					if(RemoveTask(tasks[i]) == false)
					{
						printf("Error: Task not removed\n");
						return -1;
					}
					return __pid;
				}
				else
				{
					if(__options != WNOHANG)
					{
						tasks[currentTask]->taskState = WAITING;
						tasks[currentTask]->waitingStatusLocation = __stat_loc;
						tasks[currentTask]->waitingPid = __pid;
					}
					
					tasks[i]->waitedByPid = tasks[currentTask]->pid;
					return __pid;
				}
			}
		}
	}

	// If PID is (pid_t) -1, match any process.
	else if(__pid == -1)
	{
		for(uint32_t i = 0; i < numTasks; i++)
		{
			if(tasks[i]->taskState == TERMINATED)
			{
				*__stat_loc = tasks[i]->status;
				if(RemoveTask(tasks[i]) == false)
				{
					printf("Error: Task not removed\n");
					return -1;
				}
				return tasks[i]->pid;
			}
			else
			{
				if(__options != WNOHANG)
				{
					tasks[currentTask]->taskState = WAITING;
					tasks[currentTask]->waitingStatusLocation = __stat_loc;
					tasks[currentTask]->waitingPid = __pid;
				}
				
				tasks[i]->waitedByPid = tasks[currentTask]->pid;
				return tasks[i]->pid;
			}
		}
	}

	// If PID is (pid_t) 0, match any process with the
	// same process group as the current process.
	// OR
	// If PID is less than -1, match any process whose
	// process group is the absolute value of PID.
	else {
		printf("Groups are not supported yet\n");
		return -1;
	}

	// If we reach here, a child process was found. Return its PID.
	return __pid;
}


/* Replace the current process with given program. */
int TaskManager::execve(void (*entrypoint)())
{
	if(numTasks <= 0 || currentTask < 0)
		return -1;

	tasks[currentTask]->cpustate = (CPUState*)(tasks[currentTask]->stack + STACK_SIZE - sizeof(CPUState));
	tasks[currentTask]->cpustate->eip = (uint32_t)entrypoint;
	// Bu gerekliymis, Copilot: yoksa kernel panic oluyormus
	tasks[currentTask]->cpustate->cs = gdt->CodeSegmentSelector();
	// nolur nolmaz bunlari da ekliyim
	tasks[currentTask]->cpustate->ebp = 0;
	tasks[currentTask]->cpustate->eflags = 0x202;

	return 0;
}


int TaskManager::SetDynamicPriority(pid_t __who, bool flag)
{
	if(numTasks <= 0 || currentTask < 0)
		return -1;

	Task* task = getTaskByPid(__who);
	if(task == NULL)
		return -1;

	task->dynamiclyAdjustedPriority = flag;

	return 0;
}

uint64_t myos::get_performance_counter()
{
    unsigned int lo, hi;
    __asm__ __volatile__ (
        "rdtsc" : "=a" (lo), "=d" (hi)
    );
    return ((unsigned long long)hi << 32) | lo;
}



unsigned int seed = 0x31156131;

/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits.  */
int myos::rand_r ()
{
	unsigned int next = seed;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (unsigned int) (next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	seed = next;

	return result;
}


void myos::srand_r (unsigned int seed)
{
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	::seed = seed;
}


