 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>
#include<savedTasks.h>

namespace myos
{
    typedef enum State {READY, BLOCKED, FINISHED}; /* Shows the state of the current task */
    
    namespace hardwarecommunication{
        class InterruptHandler;
    }


    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx; /* The return value  "c" */
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip; /* Instruction Pointer */
        common::uint32_t cs;  /* Code Segment */
        common::uint32_t eflags; 
        common::uint32_t esp; /* Stack Pointer, It shows where the current address of stack for the current task, stack'in en son nerde kaldığının numarası*/
        common::uint32_t ss;        
    } __attribute__((packed));
    

            
    static common::uint32_t pid_counter = 1;

    class Task
    {
    friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB, stack stored for each task
        CPUState* cpustate; /*To store the register informations about the current cpu, especially esp.*/
        
        common::uint32_t pid = 0; /*pid of the current process*/
        common::uint32_t parent_pid = 0; /*pid of the parent process*/
        common::uint32_t wait_pid = 0; 

        State task_state; /*Stores the state of the current task*/
        // static common::uint32_t pid_counter;

    public:
        Task(GlobalDescriptorTable *gdt, void ptr());
        Task();
        ~Task();
        
        common::uint32_t getTaskId();
    };
    

    class TaskManager
    {
        friend class hardwarecommunication::InterruptHandler; /* To trigger Scheduling function we should have the HandleInterrupt function from InterruptHandler class*/
        private:
            Task tasks[256];
            // TaskRecovery savedTasks[256];
            int numTasks; /*Task counter*/
            int currentTask; /*Task id of the current task*/
            GlobalDescriptorTable *gdt = nullptr;
            

        public:
            TaskManager();
            ~TaskManager();
            TaskManager(GlobalDescriptorTable *gdt);

            bool AddTask(Task* task); /* To add a new task*/
            CPUState* Schedule(CPUState* cpustate); /* To schedule a new task when one of them has a long run*/

            bool exit(); /* Terminates the task immediately*/
            bool wait(common::uint32_t pid); /* wait pid basically*/
            void printTable();
            
            common::uint32_t AddTask(void ptr()); /* Add task by passing a function parameter*/
            common::uint32_t getTaskPid();  /* Returns the pid of the current task*/
            common::uint32_t getParentPid(); /*Returns the pid of the parennt */
            common::uint32_t fork(CPUState* cpustate, Saved* array, int arraySize);           
            common::uint32_t exec(void ptr());
            
            //void exec(void ptr());
            void setTask(Saved *task);
            
            int getCurrentTaskNumber() const;
            int getIndex(common::uint32_t pid);
    };

    class TaskRecovery
    {
        private:
            common::uint32_t pid = 0; /*pid of the current process*/
            common::uint32_t parent_pid = 0; /*pid of the parent process*/
            common::uint32_t wait_pid = 0; 
            State task_state; /*Stores the state of the current task*/

        public:
            TaskRecovery();
            TaskRecovery(common::uint32_t pid_);
            TaskRecovery(common::uint32_t pid_, common::uint32_t parent_pid_);
            void setTaskState(State task_state_);
            

            common::uint32_t getTaskPid();
            common::uint32_t getParentPid();
            common::uint32_t getWaitPid();
            State getTaskState();
            void setTaskPid(common::uint32_t pid_);
    };
    
}


#endif