
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


void sleep(uint32_t milliseconds) 
{
    for (uint32_t i = 0; i < milliseconds; i++) {
        for (uint32_t j = 0; j < 400000; j++) {
            asm volatile("nop");
        }
    }
}
void sleep2(uint32_t milliseconds)
{
    for (uint32_t i = 0; i < milliseconds; i++) 
    {
        for (uint32_t j = 0; j < 10000000; j++) {
            asm volatile("nop");
        }
    }
}

char* itoa(int value, char* result, int base) {
    // Check that the base is valid
    if (base < 2 || base > 36) {
        *result = '\0';
        return result;
    }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';

    // Reverse the string
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}


void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printNumber(int number) {
    char str[12];
    itoa(number, str, 10);
    printf(str);
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

void long_runnig_program()
{
    int result = 0;
    int n = 10;
    for(int i = 0; i < n; i++)
    {
        for(int j = 0; j < n; j++)
        {
            result+=i*j;
        }
    }
    printf("Result: ");
    printNumber(result);
    printf("\n");
    
}
// int n = 7;
void printCollatzSequence() {

    int n = 7;
    printNumber(n);
    printf(" : ");
    while (n != 1) 
    {

        printNumber((int)n);
        printf(", ");
        if (n % 2 == 0) 
        {
            n = n / 2;
        } else 
        {
            n = 3 * n + 1;
        }
    }
    printf("1");
    printf("\n");
    n = n - 1;
}

// void sysprintf(char* str)
// {
//     asm("int $0x80" : : "a" (4), "b" (str));
// }

void taskA()
{
    while(true)
        sysprintf("A");
}

void taskB()
{
    while(true)
        sysprintf("B");
}
void LinearSearch()
{
    
    int target = 60;
    int output = -1;
    int array[10] = {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};

    for(int i = 0 ; i < 10; i++ )
    {
        if(array[i] == target)
        {
            output = i;
            break;
        }
    }
    printf("Output: ");
    printNumber(output);
    printf("\n");
}

void BinarySearch() {
    int array[10] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170}; // Sorted array
    int x = 110;
    int size = 10;

    int left = 0;
    int right = size - 1;
    int output = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (array[mid] == x) {
            output = mid;
            break;
        }
        if (array[mid] < x) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    printf( "Output: ");
    printNumber(output);
    printf("\n");
}

bool flag = false;

void lifeCycle1()
{
    int childPids[3] = {0, 0, 0};
 
    for(int i = 0 ; i < 3; i++)
    {
        int pid = childPids[i];
        printf("Forked\n");
        fork(&pid);

        if(pid > 0)
        {
            // printf("Parent task is waiting for the child.\n");
            // waitpid(childPids[i]);
            childPids[i] = pid; /* Save child pids */
            // printf(" Parent with id ");
            // printNumber(getPid());
            // printf("\n");
        }
        else
        {
            printf("Child Task ");
            printNumber(getPid());
            printf(" is created by Parent ");
            printNumber(getParentPid());
            printf("\n");

            while(flag == false)   /* Childs will wait until all child processes are created and parent signal them */
            { };
            printf("Child Task ");
            printNumber(getPid());
            printf(" is executing.");
            printf("\n");
            //printProcessTable();
            //sleep(1000);
            exec(long_runnig_program); /* Execute the task and terminate */
        }
    }
    printf("\nParent is waiting for all childs to terminate...\n");    
    printProcessTable();
    // sleep(10000);

    flag = true; /*Release all processes */
    for(int i = 0 ; i < 3 ; i++)
    {
        waitpid(childPids[i]);  /* Parent is waiting for all childs to terminate */
    }
    sleep(10000);
    printf("\n\n\n\n\n\n\n\n");
    printProcessTable();
    // sleep(1000);
    printf("Parent is terminated.");
    sys_exit();
}
void testSleepFunction()
{
    printf("Testing Sleep");
    sleep(1000);
    printf("Terminating");

}
// void singleForkExample()
// {
//     int pid = 0;
    
//     fork(&pid);
//     if(pid > 0)
//     {
//         waitpid(pid);
//         printf("Parent task is waiting for the child.\n");
//         printf("Parent ");
//         printNumber(getPid());
//         printf(" is executing\n");
//         printNumber(pid);
//     }
//     else
//     {
//         //sleep(10000);
//         printf("Child Task ");
//         printNumber(getPid());
//         printf(" is executing");
//         printf("\n");
//         exec(long_runnig_program);
//     }   
//     sys_exit();
// }

void multipleForks()
{
    int childPids[3] = {0, 0, 0};
    // int priorities[3] = {0, 0, 0};
    for(int i = 0 ; i < 3; i++)
    {
        int pid = childPids[i];
        // int priority = priorities[3];
        printf("Forked: ");
        fork(&pid);

        if(pid > 0)
        {
            printf("Parent task is waiting for the child.\n");
            waitpid(childPids[i]);
            printf("Parent Task ");
            printNumber(getPid());
            printf(" is executing.\n");
            // sleep(1000);

        }
        else
        {
            printf("Child Task ");
            printNumber(getPid());
            printf(" is executing.");
            printf("\n");
            // printProcessTable();
            sys_exit();
        }
    }
    printf("Finished");
    sys_exit();
}

int priorities[3] = {1, 2, 3};
int counter = 0;
bool flag2 = false;

void ThirdStrategy()
{
    int childPids[3] = {0, 0, 0};
    // int priorities[3] = {1, 0, 0};
    for(int i = 0 ; i < 3; i++)
    {
        int pid = childPids[i];
        // int priority = priorities[3];
        printf("Forked\n");
        fork(&pid);
        if(pid > 0)
        {
            counter++;
            childPids[i] = pid;
            // printf("Parent task is waiting for the child.\n");
            // waitpid(childPids[i]);
            // printf("Parent Task ");
            // printNumber(getPid());
            // printf(" is executing.\n");
            // sleep(1000);
        }
        else
        {
            setPriority(&pid, &priorities[counter]);

            printf("\nChild Task ");
            printNumber(getPid());
            printf(" with prioirty ");
            printNumber((int)getPriority());
            printf(" is waiting for all tasks...\n");
            // counter++;
            while(flag2 == false)
            {};
            printf(" is executing.");
            printf("\n");
            //counter++;
            // printProcessTable();
            // sys_exit();
        }
    
    }
    
    printf("\nParent is finished...\n");
    printProcessTable();
    // printf("Finished");
    sys_exit();
}

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
    
    // printf("This is my operating System\n");


    GlobalDescriptorTable gdt;
    

    //TaskManager taskManager;
    // BinarySearch();
    TaskManager taskManager(&gdt);
    //Task task1(&gdt, lifeCycle1);
    Task task2(&gdt, ThirdStrategy);
    
    //taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    //sleep(10000);s
    //Task task2(&gdt, multipleForks2);
    //taskManager.AddTask(&task2);
    // printf("Programs are loaded\n");
    
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    
    SyscallHandler syscalls(&interrupts, 0x80); // 0x80 numaralı interruptları karşılayan handler
    
    //printf("Initializing Hardware, Stage 1\n");

    
    // #ifdef GRAPHICSMODE
    //     Desktop desktop(320,200, 0x00,0x00,0xA8);
    // #endif
    
    // DriverManager drvManager;
    
    //     #ifdef GRAPHICSMODE
    //         KeyboardDriver keyboard(&interrupts, &desktop);
    //     #else
    //         PrintfKeyboardEventHandler kbhandler;
    //         KeyboardDriver keyboard(&interrupts, &kbhandler);
    //     #endif
    //     drvManager.AddDriver(&keyboard);
        
    
    //     #ifdef GRAPHICSMODE
    //         MouseDriver mouse(&interrupts, &desktop);
    //     #else
    //         MouseToConsole mousehandler;
    //         MouseDriver mouse(&interrupts, &mousehandler);
    //     #endif
    //     drvManager.AddDriver(&mouse);
        
    //     PeripheralComponentInterconnectController PCIController;
    //     PCIController.SelectDrivers(&drvManager, &interrupts);

    //     #ifdef GRAPHICSMODE
    //         VideoGraphicsArray vga;
    //     #endif
        
    // //printf("Initializing Hardware, Stage 2\n");
    //     drvManager.ActivateAll();
        
    // //printf("Initializing Hardware, Stage 3\n");

    // #ifdef GRAPHICSMODE
    //     vga.SetMode(320,200,8);
    //     Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
    //     desktop.AddChild(&win1);
    //     Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
    //     desktop.AddChild(&win2);
    // #endif


    
    //printf("\nS-ATA primary master: ");
    // AdvancedTechnologyAttachment ata0m(true, 0x1F0);
    // ata0m.Identify();
    
    // //printf("\nS-ATA primary slave: ");
    // AdvancedTechnologyAttachment ata0s(false, 0x1F0);
    // ata0s.Identify();
    // ata0s.Write28(0, (uint8_t*)"http://www.AlgorithMan.de", 25);
    // ata0s.Flush();
    // ata0s.Read28(0, 25);
    
    // //printf("\nS-ATA secondary master: ");
    // AdvancedTechnologyAttachment ata1m(true, 0x170);
    // ata1m.Identify();
    
    // //printf("\nS-ATA secondary slave: ");
    // AdvancedTechnologyAttachment ata1s(false, 0x170);
    // ata1s.Identify();
    // third: 0x1E8
    // fourth: 0x168             

                   
    //amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);

    // // IP Address
    // uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
    // uint32_t ip_be = ((uint32_t)ip4 << 24)
    //             | ((uint32_t)ip3 << 16)
    //             | ((uint32_t)ip2 << 8)
    //             | (uint32_t)ip1;
    // eth0->SetIPAddress(ip_be);
    // EtherFrameProvider etherframe(eth0);
    // AddressResolutionProtocol arp(&etherframe);    

    
    // // IP Address of the default gateway
    // uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
    // uint32_t gip_be = ((uint32_t)gip4 << 24)
    //                | ((uint32_t)gip3 << 16)
    //                | ((uint32_t)gip2 << 8)
    //                | (uint32_t)gip1;
    
    // uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
    // uint32_t subnet_be = ((uint32_t)subnet4 << 24)
    //                | ((uint32_t)subnet3 << 16)
    //                | ((uint32_t)subnet2 << 8)
    //                | (uint32_t)subnet1;
                   
    // InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnet_be);
    // InternetControlMessageProtocol icmp(&ipv4);
    // UserDatagramProtocolProvider udp(&ipv4);
    // TransmissionControlProtocolProvider tcp(&ipv4);
    
    interrupts.Activate();
    //printf("\n\n\n\n");
    
    // arp.BroadcastMACAddress(gip_be);
    
    
    // PrintfTCPHandler tcphandler;
    // TransmissionControlProtocolSocket* tcpsocket = tcp.Listen(1234);
    // tcp.Bind(tcpsocket, &tcphandler);
    // tcpsocket->Send((uint8_t*)"Hello TCP!", 10);

    
    // icmp.RequestEchoReply(gip_be);
    
    // PrintfUDPHandler udphandler;
    // UserDatagramProtocolSocket* udpsocket = udp.Connect(gip_be, 1234);
    // udp.Bind(udpsocket, &udphandler);
    // udpsocket->Send((uint8_t*)"Hello UDP!", 10);
    
    // UserDatagramProtocolSocket* udpsocket = udp.Listen(1234);
    // udp.Bind(udpsocket, &udphandler);

    
    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
    }
}
