/*****************************************************


FATE_OS_C v1.1
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

#include <msp.h>
#include "fate.h"

/*
Idle thread
Executes whenever no other thread is scheduled to run, doing nothing
Later, can be optimized using WFI (wait for interrupt) instruction
*/
void idle_thread(void)
{
	while(1);
}

/*
List that holds the information structure for each task (task_ctrl_blk).
Size of this list limits the number of tasks FATE-OS supports.
*/
task_ctrl_blk Task_list[8];

/*
List that matches events to a corresponding task
(just 2 events for now)
*/
task_ctrl_blk *Event_task_list[2];

/*
Pointer to element in "Task_list" that is currently executing
(Idle task by default).
Used in Timer ISR to determine if we are running highest priority task or not
*/
task_ctrl_blk *current_task = &(Task_list[0]);


/*
Must always be called in "main" prior to adding other tasks.

Initialized element 0 of "Task_list" as the Idle task and sets all 
others as empty (state == Undefined) so they can eventually be setup
through calls to "Task_add"

*/
void Task_list_init(void)
{
	int i;
	
	Task_list[0].state = TASK_RUNNING;
	Task_list[0].function = (uint32_t)idle_thread;
	Task_list[0].period = 1;
	Task_list[0].count = 0;
	Task_list[0].priority = -1;
	
	for(i=1;i<8;i++)
	{
		Task_list[i].state = TASK_UNDEFINED;
		Task_list[i].function = (uint32_t)idle_thread;
		Task_list[i].period = 1;
		Task_list[i].count = 0;
		Task_list[i].priority = -1;
	}
	
	//Clear all aperiodic events
	for(i=0;i<2;i++)
	{
		Event_task_list[i] = (task_ctrl_blk *)0;
	}
}

/*
Returns a pointer to the "Task_list" entry of the highest priority active task
(a Task is active if it is in Running or Suspended states)
*/
__inline task_ctrl_blk *get_priority_task(void)
{
	int i;
	int highest = 0;
	int8_t highest_priority = -1;
	
	for(i=0; i<8; i++)
	{
		if((Task_list[i].state == TASK_RUNNING) || (Task_list[i].state == TASK_SUSPENDED))
		{
			if(Task_list[i].priority > highest_priority)
			{
				highest_priority = Task_list[i].priority;
				highest = i;
			}
		}
	}
	return &(Task_list[highest]);
}

/*
Called by application code (main) to setup periodic tasks
Requires pointer to the function that implements the task, as 
well as its period (in system ticks = 10ms) and priority (1 to 255, 1 is lowest)
*/
void Task_add(uint32_t function, uint32_t period, uint32_t priority)
{
	int i;
	for(i=1;i<8;i++)
	{
		//Find first unused task slot
		if(Task_list[i].state == TASK_UNDEFINED)
			break;
	}
	Task_list[i].state = TASK_STOPPED;
	Task_list[i].function = function;
	Task_list[i].period = period;
	Task_list[i].count = 0;
	Task_list[i].priority = priority;
}


/*
Called by application code (main) to setup aperiodic tasks
Requires pointer to the function that implements the task, as 
well as its triggering event and priority (1 to 255, 1 is lowest)

Supported events are in "fate.h", defined in "enum events"
*/
void Task_event_add(uint32_t function, enum events event, uint32_t priority)
{
	int i;
	for(i=1;i<8;i++)
	{
		//Find first unused task slot
		if(Task_list[i].state == TASK_UNDEFINED)
			break;
	}
	Task_list[i].state = TASK_STOPPED;
	Task_list[i].function = function;
	//For aperiodic tasks: period set as 0
	Task_list[i].period = 0;
	//For aperiodic tasks, set count as 1 (so periodic schedule never schedules it)
	Task_list[i].count = 1;
	Task_list[i].priority = priority;
	
	//Configure Device and Interrupt for corresponding event
	Enable_event(event);
	
	//Set pointer to newly configured task in event-task list
	Event_task_list[event] = &(Task_list[i]);
}


/*
Configures Device and Interrupt for corresponding event
Called by "Task_event_add" to setup event for aperiodic tasks	
*/
void Enable_event(enum events event)
{
	switch(event)
	{
		case SWITCH_P1_1:
		{
			//Configure Pin as GPIO
			P1SEL0 &= (uint8_t)(~BIT1);
			P1SEL1 &= (uint8_t)(~BIT1);
			//Configure Pin as input
			P1DIR &= (uint8_t)(~BIT1);
			//Enable internal resistors
			P1REN |= (uint8_t)BIT1;
			//Configure pull-up resistors
			P1OUT |= (uint8_t)BIT1;
			//Enable pin interrupt
			P1IE |= (uint8_t)BIT1;
			//Configure negative edge (active low switches) 
			P1IES |= (uint8_t)BIT1;
			
			//Enable Port interrupt in NVIC
			//Equal priority as timer interrupt
			//We don't want anything interrupting our scheduler
			//since we are manipulating stack, bad things could happen
			//also, scheduler has to be precise, or we drift out of time
			NVIC_EnableIRQ(PORT1_IRQn);
			NVIC_SetPriority(PORT1_IRQn, 2);
			
			break;
		}
		case SWITCH_P1_4:
		{
			//Configure Pin as GPIO
			P1SEL0 &= (uint8_t)(~BIT4);
			P1SEL1 &= (uint8_t)(~BIT4);
			//Configure Pin as input
			P1DIR &= (uint8_t)(~BIT4);
			//Enable internal resistors
			P1REN |= (uint8_t)BIT4;
			//Configure pull-up resistors
			P1OUT |= (uint8_t)BIT4;
			//Enable pin interrupt
			P1IE |= (uint8_t)BIT4;
			//Configure negative edge (active low switches) 
			P1IES |= (uint8_t)BIT4;
			
			//Enable Port interrupt in NVIC
			//Equal priority as timer interrupt
			//We don't want anything interrupting our scheduler
			//since we are manipulating stack, bad things could happen
			//also, scheduler has to be precise, or we drift out of time
			NVIC_EnableIRQ(PORT1_IRQn);
			NVIC_SetPriority(PORT1_IRQn, 2);
			
			break;
		}
		default: break;//do nothing, wrong event
	}
}


/*
Encapsulates inline assembly to get current value of Stack Pointer
Used in Timer ISR (system tick) to access the stack and manipulate the return address,
so we return to the task we want
*/
__inline uint32_t get_current_SP()
{
    uint32_t spReg; 
    __asm
    {
        MOV spReg, __current_sp()
    }
	return spReg;
}

/*
Main scheduler implementation

Occurs every 10ms

Updates "count" in every task so we keep track of time
Based on highest priority currently active task, manipulates stack to change return address
(so we return from ISR to the task we want to run) and updates information in 
pointer to current task and Task_list.

*/
void TA0_N_IRQHandler()
{
	uint32_t sp_p;
	int i;
	task_ctrl_blk *new_task;
	
	
	//Value of current stack pointer
	sp_p = get_current_SP();
	//Find Exception code: basically makes sp_p point to base of current stack frame - 4
	while((*((uint32_t *)sp_p)) != 0xFFFFFFE9)
		sp_p += (uint32_t)4;
	//Now that we are pointing to base of current stack frame, increment by 0x1C
	//so we are pointing at Return Address
	sp_p += 0x1C;
	
	//Increment "count" on all tasks, modulo task period
	//Set task as SUSPENDED (active) if it was STOPPED 
	//and count is back to 0 (matched period)
	for(i=1;i<8;i++)
	{
		//Don't upgrade count if task is aperiodic (period == 0)
		if(Task_list[i].period)
		{
			Task_list[i].count++;
			Task_list[i].count %= Task_list[i].period;
		}
		if(Task_list[i].count == 0)
		{
			if(Task_list[i].state == TASK_STOPPED)
				Task_list[i].state = TASK_SUSPENDED;
		}
	}
	//Get pointer to highest priority active (running or suspended) task
	new_task = get_priority_task();
	
	//Is the current highest priority active task not the currently running task?
	if(new_task != current_task)
	{
		//Yes, it is
		//Set current task to "suspended", if it is "running"; it might have stopped itself
		if(current_task->state == TASK_RUNNING)
			current_task->state = TASK_SUSPENDED;
		//Set new task to "running"
		new_task->state = TASK_RUNNING;
		//Update current task pointer
		current_task = new_task;
		//Return to new task by changing return address
		*((uint32_t *)sp_p) = current_task->function;
	}
	else
	{
		//No, we're still running highest priority active task
		//Is the current task finished?
		if(current_task->state == TASK_STOPPED)
		{
			//Yes: go back to idle task
			current_task = &(Task_list[0]);
			current_task->state = TASK_RUNNING;
			*((uint32_t *)sp_p) = current_task->function;
		}
		//No, current task is not finished
		//Return to same task (do nothing)
	}
		
	//clear Timer interrupt flag
	TA0CTL &= (uint16_t)(~(BIT0));
}

/*
Port 1 Interrupt handler
Processes events for aperiodic tasks 
*/
void PORT1_IRQHandler(void)
{
	if(P1IFG & BIT1)
	{
		P1IFG &= (uint8_t)(~BIT1);
		//If corresponding event-task is initialized
		if(Event_task_list[SWITCH_P1_1])
		{
			//Activate task (schedule will eventually run it)
			Event_task_list[SWITCH_P1_1]->state = TASK_SUSPENDED;
		}
	}
	if(P1IFG & BIT4)
	{
		P1IFG &= (uint8_t)(~BIT4);
		//If corresponding event-task is initialized
		if(Event_task_list[SWITCH_P1_4])
		{
			//Activate task (schedule will eventually run it)
			Event_task_list[SWITCH_P1_4]->state = TASK_SUSPENDED;
		}
	}
}

/*
Configures Timer for system tick, NVIC and CPU interrupts,
and starts the idle task
*/
void Task_schedule(void)
{
	//configure timer
	TA0CTL |= (uint16_t)(BIT8); //ACLK
	TA0CCR0 = (uint16_t)328; //10ms
	TA0CTL |= (uint16_t)BIT1; //interrupt enable
	TA0CTL |= (uint16_t)BIT4; //UP MODE
	
	//enable NVIC timer interrupts
	NVIC_EnableIRQ(TA0_N_IRQn);
	NVIC_SetPriority(TA0_N_IRQn, 2);
	
	//enable CPU interrupts
	__ASM("CPSIE I");
	
	idle_thread();
}

