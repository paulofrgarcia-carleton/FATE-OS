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


#define NUM_TASKS 8
#define NUM_EVENTS 2


/** Definitions for different Task states. */
enum task_state {
    /** Undefined: corresponding task has not been initialized. */
    TASK_STOPPED,
    /** Stopped: corresponding task is not scheduled to run (no start event, i.e., period expiration, yet) */
    TASK_SUSPENDED,
    /** Suspended: task is ready to run, but is not yet the highest priority task */
    TASK_RUNNING,
    /** Running: currently executing task */
    TASK_UNDEFINED
};

/** List of events that can be used to start aperiodic tasks */
enum events {
    /** Switch p1.1 */
    SWITCH_P1_1,
    /** Switch p1.4 */
    SWITCH_P1_4
};

/** Structure that holds information for each task */
typedef struct 
{
    /** Address of function that implements thread */
	intptr_t function;
    /** Thread's periodicity in number of system ticks */
	uint32_t period;
    /** Number of system ticks that elapsed while task is stopped */
	uint32_t count;
    /** Number of system ticks to wait before scheduling task */
    uint32_t start_offset;
    /** The number of ticks from when the task starts to when it must complete */
    uint32_t deadline;
    /** The number of ticks remaining until the tasks deadline */
    uint32_t deadline_remaining;
    /** -1 not initialized, 0 stopped, 1 suspended, 2 running */
    enum task_state state:8;
}
task_ctrl_blk;


// Various function definitions

/**
 *  Initilize the task list to defualt values.
 *
 *  @note This function should be called before any other fate functions.
 */
void Task_list_init(void);

/**
 *  Add a new periodic task to the task list.
 *
 *  @param function The function which should be called for this task
 *  @param period Number of system ticks per task
 *  @param period Number of system ticks to wait before scheduling the task for the first time
 *  @param priority The priority of this task, high numbers are prioritized
 *
 *  @return 0 if the task was successfully added to the task list
 */
uint8_t Task_add(intptr_t function, uint32_t period, uint32_t start_offset,
                uint32_t deadline);

/**
 *  Add a new aperiodic task to the task list.
 *
 *  @param function The function which should be called for this task
 *  @param event The event which should trigger this task
 *  @param priority The priority of this task, high numbers are prioritized
 *
 *  @return 0 if the task was successfully added to the task list
 */
uint8_t Task_event_add(intptr_t function, enum events event, uint32_t deadline);

/**
 *  Start the task scheduler.
 *
 *  @note This function does not return.
 */
void Task_schedule(void);


/**
 *  List of tasks.
 */
extern task_ctrl_blk Task_list[NUM_TASKS];

/*
Macro called by each task when it finishes execution

Since we do not want Task functions to call other function (no proper stack handling yet)
this is implemented as a Macro rather than a function.

"Task_stop" finds the calling task by function address in the task list
and changes its state to Stopped. It then sets the pending bit for the timer A0
interrupt so that the scheduler will run.
*/
//Stops a task
#define Task_stop(X) { \
	int i; \
	for(i=1;i<NUM_TASKS;i++) \
	{ \
		if(Task_list[i].function == X) \
		{ \
			Task_list[i].state = TASK_STOPPED; \
            NVIC_SetPendingIRQ(TA0_N_IRQn); \
            while(1); \
		} \
	}\
}

#endif
