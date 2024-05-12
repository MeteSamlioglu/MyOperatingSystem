
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 


typedef enum SystemCalls{FORK, EXEC, WAITPID, ADDTASK, GETPID, EXIT, PRINTF};


SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


void printf(char*);

void myos::waitpid(common::uint8_t wPid)
{
    asm("int $0x80" : : "a" (SystemCalls::WAITPID),"b" (wPid));
}

void myos::sys_exit()
{
    asm("int $0x80" : : "a" (SystemCalls::EXIT));
}

void myos::sysprintf(char* str)
{
    asm("int $0x80" : : "a" (SystemCalls::PRINTF), "b" (str));
}

void myos::fork()
{
    asm("int $0x80" :: "a" (SystemCalls::FORK));
}

void myos::fork(int *pid)
{
    asm("int $0x80" :"=c" (*pid): "a" (SystemCalls::FORK));
}

int myos::exec(void entrypoint())
{
    int result;
    asm("int $0x80" : "=c" (result) : "a" (SystemCalls::EXEC), "b" ((uint32_t)entrypoint));
    return result;
}
//  b = ebx
int myos::addTask(void entrypoint())
{
    int result;
    asm("int $0x80" : "=c" (result) : "a" (SystemCalls::ADDTASK), "b" ((uint32_t)entrypoint));
    return result;
}

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    
    /* Stack pointer içinde CPU state'i saklıyor*/
    /*Yukarıda a'ya SystemCall::GETPID'i koyduk yani integer 1 bu da eax'e karşılık geliyor  */

    switch(cpu->eax)
    {
        case SystemCalls::PRINTF:
            printf((char*)cpu->ebx);
            break;
        //case 5: //Reserverd for fork    
        
        default:
            break;
    }

    
    return esp;
}

