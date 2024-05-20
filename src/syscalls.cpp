
#include <syscalls.h>
#include <savedtasks.h>
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 


typedef enum SystemCalls{FORK, EXEC, WAITPID, ADDTASK, GETPID, EXIT, PRINTF, PPID, PTABLE, SETPR, GETPRIO};

uint32_t saved_pids[256];
uint32_t saved_ppids[256];

SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}

int savedTasksCounter = 0;

Saved saved_tasks[256];



void printf(char*);
void printNumber(int number);

void myos::waitpid(common::uint8_t waitPid)
{
    asm("int $0x80" : : "a" (SystemCalls::WAITPID), "b" (waitPid));
}

int myos::getParentPid()
{
    int parent_pid = -1;        
        
    asm("int $0x80" : "=c" (parent_pid) : "a" (SystemCalls::PPID)); // 0x80 -> interrupt interrupt number

    return parent_pid;
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

void myos::printProcessTable()
{
    asm("int $0x80" :: "a" (SystemCalls::PTABLE));
}

void myos::fork(int *pid)
{
    asm("int $0x80" :"=c" (*pid): "a" (SystemCalls::FORK));
}

void myos::setPriority(int *pid, int* priority)
{
    asm("int $0x80" :"=c" (*pid) : "a" (SystemCalls::SETPR), "b" (*priority));
}

int myos::getPriority()
{
    int priority = 0; 
    
    asm("int $0x80" : "=c" (priority) : "a" (SystemCalls::GETPRIO)); 
    
    return priority;
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

void print()
{
    for(int i = 0 ; i < 8; i++)
    {
        printf("pid : ");
        printNumber(saved_tasks[i].pid);
        printf(" ppid: ");
        printNumber(saved_tasks[i].ppid);
        printf("\n");
    }
}




uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    
    /* Stack pointer içinde CPU state'i saklıyor*/
    /*Yukarıda a'ya SystemCall::GETPID'i koyduk yani integer 1 bu da eax'e karşılık geliyor  */
    uint32_t ecx_;
    uint32_t esp_;
    uint32_t ecx2_;
    uint32_t esp2_;
    switch(cpu->eax)
    {
        case SystemCalls::PRINTF:
            printf((char*)cpu->ebx);
            break;
        
        case SystemCalls::ADDTASK:
            cpu->ecx = InterruptHandler::syscall_addTask(cpu->ebx);
        
        case SystemCalls::GETPID:
            cpu->ecx = InterruptHandler::syscall_getpid(saved_tasks, 256);
            break;
        case SystemCalls::GETPRIO:
            // printf("Triggered");
            cpu->ecx = InterruptHandler::syscall_getPriority();
            break;
        case SystemCalls::EXIT:
            if(InterruptHandler::system_exit())
                return InterruptHandler::HandleInterrupt(esp);
            break;
        case SystemCalls::EXEC:

            esp_ = InterruptHandler::system_execute(cpu->ebx);
            
            return InterruptHandler::HandleInterrupt(esp);
 
            break;
        case SystemCalls::WAITPID:
            if(InterruptHandler::system_waitpid(esp))
                return InterruptHandler::HandleInterrupt(esp); /* Schedule another process*/
            break;
        case SystemCalls::FORK:

            ecx_ = InterruptHandler::system_fork(cpu,saved_tasks, 256);
            // saved_tasks[savedTasksCounter] = getPid() 
            esp_ = InterruptHandler::HandleInterrupt(esp);

            cpu->ecx = ecx_;
            return esp_;
            //return InterruptHandler::HandleInterrupt(esp);
                
            break;
        case SystemCalls::PPID:
            cpu->ecx = InterruptHandler::system_getPPID();
            break;

        case SystemCalls::PTABLE:
            
            InterruptHandler::system_printPTable(saved_tasks, 256);        
            return InterruptHandler::HandleInterrupt(esp);
            break;
        case SystemCalls::SETPR:
       
            InterruptHandler::system_setPriorty(cpu);        

            return InterruptHandler::HandleInterrupt(esp);
            
        default:
            break;
    }

    
    return esp;
}

