//-----------------------------------------------------//
// CONECTAR EN P1.6 LA LÍNEA DE DATOS DEL RECEPTOR IR  //
//-----------------------------------------------------//

#include <msp430g2553.h>
#include "start5.h"
#define TDELAY 100

int     i = 0, j = 0;
volatile int32	irPacket;
int   irPacket2;
unsigned PASSWORD [4];
volatile int16	packetData[48];
volatile int8	newPacket = FALSE;
volatile int8	packetIndex = 0;

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
void main(void) {

    IFG1=0;                     // clear interrupt flag1
    WDTCTL=WDTPW+WDTHOLD;       // stop WD

    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;

    P1SEL  &= ~BIT6;                        // Setup P1.6 as GPIO not XIN
    P1SEL2 &= ~BIT6;
    P1DIR &= ~BIT6;
    P1IFG &= ~BIT6;                     // Clear any interrupt flag
    P1IE  |= BIT6;                      // Enable PORT 1 interrupt on pin change

    HIGH_1_LOW;

    TA0CCR0 = 0x8000;                   // create a 16mS roll-over period
    TACTL &= ~TAIFG;                    // clear flag before enabling interrupts = good practice
    TACTL = ID_3 | TASSEL_2 | MC_1;     // Use 1:1 presclar off MCLK and enable interrupts

    _enable_interrupt();

	while(1)  {
		if(i==34){
			switch (irPacket)
			{
			case 16712445:
			    irPacket2 = 10;
			    __delay_cycles(TDELAY);
			    break;
			case 16738455:
			    irPacket2 = 0;
			    __delay_cycles(TDELAY);
			    break;
			case 16724175:
			    irPacket2 = 1;
			    __delay_cycles(TDELAY);
			    break;
			case 16718055:
			    irPacket2 = 2;
			    __delay_cycles(TDELAY);
			    break;
			case 16743045:
			    irPacket2 = 3;
			    __delay_cycles(TDELAY);
			    break;
			case 16716015:
			    irPacket2 = 4;
			    __delay_cycles(TDELAY);
			    break;
			case 16726215:
			    irPacket2 = 5;
			    __delay_cycles(TDELAY);
			    break;
			case 16734885:
			    irPacket2 = 6;
			    __delay_cycles(TDELAY);
			    break;
			case 16728765:
			    irPacket2 = 7;
			    __delay_cycles(TDELAY);
			    break;
			case 16730805:
			    irPacket2 = 8;
			    __delay_cycles(TDELAY);
			    break;
			case 16732845:
			    irPacket2 = 9;
			    __delay_cycles(TDELAY);
			    break;
			}
			irPacket = 0;
			if (irPacket2==10){
			    j=0;
			}
			if (!( (PASSWORD[j-1]==irPacket2 && j>0) || (PASSWORD[3]==irPacket2 && j==0) ))
			{
			    PASSWORD[j]=irPacket2;
			    j++;
			    if (j>3)
			    {
			        j=0;
			    }
			}


		}
		if (newPacket){
		    i=0;
		    newPacket = FALSE;

		}

	} // end infinite loop
} // end main



#pragma vector = PORT1_VECTOR			// This is from the MSP430G2553.h file

__interrupt void pinChange (void) {

	int8	pin;
	int16	pulseDuration;			// The timer is 16-bits

	if (IR_PIN)		pin=1;	else pin=0;

	switch (pin) {					// read the current pin level
		case 0:						// !!!!!!!!!NEGATIVE EDGE!!!!!!!!!!
			pulseDuration = TAR;
			if((pulseDuration < maxLogic0Pulse) && (pulseDuration > minLogic0Pulse)){
				irPacket = (irPacket << 1) | 0;
			}
			else if((pulseDuration < maxLogic1Pulse) && (pulseDuration > minLogic1Pulse)){
				irPacket = (irPacket << 1) | 1;
			}
			packetData[packetIndex++] = pulseDuration;
			TACTL = 0;				//turn off timer A e.w.
			LOW_1_HIGH; 				// Setup pin interrupr on positive edge
			i++;
			break;

		case 1:							// !!!!!!!!POSITIVE EDGE!!!!!!!!!!!
			TAR = 0x0000;						// time measurements are based at time 0
			TA0CCR0 = 0x2710;
			TACTL = ID_3 | TASSEL_2 | MC_1 | TAIE;
			HIGH_1_LOW; 						// Setup pin interrupr on positive edge
			break;
	} // end switch
	P1IFG &= ~BIT6;			// Clear the interrupt flag to prevent immediate ISR re-entry

} // end pinChange ISR


#pragma vector = TIMER0_A1_VECTOR			// This is from the MSP430G2553.h file
__interrupt void timerOverflow (void) {

	TACTL = 0;
	TACTL ^= TAIE;
	packetIndex = 0;
	newPacket = TRUE;
	TACTL &= ~TAIFG;
}
