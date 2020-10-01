/*****************************************************


FATE_OS_H v1.1
The "Fake Time Environment Operating System"

Developed by 
Paulo Garcia
Dpt. of Systems and Computer Engineering
Carleton University
Ottawa, Ontario, Canada

This code if for educational purposes only (SYSC3310 - Introduction to Real Time Systems)
We do not guarantee this code will work on any given situation. 
Do not use this code in production software.


V1.0:

Basic support for up to 8 periodic tasks.
Tasks cannot call other functions nor should they have local variables.

V1.1:
Added support for aperiodic tasks (port interrupt events only)

******************************************************/

#ifndef FATE_OS_H
#define FATE_OS_H

#include <msp.h>
#include <stdint.h>


/*
Definitions for different Task states.

Undefined: corresponding task has not been initialized.
Stopped: corresponding task is not scheduled to run (no start event, i.e., period expiration, yet)
Suspended: task is ready to run, but is not yet the highest priority task
Running: currently executing task
*/
#define TASK_UNDEFINED -1
#define TASK_STOPPED 0
#define TASK_SUSPENDED 1
#define TASK_RUNNING 2



/*
List of events that can be used to start aperiodic tasks
For now, only switches (p1.1 and p1.4)
*/
enum events {SWITCH_P1_1 = 0, SWITCH_P1_4};

/*
Structure that hold information for each task

Task's current state, pointer to C function that implements the task, task period, task priority, 
and counter for system ticks

*/
typedef struct 
{
	int8_t state; //-1 not initialized, 0 stopped, 1 suspended, 2 running
	uint32_t function; //address of function that implements thread
	uint32_t period; //thread's periodicity in number of system ticks
	uint32_t count; //number of system ticks that elapsed while task is stopped
	int8_t priority; //task's priority 0 lowest, 255 highest
} 
task_ctrl_blk;

/*
Various function definitions: see "fate.c"
*/
void idle_thread(void);
void Task_list_init(void);
task_ctrl_blk *get_priority_task(void);
void Task_add(uint32_t function, uint32_t period, uint32_t priority);
void Task_event_add(uint32_t function, enum events event, uint32_t priority);
__inline uint32_t get_current_SP(void);
void Task_schedule(void);
void Enable_event(enum events event);

/*
Macro called by each task when it finishes execution

Since we do not want Task functions to call other function (no proper stack handling yet)
this is implemented as a Macro rather than a function.

"Task_stop" finds the calling task by function address in the task list
and changes its state to Stopped.
*/
//Stops a task
#define Task_stop(X) { \
	extern task_ctrl_blk Task_list[8]; \
	int i; \
	for(i=1;i<8;i++) \
	{ \
		if(Task_list[i].function == X) \
		{ \
			Task_list[i].state = TASK_STOPPED; \
		while(1); \
		} \
	}\
}

#endif
