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

// Prototypes
void idle_thread(void);
void Enable_event(enum events event);

void TA0_N_IRQHandler(void);
void PORT1_IRQHandler(void);


/**
 *  Idle thread
 *  Executes whenever no other thread is scheduled to run, sleep until the interrupt
 *  occures.
 */
void idle_thread(void)
{
    while(1) __WFI();
}

/**
 *  List that holds the information structure for each task (task_ctrl_blk).
 *  Size of this list limits the number of tasks FATE-OS supports.
 */
task_ctrl_blk Task_list[NUM_TASKS];

/**
 *  List that matches events to a corresponding task
 *  (just 2 events for now)
 */
task_ctrl_blk *Event_task_list[NUM_EVENTS];

/**
 *  Pointer to element in "Task_list" that is currently executing
 *  (Idle task by default).
 *  Used in Timer ISR to determine if we are running highest priority task or not
 */
task_ctrl_blk *current_task = &(Task_list[0]);

/**
 *  Must always be called in "main" prior to adding other tasks.
 
 *  Initialized element 0 of "Task_list" as the Idle task and sets all
 *  others as empty (state == Undefined) so they can eventually be setup
 *  through calls to "Task_add"
 */
void Task_list_init(void)
{
    int i;
    
    Task_list[0].state = TASK_RUNNING;
    Task_list[0].function = (intptr_t)idle_thread;
    Task_list[0].period = 1;
    Task_list[0].count = 0;
    
    for(i=1;i<NUM_TASKS;i++)
    {
        Task_list[i].state = TASK_UNDEFINED;
        Task_list[i].function = (intptr_t)idle_thread;
        Task_list[i].period = 1;
        Task_list[i].count = 0;
    }
    
    //Clear all aperiodic events
    for(i=0;i<NUM_EVENTS;i++)
    {
        Event_task_list[i] = (task_ctrl_blk *)0;
    }
}

/**
 *  Returns a pointer to the "Task_list" entry of the highest priority active task
 *  (a Task is active if it is in Running or Suspended states)
 */
static inline task_ctrl_blk *get_priority_task(void)
{
    int i;
    int earliest = 0;
    uint32_t earliest_deadline = UINT32_MAX;
    
    // Skip the idle task, it's always going to show up as having a deadline
    // of 0, but skipping it effectivly makes it's deadline UINT32_MAX
    for(i=1;i<NUM_TASKS;i++)
    {
        if((Task_list[i].state == TASK_RUNNING) || (Task_list[i].state == TASK_SUSPENDED))
        {
            if(!Task_list[i].start_offset && (Task_list[i].deadline_remaining < earliest_deadline))
            {
                earliest_deadline = Task_list[i].deadline_remaining;
                earliest = i;
            }
        }
    }
    return Task_list + earliest;
}

/**
 *  Called by application code (main) to setup periodic tasks
 *  Requires pointer to the function that implements the task, as
 *  well as its period (in system ticks = 10ms) and priority (1 to 255, 1 is lowest)
 */
uint8_t Task_add(intptr_t function, uint32_t period, uint32_t start_offset,
                 uint32_t deadline)
{
    int i;
    for(i = 1; (i<NUM_TASKS) && (Task_list[i].state != TASK_UNDEFINED); i++);
    if (i < NUM_TASKS)
    {
        Task_list[i].state = TASK_STOPPED;
        Task_list[i].function = function;
        Task_list[i].period = period;
        Task_list[i].start_offset = start_offset;
        Task_list[i].count = (uint32_t)-1;
        Task_list[i].deadline = deadline;
        return 0;
    }
    return 1;
}


/**
 *  Called by application code (main) to setup aperiodic tasks
 *  Requires pointer to the function that implements the task, as
 *  well as its triggering event and priority (1 to 255, 1 is lowest)
 *
 *  Supported events are in "fate.h", defined in "enum events"
 */
uint8_t Task_event_add(intptr_t function, enum events event, uint32_t deadline)
{
    int i;
    for(i = 1; (i<NUM_TASKS) && (Task_list[i].state != TASK_UNDEFINED); i++);
    if (i < NUM_TASKS)
    {
        Task_list[i].state = TASK_STOPPED;
        Task_list[i].function = function;
        //For aperiodic tasks: period set as 0
        Task_list[i].period = 0;
        //For aperiodic tasks, set count as 1 (so the periodic scheduler never schedules it)
        Task_list[i].count = 1;
        Task_list[i].deadline = deadline;
        
        //Configure Device and Interrupt for corresponding event
        Enable_event(event);
        
        //Set pointer to newly configured task in event-task list
        Event_task_list[event] = &(Task_list[i]);
        
        return 0;
    }
    return 1;
}


/**
 *  Configures Device and Interrupt for corresponding event
 *  Called by "Task_event_add" to setup event for aperiodic tasks
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


/**
 *  Encapsulates inline assembly to get current value of Stack Pointer
 *  Used in Timer ISR (system tick) to access the stack and manipulate the return address,
 *  so we return to the task we want
 */
__inline intptr_t get_current_SP(void)
{
#ifdef __ARMCC_VERSION
    // Keil uVision
    intptr_t spReg;
    __asm {
        MOV spReg, __current_sp()
    }
#elif defined(__GNUC__)
    // GCC
    register intptr_t spReg;
    
    __ASM volatile ("MRS %0, msp\n" : "=r" (spReg) );
#endif
    return spReg;
}

/**
 *  Main scheduler implementation
 *
 *  Occurs every 10ms
 *
 *  Updates "count" in every task so we keep track of time
 *  Based on highest priority currently active task, manipulates stack to change return address
 *  (so we return from ISR to the task we want to run) and updates information in
 *  pointer to current task and Task_list.
 */
void TA0_N_IRQHandler()
{
    intptr_t sp_p;
    int i;
    task_ctrl_blk *new_task;
    
    
#ifdef __ARMCC_VERSION
    //Value of current stack pointer
    sp_p = get_current_SP();
    //Find Exception code: basically makes sp_p point to base of current stack frame - 4
    while((*((intptr_t *)sp_p)) != (intptr_t)0xFFFFFFE9)
        sp_p += (intptr_t)4;
#elif defined(__GNUC__)
    // Ignore warning for casting result of __builtin_frame_address to an integer type
#pragma GCC diagnostic ignored "-Wbad-function-cast"
    //Value of current stack pointer
    sp_p = (intptr_t)__builtin_frame_address(0);
    // Stop ignoring warnings about casting results of functions to the wrong type
#pragma GCC diagnostic pop
    while((*((intptr_t *)sp_p)) != (intptr_t)0xFFFFFFF9)
        sp_p += 4;
#endif
    //Now that we are pointing to base of current stack frame, increment by 0x1C
    //so we are pointing at Return Address
    sp_p += 0x1C;
    
    // If the timer has overflowed we need to update all of our counters
    if (TA0CTL & (uint16_t)(~(BIT0))) {
        //Increment "count" on all tasks, modulo task period
        //Set task as SUSPENDED (active) if it was STOPPED
        //and count is back to 0 (matched period)
        for(i=1;i<8;i++)
        {
            // If the start offset has not expired, decrement it
            if (Task_list[i].start_offset > 0)
            {
                Task_list[i].start_offset--;
            }
            //Don't increment count if task is aperiodic (period == 0)
            else if(Task_list[i].period)
            {
                Task_list[i].count++;
                Task_list[i].count %= Task_list[i].period;
            }
            if(Task_list[i].count == 0)
            {
                if(Task_list[i].state == TASK_STOPPED)
                {
                    Task_list[i].state = TASK_SUSPENDED;
                    // As task becomes ready it's deadline begins to near
                    Task_list[i].deadline_remaining = Task_list[i].deadline;
                }
            }
            // If task is running or suspended, it's deadline is gettting closer
            if ((Task_list[i].state == TASK_SUSPENDED) || (Task_list[i].state == TASK_RUNNING))
            {
                // Cap deadline remaining at 0 so that it doesn't wrap
                if (Task_list[i].deadline_remaining > 0)
                {
                    Task_list[i].deadline_remaining--;
                }
            }
        }
        
        //clear Timer interrupt flag
        TA0CTL &= (uint16_t)(~(BIT0));
    }
    
    //Get pointer to highest priority active (running or suspended) task
    new_task = get_priority_task();
    
    //Is the current highest priority active task not the currently running task?
    if(new_task != current_task)
    {
        // Stop all of the task timers for fixed execution time tasks
        TA1CTL &= ~(TIMER_A_CTL_MC_MASK);
        TA2CTL &= ~(TIMER_A_CTL_MC_MASK);
        TA3CTL &= ~(TIMER_A_CTL_MC_MASK);
        
        //Yes, it is
        //Set current task to "suspended", if it is "running"; it might have stopped itself
        if(current_task->state == TASK_RUNNING)
            current_task->state = TASK_SUSPENDED;
        //Set new task to "running"
        new_task->state = TASK_RUNNING;
        //Update current task pointer
        current_task = new_task;
        //Return to new task by changing return address
        *((intptr_t *)sp_p) = current_task->function;
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
            *((intptr_t *)sp_p) = current_task->function;
        }
        //No, current task is not finished
        //Return to same task (do nothing)
    }
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

