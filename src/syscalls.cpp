
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 


typedef enum SystemCalls{FORK, EXEC, WAITPID, ADDTASK, GETPID, EXIT, PRINTF};


SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset())
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

/*
    When an interrupt happened with number 0x80, assign SystemCalls:EXIT value to eax register
*/
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

/* 
    
    When syscall occurs the process is being interrupted. To perform this interruption we must write assembly

*/
int myos::getPid()
{
    int task_pid = -1; 
    
    asm("int $0x80" : "=c" (task_pid) : "a" (SystemCalls::GETPID)); // 0x80 -> interrupt interrupt number
    
    return task_pid;
}

/*
    Returns the pid of the newly created task
    Assign eax = EXEC
    Assign ecx = pid
    Assign ebx = ptr
*/
int myos::exec(void ptr())
{
    int pid;
    
    asm("int $0x80" : "=c" (pid) : "a" (SystemCalls::EXEC), "b" ((uint32_t)ptr));
    
    return pid;
}


/*
ecx registerini result'a yaz, SystemCalls::ADDTASK değerini eax'e yaz, ebx'i entrypointe yaz. 
*/

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
    uint32_t ecx_;
    uint32_t esp_;
    switch(cpu->eax)
    {
        case SystemCalls::PRINTF:
            printf((char*)cpu->ebx);
            break;
        
        case SystemCalls::ADDTASK:
            cpu->ecx = InterruptHandler::syscall_addTask(cpu->ebx);
        
        case SystemCalls::GETPID:
            cpu->ecx = InterruptHandler::syscall_getpid();
            break;
        case SystemCalls::EXIT:
            if(InterruptHandler::system_exit())
                return InterruptHandler::HandleInterrupt(esp);
            break;
        case SystemCalls::EXEC:
            esp = InterruptHandler::system_execute(cpu->ebx);
            break;
        case SystemCalls::WAITPID:
            if(InterruptHandler::system_waitpid(esp))
                return InterruptHandler::HandleInterrupt(esp); /* Schedule another process*/
            break;
        case SystemCalls::FORK:
            //cpu->ecx = InterruptHandler::system_fork(cpu);
            ecx_ = InterruptHandler::system_fork(cpu);
            esp_ = InterruptHandler::HandleInterrupt(esp);
            cpu->ecx = ecx_;
            return esp_;
            //return InterruptHandler::HandleInterrupt(esp);
                
            break;
        default:
            break;
    }

    
    return esp;
}

