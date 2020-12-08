#include <msp.h>

//Must always include our OS header file
#include "fate.h"


// Prototypes
void LED_toggle(void);
void LED_RGB_toggle(void);
void Task_1 (void);
void Task_2 (void);
void Task_3 (void);

//Functions that implement our 2 periodic tasks
//Return type and arguments must always be void
//For now, no local variables
//No calling other functions
//When execution is finished, must always call "Task_stop" with its name
//NOT return: that would probably crash our kernel
void LED_toggle(void)
{
	P1OUT ^= (uint8_t)BIT0;
	Task_stop((intptr_t)LED_toggle);
}
void LED_RGB_toggle(void)
{
	P2OUT = (uint8_t)((P2OUT & (uint8_t)0xF8) | ((P2OUT + (uint8_t)1) & ((uint8_t)7)));
	Task_stop((intptr_t)LED_RGB_toggle);
}

// Tasks with fixed execution time
void Task_1 (void)
{
    if (!(TIMER_A1->CTL & (TIMER_A_CTL_TASSEL_1))) {
        // Timer is not already running
        
        // Clock from ACLK, divide by 8
        TIMER_A1->CTL = TIMER_A_CTL_TASSEL_1 | TIMER_A_CTL_ID__8;
        TIMER_A1->CTL |= TIMER_A_CTL_CLR;
        // Set top
        TIMER_A1->CCR[0] = 4096;
    }
    // Start in up mode
    TIMER_A1->CTL |= TIMER_A_CTL_MC_1;
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    P2->OUT |= (1<<2);
    
    // Wait for overflow
    while (!(TIMER_A1->CTL & (TIMER_A_CTL_IFG)));
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    
    // Disable timer
    TIMER_A1->CTL = 0;
    Task_stop((intptr_t)Task_1);
}

void Task_2 (void)
{
    if (!(TIMER_A2->CTL & (TIMER_A_CTL_TASSEL_1))) {
        // Timer is not already running
        
        // Clock from ACLK, divide by 8
        TIMER_A2->CTL = TIMER_A_CTL_TASSEL_1 | TIMER_A_CTL_ID__8;
        TIMER_A2->CTL |= TIMER_A_CTL_CLR;
        // Set top
        TIMER_A2->CCR[0] = 40960;
    }
    // Start in up mode
    TIMER_A2->CTL |= TIMER_A_CTL_MC_1;
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    P2->OUT |= (1<<0);
    
    // Wait for overflow
    while (!(TIMER_A2->CTL & (TIMER_A_CTL_IFG)));
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    
    // Disable timer
    TIMER_A2->CTL = 0;
    Task_stop((intptr_t)Task_2);
}

void Task_3 (void)
{
    if (!(TIMER_A3->CTL & (TIMER_A_CTL_TASSEL_1))) {
        // Timer is not already running
        
        // Clock from ACLK, divide by 8
        TIMER_A3->CTL = TIMER_A_CTL_TASSEL_1 | TIMER_A_CTL_ID__8;
        TIMER_A3->CTL |= TIMER_A_CTL_CLR;
        // Set top
        TIMER_A3->CCR[0] = 12288;
    }
    // Start in up mode
    TIMER_A3->CTL |= TIMER_A_CTL_MC_1;
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    P2->OUT |= (1<<1);
    
    // Wait for overflow
    while (!(TIMER_A3->CTL & (TIMER_A_CTL_IFG)));
    
    P2->OUT &= ~((1<<0)|(1<<1)|(1<<2));
    
    // Disable timer
    TIMER_A3->CTL = 0;
    Task_stop((intptr_t)Task_3);
}


int main(void)
{
	
	//configure Ports
	
	//LED p1.0 (Red)
	P1SEL0 &= (uint8_t)(~(BIT0));
	P1SEL1 &= (uint8_t)(~(BIT0));
	P1DIR |= (uint8_t)BIT0;
	P1OUT &= (uint8_t)(~(BIT0));
	
	//LED P2.0.1.2 (RGB)
	P2SEL0 &= (uint8_t)(~(BIT0)|(BIT1)|(BIT2));
	P2SEL1 &= (uint8_t)(~((BIT0)|(BIT1)|(BIT2)));
	P2DIR |= (uint8_t)((BIT0)|(BIT1)|(BIT2));
	P2OUT &= (uint8_t)(~((BIT0)|(BIT1)|(BIT2)));
	
	//Initialize Task list, includes setting up idle task
	//Always the first function that must be called
	Task_list_init();
	
	//Initialize periodic task, with periods 100
	//Task_add((intptr_t)LED_toggle, 100, 150, 10000);
    // Aperiodic task pased on P1.4 button
	//Task_event_add((intptr_t)LED_RGB_toggle, SWITCH_P1_4, 100);
    
    Task_add((uint32_t)Task_1, 1500, 300, 100);
    Task_add((uint32_t)Task_2, 1500, 0, 1500);
    Task_add((uint32_t)Task_3, 1500, 100, 700);

	//This will begin scheduling our tasks 
	Task_schedule();
	
	return 0;
}
