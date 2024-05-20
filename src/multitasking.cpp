
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

/* Task Class Implementations*/

common::uint32_t pid_counter = 1;

void printf(char* str);
void printNumber(int num);
void sleep(uint32_t milliseconds);

Task::Task(GlobalDescriptorTable *gdt, void ptr())
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
    cpustate -> eip = (uint32_t)ptr;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    
    wait_pid = 0;
    parent_pid = 0;
    pid = pid_counter++;
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
common::uint32_t TaskManager::AddTask(void ptr())
{
    tasks[numTasks].task_state=READY;
    
    tasks[numTasks].pid = pid_counter++;


    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = 0;
    tasks[numTasks].cpustate -> ebx = 0;
    tasks[numTasks].cpustate -> ecx = 0;
    tasks[numTasks].cpustate -> edx = 0;

    tasks[numTasks].cpustate -> esi = 0;
    tasks[numTasks].cpustate -> edi = 0;
    tasks[numTasks].cpustate -> ebp = 0;
    
    tasks[numTasks].cpustate -> eip = (uint32_t)ptr;
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
    // printf("New Task is added with Pid: ");
    // printNumber((int)tasks[numTasks].pid);
    // printf("\n");
    
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
        //return -1;
    tasks[currentTask].task_state = FINISHED;
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
    
    ptr();

    return 1;
}
/* Make the flag of the task finished, so its never going to be scheduled*/

bool TaskManager::exit()
{
    tasks[currentTask].task_state = FINISHED; /* Never schedule it again!*/
    return true;
}


void saveCPUState(CPUState* to, const CPUState* from)
{
    if(!to)
    {
        printf("Copying operation is not happened, CPU is not initialized \n");
        return;
    }
    if(!from)
    {
        printf("Copying operation is not happened, source CPU state is empty \n");
        return;
    }
    to->eax = from->eax;
    to->ebx = from->ebx;
    to->ecx = from->ecx;
    to->edx = from->edx;
    to->esi = from->esi;
    to->edi = from->edi;
    to->ebp = from->ebp;
    to->error = from->error;
    to->eip = from->eip;
    to->cs = from->cs;
    to->eflags = from->eflags;
    to->esp = from->esp;
    to->ss = from->ss;
}

common::uint32_t TaskManager::fork(CPUState* cpustate, Saved* saved_tasks, int arraySize)
{

    if (numTasks >= 256)
        return 0;
    tasks[numTasks].task_state = READY;
    tasks[numTasks].parent_pid = tasks[currentTask].pid;
    tasks[numTasks].pid = pid_counter++;
    // tasks[numTasks].priority = DEFAULT_PRIORTIY;

    // printf("Pid Counter ");
    // printNumber((int)tasks[numTasks].pid);
    
    saved_tasks[currentTask].pid = tasks[currentTask].pid;
    saved_tasks[currentTask].ppid = tasks[currentTask].pid; //parent pid of parent is zero
    // saved_tasks[currentTask].priority = DEFAULT_PRIORTIY;

    saved_tasks[numTasks].pid = tasks[numTasks].pid;
    saved_tasks[numTasks].ppid = tasks[numTasks].parent_pid;
    // saved_tasks[numTasks].priority =   tasks[numTasks].priority;

    
    //printf("Parent pid ");
    //printNumber(saved_tasks[numTasks].ppid);    
    //printf("\n");

    for(int i = 0 ; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }
    
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));

    saveCPUState(tasks[numTasks].cpustate, cpustate);
    
    uint32_t stackOffset = (uint32_t)cpustate->esp - (uint32_t)tasks[currentTask].stack; /* esp difference*/

    tasks[numTasks].cpustate->esp = (uint32_t) tasks[numTasks].stack + stackOffset;
    tasks[numTasks].cpustate->ecx = 0; 
    
    numTasks++;
    // return tasks[numTasks-1].task_state = BLOCKED; 
    return tasks[numTasks-1].pid; /*Return current id's process id*/
}

/*
    Sets the actual priority of the task
    
    tasks[currentTask].priority : Priorty is a constant which will not change and show process's priority
    tasks[currentTask].scheduling_priority : A priority value which will be updated in each context switch

*/
common::uint32_t TaskManager::forkPriority(CPUState* cpustate)
{    
    tasks[currentTask].priority = cpustate->ebx;
    tasks[currentTask].scheduling_priority = cpustate->ebx;
 
}


int TaskManager::getCurrentTaskNumber() const
{
    
    return currentTask;

}
void TaskManager::setTask(Saved *task)
{
    tasks[currentTask].pid = task->pid;
    tasks[currentTask].parent_pid = task->ppid;
}


/*
    Returns the pid of the current task.
*/
common::uint32_t TaskManager::getTaskPid()
{
    return tasks[currentTask].pid;
}

int TaskManager::getProcess(common::uint32_t pid)
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

    int check_exist = getProcess(pid);

    if(tasks[check_exist].task_state == FINISHED)
    {
        return false;
    }
    if(check_exist == -1 && numTasks <= check_exist) /* Check if any task exist whose pid equals to parameter*/
    {   
        
        return false;
    }
    if(tasks[currentTask].pid == pid || pid == 0) //Change this, self waiting check
    {  


        return false;
    }
    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].wait_pid = pid;
    tasks[currentTask].task_state = BLOCKED;
       
    return true;
}

common::uint32_t TaskManager::getParentPid()
{
    return tasks[currentTask].parent_pid;
}

common::uint32_t TaskManager::getPriority()
{
    return tasks[currentTask].priority;
}


void TaskManager::printProcessTable(Saved* savedTasks, int size)
{
    printf("---------------------------------------------\n");
    printf("ProcessId    ProcessParentId   ProcessState");
    printf("\n");
    
    for(int i = 0 ; i < numTasks; i++)
    {
        printf(" ");
        printNumber(savedTasks[i].pid);
        printf("                   ");
        printNumber(savedTasks[i].ppid);
        printf("             ");
        if(tasks[i].task_state == READY && i == getCurrentTaskNumber())
        {
            printf("RUNNING");
        }
        else if(tasks[i].task_state == READY && i != getCurrentTaskNumber())
        {
            printf("READY");
        }
        if(tasks[i].task_state == FINISHED)
        {
            printf("FINISHED");
        }
        if(tasks[i].task_state == BLOCKED)
        {
            printf("BLOCKED");
        }
        printf("\n");
    }
    printf("---------------------------------------------\n");
    sleep(10000);

}
/***
 *  Checks if the all the priorities are the same or not
 * 
 *  Returns: True if all the priorities are the same
 * 
 * */
bool TaskManager::isRoundRobinBasedPriority( ) const
{
    // if(numTasks == 1)
    //     return true;

    int firstElement = tasks[0].scheduling_priority;
    for (int i = 1; i < numTasks; ++i) 
    {
        if (tasks[i].scheduling_priority != firstElement) 
        {

            return false;
        }
    }
    return true;
}
void TaskManager::updateAllPriorities(int index)
{
 
    for(int i = 0; i < numTasks; i++)
    {
        if(i != index)
        {
            if(tasks[i].scheduling_priority == 1 && tasks[i].scheduling_priority == 0)
            {
                tasks[i].scheduling_priority = tasks[i].priority; /* Store its actual priority */
            }
            else
            {
                tasks[i].scheduling_priority = tasks[i].scheduling_priority - 1;
            }
        }
        else
        {
            if(tasks[i].scheduling_priority == 1 && tasks[i].scheduling_priority == 0)
            {
                tasks[i].scheduling_priority = tasks[i].priority; /* Store its actual priority */
            } 
        }
    }
}
CPUState* TaskManager::Schedule(CPUState* cpustate)
{
 
    if(currentTask > 0 || currentTask == 0) /* Current Task must be greater than 0*/
        tasks[currentTask].cpustate = cpustate;
    
    if(numTasks < 0 || numTasks == 0) /* numTask can not be negative */
        return cpustate;
    
    int blockedProcessIndex = 0;
    int counter = 0;
    int ready_states = 0;
    int index = (currentTask + 1) % numTasks;
    bool isPriorityBased = isRoundRobinBasedPriority();

    if(tasks[index].task_state == READY)
    {
        currentTask = index;
        return tasks[currentTask].cpustate; /* Return the current task's cpustate to the assembly*/
    }
    else
    {
        while (tasks[index].task_state!=READY)
        {
            if(tasks[index].task_state==BLOCKED && tasks[index].wait_pid>0){

                blockedProcessIndex = 0;
                
                blockedProcessIndex = getProcess(tasks[index].wait_pid);
                
                if(blockedProcessIndex > -1 && tasks[blockedProcessIndex].task_state!=BLOCKED){
                    if (tasks[blockedProcessIndex].task_state==FINISHED)
                    {
                        tasks[index].wait_pid=0;
                        tasks[index].task_state=READY;
                    }
                    else if (tasks[blockedProcessIndex].task_state==READY)
                    {
                        index = blockedProcessIndex;
                    }
                    
                }
            }
            index= (++index) % numTasks;
        }
        currentTask = index;
        return tasks[currentTask].cpustate; /* Return the current task's cpustate to the assembly*/
    }
}
// CPUState* TaskManager::Schedule(CPUState* cpustate)
// {
//     if(numTasks <= 0)
//         return cpustate;
    
//     if(currentTask >= 0)
//     {
//         tasks[currentTask].cpustate = cpustate;
//     }    
//     int minPriority;
//     int minPriorityIndex;
//     bool isPriorityBased = false;
//     int index = (currentTask + 1) % numTasks; /* Iterate in the tasks*/

//     if(isPriorityBased == false && numTasks > 3) /* Find the task with highest priority*/
//     {
//         minPriority = 100;
//         minPriorityIndex = -1;
        
//         for(int i = 0; i < numTasks; i++)
//         {
//             if(tasks[i].scheduling_priority < minPriority)
//             {
//                 minPriority = tasks[i].scheduling_priority;
//                 minPriorityIndex = i;
//             }
//         }
//         // index = minPriorityIndex;
//         // printf("MaxPriorityIndex and Priority\n");
//         if(minPriorityIndex != -1 && minPriority != 0)
//             index = minPriorityIndex;
        
//         // printf("Choosen Index ");
//         // printNumber(index);
//         // printNumber(minPriority);
//         // printf(" ");
//         //printNumber(numTasks);
//         // updateAllPriorities(index); /* Decrease all the priorities */
//         // index = minPriorityIndex;
//     }
//     // // printf("\n");
//     while (tasks[index].task_state!=READY) //if its not ready it can be blocked ???
//     {
        
//         if(tasks[index].task_state==BLOCKED && tasks[index].wait_pid > 0) /*If the current task is parent*/
//         {

//             int blockedTask = 0;
//             blockedTask = getProcess(tasks[index].wait_pid); /* We got the parent task which is blocked */
            
//             /* Check the task which blocks this tasks */
//             if( blockedTask > -1 && tasks[blockedTask].task_state!=BLOCKED)
//             {
//                 if (tasks[blockedTask].task_state == FINISHED)
//                 {
//                     tasks[index].wait_pid=0;
//                     tasks[index].task_state=READY;
//                     continue;
//                 }
                
//                 else if (tasks[blockedTask].task_state == READY)
//                 {
//                     index = blockedTask;
//                     continue;
//                 }
//             }
//         }
//         index= (++index) % numTasks;
//     }
//     updateAllPriorities(index);
//     currentTask=index;
//     return tasks[currentTask].cpustate;
// }



/* TaskRecovery */

TaskRecovery::TaskRecovery()
{
    pid = 0;
    parent_pid = 0;
    wait_pid = 0;
}
TaskRecovery::TaskRecovery(common::uint32_t pid_)
:pid(pid_)
{

}
TaskRecovery::TaskRecovery(common::uint32_t pid_, common::uint32_t parent_pid_)
:pid(pid_), parent_pid(parent_pid_)
{

}
void TaskRecovery::setTaskState(State task_state_)
{
    task_state = task_state_;
}

common::uint32_t TaskRecovery::getTaskPid()
{
    return pid;
}
common::uint32_t TaskRecovery::getParentPid()
{
    return parent_pid;
}
common::uint32_t TaskRecovery::getWaitPid()
{
    return wait_pid;
}
State TaskRecovery::getTaskState()
{
    return task_state;
}
void TaskRecovery::setTaskPid(common::uint32_t pid_)
{
    pid = pid_;
}