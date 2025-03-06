
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

static int screen_height = 25;
static int screen_width = 80;
static int screen_halfwidth = 40;

// Nedense calismiyor
void setVideoMode()
{
    asm volatile (
        "movw $0x0003, %%ax\n"
        "int $0x10\n"
        "movw $0x1112, %%ax\n"
        "movb $0x00, %%bl\n"
        "int $0x10\n"
        : : : "ax", "bl"
    );

    screen_width = 80;
    screen_height = 50;
}


void printf(char* str)
{
	static uint16_t (*VideoMemory)[80] = (uint16_t(*)[80])0xb8000;

	static uint8_t x=0, y=0;
	static uint8_t x_base = 0;

	for(int i = 0; str[i] != '\0'; ++i)
	{
		switch(str[i])
		{
			case '\n':
				x = x_base;
				y++;
				break;
			case '\r':
				y = screen_height;
				break;
			default:
				VideoMemory[y][x] = (VideoMemory[y][x] & 0xFF00) | str[i];
				x++;
				break;
		}

		if(x - x_base >= screen_halfwidth)
		{
			x = x_base;
			y++;
		}

		if(y >= screen_height)
		{
			if(x_base == 40)
			{
				for(y = 0; y < screen_height; y++)
					for(x = 0; x < screen_width; x++)
						VideoMemory[y][x] = (VideoMemory[y][x] & 0xFF00) | ' ';
				x = 0;
				y = 0;
				x_base = 0;
				
			}
			else if(x_base == 0)
			{
				x = 40;
				y = 0;
				x_base = 40;
			}
		}
	}
}

void printfHex(uint8_t key)
{
	char* foo = "00";
	char* hex = "0123456789ABCDEF";
	foo[0] = hex[(key >> 4) & 0xF];
	foo[1] = hex[key & 0xF];
	printf(foo);
}

void printfHex16(uint16_t key)
{
	printfHex((key >> 8) & 0xFF);
	printfHex( key & 0xFF);
}

void printfHex32(uint32_t key)
{
	printfHex((key >> 24) & 0xFF);
	printfHex((key >> 16) & 0xFF);
	printfHex((key >> 8) & 0xFF);
	printfHex( key & 0xFF);
}

void printfDec32(uint32_t key)
{
	if(key == 0)
	{
		printf("0");
		return;
	}
	char* foo = "0000000000";
	int i = 9;
	while(key > 0)
	{
		foo[i] = '0' + key % 10;
		key /= 10;
		i--;
	}
	i++;
	printf(foo + i);
}



class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
	void OnKeyDown(char c)
	{
		char* foo = " ";
		foo[0] = c;
		printf(foo);
	}
};

class MouseToConsole : public MouseEventHandler
{
	int8_t x, y;
public:
	
	MouseToConsole()
	{
		uint16_t* VideoMemory = (uint16_t*)0xb8000;
		x = 40;
		y = 12;
		VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
							| (VideoMemory[80*y+x] & 0xF000) >> 4
							| (VideoMemory[80*y+x] & 0x00FF);        
	}
	
	virtual void OnMouseMove(int xoffset, int yoffset)
	{
		static uint16_t* VideoMemory = (uint16_t*)0xb8000;
		VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
							| (VideoMemory[80*y+x] & 0xF000) >> 4
							| (VideoMemory[80*y+x] & 0x00FF);

		x += xoffset;
		if(x >= 80) x = 79;
		if(x < 0) x = 0;
		y += yoffset;
		if(y >= 25) y = 24;
		if(y < 0) y = 0;

		VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
							| (VideoMemory[80*y+x] & 0xF000) >> 4
							| (VideoMemory[80*y+x] & 0x00FF);
	}
	
};

class PrintfUDPHandler : public UserDatagramProtocolHandler
{
public:
	void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
	{
		char* foo = " ";
		for(int i = 0; i < size; i++)
		{
			foo[0] = data[i];
			printf(foo);
		}
	}
};


class PrintfTCPHandler : public TransmissionControlProtocolHandler
{
public:
	bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
	{
		char* foo = " ";
		for(int i = 0; i < size; i++)
		{
			foo[0] = data[i];
			printf(foo);
		}
		
		
		
		if(size > 9
			&& data[0] == 'G'
			&& data[1] == 'E'
			&& data[2] == 'T'
			&& data[3] == ' '
			&& data[4] == '/'
			&& data[5] == ' '
			&& data[6] == 'H'
			&& data[7] == 'T'
			&& data[8] == 'T'
			&& data[9] == 'P'
		)
		{
			socket->Send((uint8_t*)"HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n",184);
			socket->Disconnect();
		}
		
		
		return true;
	}
};


void sysprintf(char* str)
{
	asm("int $0x80" : : "a" (PRINTF), "b" (str));
}

void sysprintfHex(uint8_t key)
{
	asm("int $0x80" : : "a" (PRINTFHEX), "b" (key));
}

// #define __LONG_LONG_MAX__ 0x7fffffffffffffffLL
// #define __INT32_MAX__ 0x7fffffff
#define time 26843545

void init();
void init1();
void init2();
void init3();
void init4();
void idle();

void taskA();
void taskB();
void taskC();
void testChildOfChild();
void collatzTask();
void binarySearchTestTask();
void linearSearchTestTask();
void llpTestTask();
int binarySearch(int arr[], int n, int x);
int linearSearch(int arr[], int n, int x);

void collatz(int n);
int long_running_program(int n);

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
	for(constructor* i = &start_ctors; i != &end_ctors; i++)
		(*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
	// printf("Hello World! --- http://www.AlgorithMan.de\n");

	// Switch to 80x25 text mode
	// setVideoMode();

	srand_r(get_performance_counter());

	GlobalDescriptorTable gdt;
	
	
	uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
	size_t heap = 10*1024*1024;
	MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);


	TaskManager taskManager(&gdt);
	
	// Task *task1 = new Task(&gdt, taskA);
	// Task *task2 = new Task(&gdt, taskB);
	// taskManager.AddTask(task1);
	// taskManager.AddTask(task2);
	
	// Task *initTask = new Task(&gdt, init);
	// taskManager.AddTask(initTask);
	
	// Task *init1Task = new Task(&gdt, init1);
	// taskManager.AddTask(init1Task);

	// Task *init2Task = new Task(&gdt, init2);
	// taskManager.AddTask(init2Task);

	Task *init3Task = new Task(&gdt, init3);
	taskManager.AddTask(init3Task);

	


	InterruptManager interrupts(0x20, &gdt, &taskManager);
	SyscallHandler syscalls(&interrupts, 0x80);

	// Exception0x0DHandler handler0x0D(&interrupts, 0x0D);

	// Bu calismiyo
	// setting pit frequency to 500 Hz (every 2 ms)
	// setting pit frequency to 1000 Hz (every 1 ms)
	// interrupts.setPITFrequency(1000);

	interrupts.Activate();

	while(1)
	{
		uint32_t i = 0;
		while(true)
		{
			if(i % time == 0)
				// sysprintf("K");
				// sysprintf("\n");
			++i;
		}
		// #ifdef GRAPHICSMODE
		// 	desktop.Draw(&vga);
		// #endif
	}
}

void idle()
{
	uint32_t i = 0;
	while(true)
	{
		// if(i % time == 0)
		// 	sysprintf("idle");
		// ++i;
	}
}


void init1()
{
	int choice = rand_r() % 4;
	void (*entrypoint)();
	if(choice == 0)
	{
		entrypoint = llpTestTask;
		printf("llpTestTask\n");	
	}
	else if(choice == 1)
	{
		entrypoint = collatzTask;
		printf("collatzTask\n");
	}
	else if(choice == 2)
	{
		entrypoint = binarySearchTestTask;
		printf("binarySearchTestTask\n");
	}
	else if(choice == 3)
	{
		entrypoint = linearSearchTestTask;
		printf("linearSearchTestTask\n");
	}

	for(int i = 0; i < 10; i++)
		entrypoint();
	
	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("i");
		++i;
	}
}

void init2()
{
	int choice = rand_r() % 4;
	void (*entrypoint1)();
	if(choice == 0)
	{
		entrypoint1 = llpTestTask;
		printf("llpTestTask\n");
	}
	else if(choice == 1)
	{
		entrypoint1 = collatzTask;
		printf("collatzTask\n");
	}
	else if(choice == 2)
	{
		entrypoint1 = binarySearchTestTask;
		printf("binarySearchTestTask\n");
	}
	else if(choice == 3)
	{
		entrypoint1 = linearSearchTestTask;
		printf("linearSearchTestTask\n");
	}

	choice = rand_r() % 4;
	void (*entrypoint2)() = entrypoint1;
	while(entrypoint2 != entrypoint1)
	{
		if(choice == 0)
			entrypoint2 = llpTestTask;
		else if(choice == 1)
			entrypoint2 = collatzTask;
		else if(choice == 2)
			entrypoint2 = binarySearchTestTask;
		else if(choice == 3)
			entrypoint2 = linearSearchTestTask;
	}
	if(choice == 0)
		printf("llpTestTask\n");
	else if(choice == 1)
		printf("collatzTask\n");
	else if(choice == 2)
		printf("binarySearchTestTask\n");
	else if(choice == 3)
		printf("linearSearchTestTask\n");



	for(int i = 0; i < 3; i++)
		entrypoint1();
	for(int i = 0; i < 3; i++)
		entrypoint2();
	
	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("i");
		++i;
	}
}

void init3()
{
	int collatz_number = 0xfffff - 1;
	int long_running_program_number = 10000;

	pid_t childCltz02 = fork();
	if(childCltz02 == 0)
	{
		long_running_program(long_running_program_number);
		// taskA();
		exit(EXIT_SUCCESS);
	}
	waitpid(childCltz02, NULL, WNOHANG);
	setpriority(childCltz02, HIGHEST);

	pid_t childLrp03 = fork();
	if(childLrp03 == 0)
	{
		taskA();
		exit(EXIT_SUCCESS);
	}
	waitpid(childLrp03, NULL, WNOHANG);

	pid_t childLrp04 = fork();
	if(childLrp04 == 0)
	{
		long_running_program(long_running_program_number);
		exit(EXIT_SUCCESS);
	}
	waitpid(childLrp04, NULL, WNOHANG);

	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("i");
		++i;
	}
}


void init()
{
	int collatz_number = 7;
	int long_running_program_number = 10000;

	// pid_t childCltz02 = fork();
	// if(childCltz02 == 0)
	// {
	// 	execve(collatzTask);
	// }
	// waitpid(childCltz02, NULL, WNOHANG);

	pid_t childCltz03 = fork();
	if(childCltz03 == 0)
	{
		// setpriority(getpid(), HIGHER);
		// collatz(collatz_number);
		// taskA();
		taskB();
		exit(EXIT_SUCCESS);
	}
	waitpid(childCltz03, NULL, WNOHANG);

	// pid_t childCltz04 = fork();
	// if(childCltz04 == 0)
	// {
	// 	// printf("\n03: "); printfHex(childCltz03);
	// 	setpriority(childCltz03, HIGHEST);

	// 	// collatz(collatz_number);
	// 	taskA();
	// 	exit(EXIT_SUCCESS);
	// }
	// waitpid(childCltz04, NULL, WNOHANG);
	// setpriority(childCltz04, HIGH);

	// int a = getpid();
	// printf("\nparent 03: "); printfHex(childCltz03);


	pid_t childLrp05 = fork();
	if(childLrp05 == 0)
	{
		// printf("\n a: "); printfHex(a);
		// printf("\n02: "); printfHex(childCltz02);
		// printf("\n04: "); printfHex(childCltz04);
		// printf("\n03: "); printfHex(childCltz03);
		// printf(" -> LOW:  ");
		// printf((char *)(setpriority(childCltz03, LOW) ? "true" : "false"));
		// printf("\n");
		setpriority(getpid(), LOWEST);
		// long_running_program(long_running_program_number);
		taskB();
		exit(EXIT_SUCCESS);
	}
	waitpid(childLrp05, NULL, WNOHANG);
	// setpriority(childLrp05, HIGH);

	// printf("\nparent 03: "); printfHex(childCltz03);


	// pid_t childLrp06 = fork();
	// if(childLrp06 == 0)
	// {
	// 	// long_running_program(long_running_program_number);
	// 	taskB();
	// 	exit(EXIT_SUCCESS);
	// }
	// waitpid(childLrp06, NULL, WNOHANG);
	// setpriority(childLrp06, LOWER);

	// pid_t childLrp07 = fork();
	// if(childLrp07 == 0)
	// {
	// 	// long_running_program(long_running_program_number);
	// 	taskB();
	// 	exit(EXIT_SUCCESS);
	// }
	// waitpid(childLrp07, NULL, WNOHANG);


	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("i");
		++i;
	}
}




void taskA()
{
	// execve(taskB);

	pid_t child = fork();
	// todo sil bunlari
	// printf("child = ");
	// printfHex(child);
	// printf("\n");
	if(child == 0)
	{
		printf("child = ");
		printfHex(child);
		printf("\n");

		int counter = 0;
		uint32_t i = 0;
		while(true)
		{
			if(i % time == 0)
			{
				sysprintf("C");
				counter++;
				if(counter == 4)
					exit(EXIT_SUCCESS);
			}
			++i;
		}
	}

	waitpid(child, NULL, 0);

	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
		// {
		// 	sysprintfHex(i);
		// 	sysprintf("\n");
		// }
			sysprintf("A");
		++i;
	}
}

void taskB()
{
	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
		{
			sysprintf("B");
		}
			// sysprintf("\n");
		++i;
	}
}

void taskC()
{
	int i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("C");
			// sysprintf("\n");
		++i;
	}
}

void testChildOfChild()
{
	int collatz_number = 7;
	int long_running_program_number = 10000;

	pid_t childCltz1 = fork();
	if(childCltz1 == 0)
	{
		pid_t childOfCltz = fork();
		if(childOfCltz == 0)
		{
			collatz(collatz_number);
			exit(EXIT_SUCCESS);
		}
		int status = -1;
		waitpid(childOfCltz, &status, 0);
		printf("Child of Collatz exited with status: ");
		printf((char *)(status == EXIT_SUCCESS ? "EXIT_SUCCESS" : "EXIT_FAILURE"));
		printf("\n");
		collatz(collatz_number);
		exit(EXIT_SUCCESS);
	}
	waitpid(childCltz1, NULL, WNOHANG);
	uint32_t i = 0;
	while(true)
	{
		if(i % time == 0)
			sysprintf("T");
		++i;
	}
}

void binarySearchTestTask()
{
	int child = fork();
	if(child == 0)
	{
		int arr[500];
		for(int i = 0; i < 500; i++)
		{
			arr[i] = i * 2 + rand_r() % 2;
		}
		int n = 500;
		int x = 955;
		int result = binarySearch(arr, n, x);
		if(result == -1)
		{
			printf("Element is not present in array\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("Element "); printfHex32(x); printf("is present at index ");
			printfHex32(result);
			printf("\n");
		}
		exit(EXIT_SUCCESS);
	}
	waitpid(child, NULL, WNOHANG);
}

void linearSearchTestTask()
{
	int child = fork();
	if(child == 0)
	{
		int arr[500];
		for(int i = 0; i < 500; i++)
		{
			arr[i] = (i * 2) + (rand_r() % 2);
		}
		int n = 500;
		int x = 596;
		int result = linearSearch(arr, n, x);
		if(result == -1)
		{
			printf("Element is not present in array\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("Element "); printfHex32(x); printf("is present at index ");
			printfHex32(result);
			printf("\n");
		}
		exit(EXIT_SUCCESS);
	}
	waitpid(child, NULL, WNOHANG);
}

void collatzTask()
{
	int child = fork();
	if(child == 0)
	{
		int collatz_number = 35;
		collatz(collatz_number);
		exit(EXIT_SUCCESS);
	}
	waitpid(child, NULL, WNOHANG);
}

void llpTestTask()
{
	int child = fork();
	if(child == 0)
	{
		int long_running_program_number = 5000;
		long_running_program(long_running_program_number);
		exit(EXIT_SUCCESS);
	}
	waitpid(child, NULL, WNOHANG);
}

// Collatz sequence
void collatz(int n)
{
	printfHex32(n);
    printf(": ");
    while(n != 1)
    {
        if(n % 2 == 0)
        {
            n = n / 2;
        }
        else
        {
            n = 3 * n + 1;
        }
        printfHex32(n);
		printf(" ");
    }
    printf("\n");
}

// Long running program
int long_running_program(int n)
{
    int result = 0;
    for(int i = 0; i < n; ++i)
    {
        for(int j = 0; j < n; ++j)
        {
            result += i * j;
        }
    }
    return result;
}

int binarySearch(int arr[], int n, int x)
{
    int l = 0, r = n - 1;
    while (l <= r) {
        int m = l + (r - l) / 2;
        if (arr[m] == x)
            return m;
        if (arr[m] < x)
            l = m + 1;
        else
            r = m - 1;
    }
    return -1;
}

int linearSearch(int arr[], int n, int x)
{
    for (int i = 0; i < n; i++)
        if (arr[i] == x)
            return i;
    return -1;
}

