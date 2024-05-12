
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

/* Task Class Implementations*/

common::uint32_t Task::pid_counter = 1;

void printf(char* str);
void printNum(int num);


Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
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
    
}

Task::~Task()
{
    
}

/*
    Returns teh process id of current task
*/
common::uint32_t Task::getTaskId()
{
    return pid;
}

Task::Task()
{
    task_state = FINISHED;
    parent_pid = 0;
    wait_pid = 0;
    cpustate =  (CPUState*)(stack + 4096 - sizeof(CPUState)); 
    
    cpustate -> eax = 0; 
    cpustate -> ebx = 0;
    cpustate -> ecx = 0; /* Return değeri olarak kullanılıyo*/
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = 0; /* instruction pointer*/
    cpustate -> cs = 0;  /* code segment */
    cpustate -> eflags = 0x202;
}

/* ----------------------------------------------------------------------------------------- */
/* Task Manager class Implementation*/

TaskManager::TaskManager()
{
    /* When its initialized there is no task in it*/
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{

}

TaskManager::TaskManager(GlobalDescriptorTable *gdt_)
{
    numTasks = 0;
    currentTask = -1;
    gdt=gdt_;
}
common::uint32_t TaskManager::AddTask(void entrypoint())
{
    tasks[numTasks].task_state=READY;
    tasks[numTasks].pid=Task::pid_counter++;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = 0;
    tasks[numTasks].cpustate -> ebx = 0;
    tasks[numTasks].cpustate -> ecx = 0;
    tasks[numTasks].cpustate -> edx = 0;

    tasks[numTasks].cpustate -> esi = 0;
    tasks[numTasks].cpustate -> edi = 0;
    tasks[numTasks].cpustate -> ebp = 0;
    
    tasks[numTasks].cpustate -> eip = (uint32_t)entrypoint;
    tasks[numTasks].cpustate -> cs = gdt->CodeSegmentSelector();
    
    tasks[numTasks].cpustate -> eflags = 0x202;
    numTasks++;
    return tasks[numTasks-1].pid;
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256) /* Max Size for task array*/
        return false;
    
    tasks[numTasks].task_state = READY;
    tasks[numTasks].pid=task->pid;
    
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));

    tasks[numTasks].cpustate -> eax = task->cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate -> ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task->cpustate->edx;

    tasks[numTasks].cpustate -> esi = task->cpustate->esi;
    tasks[numTasks].cpustate -> edi = task->cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task->cpustate->ebp;
    
    tasks[numTasks].cpustate -> eip = task->cpustate->eip;
    tasks[numTasks].cpustate -> cs = task->cpustate->cs;
    tasks[numTasks].cpustate -> eflags = task->cpustate->eflags;

    numTasks++;    
    return true;
}
common::uint32_t TaskManager::exec(void ptr())
{
    if(currentTask == -1)
        return -1;
    
    tasks[currentTask].task_state=READY;
    tasks[currentTask].cpustate = (CPUState*)(tasks[currentTask].stack + 4096 - sizeof(CPUState));
    
    tasks[currentTask].cpustate -> eax = 0;
    tasks[currentTask].cpustate -> ebx = 0;
    tasks[currentTask].cpustate -> ecx = tasks[currentTask].pid;
    tasks[currentTask].cpustate -> edx = 0;
    tasks[currentTask].cpustate -> esi = 0;
    tasks[currentTask].cpustate -> edi = 0;
    tasks[currentTask].cpustate -> ebp = 0;
    tasks[currentTask].cpustate -> eip = (uint32_t)ptr;
    tasks[currentTask].cpustate -> cs = gdt->CodeSegmentSelector();
    tasks[currentTask].cpustate -> eflags = 0x202;
    
    return (uint32_t)tasks[currentTask].cpustate;

}
/* Make the flag of the task finished, so its never going to be scheduled*/

bool TaskManager::exit()
{
    tasks[currentTask].task_state = FINISHED; /* Never schedule it again!*/
    return true;
}

common::uint32_t TaskManager::fork(CPUState* cpustate)
{

    if (numTasks >= 256)
        return 0;

    tasks[numTasks].task_state = READY;
    tasks[numTasks].parent_pid = tasks[currentTask].pid;
    tasks[numTasks].pid = Task::pid_counter++;

    for(int i = 0 ; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }
    
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    common::uint32_t currentTaskOffset=((common::uint32_t)tasks[currentTask].cpustate) - ((common::uint32_t)cpustate);
    tasks[numTasks].cpustate=(CPUState*)(((common::uint32_t)tasks[numTasks].cpustate) - currentTaskOffset);
    tasks[numTasks].cpustate->ecx = 0; /*set child's pid to 0*/
    numTasks++;
    return tasks[numTasks-1].pid; /*Return current id's process id*/
}


/*
    Returns the pid of the current task.
*/
common::uint32_t TaskManager::getTaskPid()
{
    return tasks[currentTask].pid;
}

int TaskManager::getIndex(common::uint32_t pid)
{
  
    for(int i = 0; i < numTasks; i++)
    {
        if(tasks[i].pid == pid)
        {
            return i;
        }
    }
    
    return -1;
}

bool TaskManager::wait(common::uint32_t esp)
{
    CPUState *cpustate=(CPUState*)esp;
    common::uint32_t pid=cpustate->ebx;
    
    int check_exist = getIndex(pid);

    if(tasks[check_exist].task_state == FINISHED)
        return false;
    
    if(check_exist == -1 && numTasks <= check_exist) /* Check if any task exist whose pid equals to parameter*/
        return false;
    
    if(tasks[currentTask].pid==pid || pid==0) //Change this, self waiting check
        return false;

    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].wait_pid = pid;
    tasks[currentTask].task_state = BLOCKED;
    
    return true;
}

/* Modif*/
// void TaskManager::printTable(){
    
//     printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
//     printf("PID\tPPID\tSTATE\n");
//     for (int i = 0; i < numTasks; i++)
//     {
//         printNum(tasks[i].pid);
//         printf("\t  ");
//         printNum(tasks[i].parent_pid);
//         printf("\t   ");
//         if(tasks[i].task_state==State::READY){
//             if(i==currentTask)
//                 printf("RUNNING");
//             else
//                 printf("READY");
//         }else if(tasks[i].task_state==State::BLOCKED){
//             printf("BLOCKED");
//         }else if(tasks[i].task_state==State::FINISHED){
//             printf("FINISHED");
//         }
//         printf("\n");
//     }
//     printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    
//     /*Put sleep here*/
//     for (int i = 0; i < 10000000; i++){
//         printf("");
//     }
    
// }

// CPUState* TaskManager::Schedule(CPUState* cpustate)
// {
//     if(numTasks <= 0)
//         return cpustate;
    
//     if(currentTask >= 0)
//         tasks[currentTask].cpustate = cpustate;
    
//     if(++currentTask >= numTasks)
//         currentTask %= numTasks;
//     return tasks[currentTask].cpustate;
// }

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    int findTask=(currentTask+1)%numTasks;
    while (tasks[findTask].task_state!=READY)
    {
        if(tasks[findTask].task_state==BLOCKED && tasks[findTask].wait_pid>0){

            int waitTaskIndex=0;
            waitTaskIndex=getIndex(tasks[findTask].wait_pid);
            if(waitTaskIndex>-1 && tasks[waitTaskIndex].task_state!=BLOCKED){
                if (tasks[waitTaskIndex].task_state==FINISHED)
                {
                    tasks[findTask].wait_pid=0;
                    tasks[findTask].task_state=READY;
                    continue;
                }else if (tasks[waitTaskIndex].task_state==READY)
                {
                    findTask=waitTaskIndex;
                    continue;
                }
                
            }
        }
        findTask= (++findTask) % numTasks;
    }
    currentTask=findTask;
    return tasks[currentTask].cpustate;
}