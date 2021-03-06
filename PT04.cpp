#include <avr/pgmspace.h>
#include "PT04.h"
#include <Arduino.h>

/** Interrupt based cactus v2 decoder.  Connect XY-MK-5V receiver data to arduino pin 2.
 */

const int TOLERANCE = 20;

// hi-lo timing in usec. this matches pt-04 trigger when dips are set to 00
const PROGMEM prog_uint16_t pt04_pattern[] = {
	0, 180, // 0 for ignore 
	52, 72,  
	52, 72,
	52, 72,
	52, 72,
//  60, 180
};

const int pt04_patternLength = sizeof(pt04_pattern)/sizeof(pt04_pattern[0]);

volatile static int pt04_triggered;
volatile static int pt04_count;
volatile static int pt04_state;
volatile static long pt04_t;

PT04::PT04()
{
	reset();
	pt04_triggered=0;

	pinMode(2, INPUT);

#ifdef __AVR_ATmega32U4__
	// 32u4 chip has INT1 connected to PIN2
	attachInterrupt(1, pt04, CHANGE);
#else
	// all the other chips have INT0 in PIN2
	attachInterrupt(0, pt04, CHANGE);
#endif
}

bool PT04::hasTriggered()
{
	int v = pt04_triggered;
	pt04_triggered=0;
	return v;
}

void PT04::reset()
{
	pt04_state=LOW;
	pt04_count=0;
	pt04_t=0;
}

void PT04::pt04()
{
	long t1=micros();

    if(digitalRead(2)!=pt04_state) 
    {
		// wrong state, restart
		reset();
		return;
    }
    
    int currentSlot = pgm_read_word_near(&pt04_pattern[pt04_count]);
    
    if(currentSlot)
    {
		long dt=(t1-pt04_t)-currentSlot;
		if(dt<-TOLERANCE || dt>TOLERANCE) 
		{
			// did not meet timing, restart
			reset();
			return;
		}
    }
    pt04_t=t1;
    pt04_state=!pt04_state;
    pt04_count++;
    
    if(pt04_count>=pt04_patternLength) 
    {
		// pattern complete, set trigger flag and restart
		pt04_triggered=1;
		reset();
    }
}
