 
#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace myos
{
    enum SyscallType
    {
        EXIT = 1,
        FORK = 2,
        PRINTF = 4,
        PRINTFHEX = 5,
        WAITPID = 7,
        EXECVE = 11,
        GETPID = 20,
        GETPPID = 64,
        SET_PRIORITY = 97,
        SET_DYNAMIC_PRIORITY = 98
    };

    class SyscallHandler : public hardwarecommunication::InterruptHandler
    {
        
        public:
            SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber);
            ~SyscallHandler();
            
            virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);

    };

    common::pid_t fork();
    void exit(int exit_status);
    common::pid_t waitpid(common::pid_t __pid, int *__stat_loc, int __options);
    int execve(void entrypoint());
    common::pid_t getpid();
    common::pid_t getppid();
    int setpriority(common::pid_t __who, Priority __prio);
    int setdynamicpriority(common::pid_t __who, bool flag);

    
}


#endif