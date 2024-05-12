 
#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace myos
{
    
    class SyscallHandler : public hardwarecommunication::InterruptHandler
    {
        
    public:
        SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber);
        ~SyscallHandler();
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);

    };

    int exec(void entrypoint());
    int getPid();
    int addTask(void entrypoint());
    void fork();
    void fork(int *pid);
    void sys_exit();
    void waitpid(common::uint8_t wPid);
    int addTask(void entrypoint());
    void sysprintf(char* str);
}


#endif