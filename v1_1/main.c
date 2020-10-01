#include <msp.h>

//Must always include our OS header file
#include "fate.h"


//Functions that implement our 2 periodic tasks
//Return type and arguments must always be void
//For now, no local variables
//No calling other functions
//When execution is finished, must always call "Task_stop" with its name
//NOT return: that would probably crash our kernel
void LED_toggle(void)
{
	P1OUT ^= (uint8_t)BIT0;
	Task_stop((uint32_t)LED_toggle);
}
void LED_RGB_toggle(void)
{
	P2OUT = (uint8_t)((P2OUT & (uint8_t)0xF8) | ((P2OUT + (uint8_t)1) & ((uint8_t)7)));
	Task_stop((uint32_t)LED_RGB_toggle);
}


int main()
{
	
	//configure Ports
	
	//LED p1.0 (Red)
	P1SEL0 &= (uint8_t)(~(BIT0));
	P1SEL1 &= (uint8_t)(~(BIT0));
	P1DIR |= (uint8_t)BIT0;
	P1OUT |= (uint8_t)BIT0;
	
	//LED P2.0.1.2 (RGB)
	P2SEL0 &= (uint8_t)(~(BIT0)|(BIT1)|(BIT2));
	P2SEL1 &= (uint8_t)(~((BIT0)|(BIT1)|(BIT2)));
	P2DIR |= (uint8_t)((BIT0)|(BIT1)|(BIT2));
	P2OUT |= (uint8_t)((BIT0)|(BIT1)|(BIT2));
	
	//Initialize Task list, includes setting up idle task
	//Always the first function that must be called
	Task_list_init();
	
	//Initialize our periodic tasks, with periods 100 and 200 ticks (1 and 2s), respectively
	Task_add((uint32_t)LED_toggle,(uint32_t) 100, (uint32_t) 1);
	
	Task_event_add((uint32_t)LED_RGB_toggle, SWITCH_P1_4, (uint32_t) 1);
	
	//This will begin scheduling our tasks 
	Task_schedule();
	
	return 0;
}
