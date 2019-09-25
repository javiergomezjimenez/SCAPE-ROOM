

//------------------- CONEXIONADO ---------------------//
//       P1.6-> LA LÍNEA DE DATOS DEL RECEPTOR IR      //
//                   P2.3-> SWITCH                     //
//       P2.0-> LATCH (P12 DEL 74HC595 / NARANJA)      //
//          P1.(1-4)-> SELECCIÓN BIT DISPLAY           //
//                P1.5-> CLOCK (VERDE)                 //
//               P1.7-> DATA (AMARILLO)                //
//                 SALIDA -> PIN 2.4                   //
//----------------- FIN CONEXIONADO -------------------//

#include <msp430.h>
#include "start5.h"
#define TDELAY  100
#define SWITCH  BIT3
#define SALIDA  BIT4



int     i = 0, j = 0, k = 0;
volatile int32	irPacket;
char   irPacket2;
volatile char ENABLE = 0;
unsigned int PASSWORD [4];
const unsigned int PASSWORD_CORRECTA [] = {1, 9, 2, 8};
volatile int16	packetData[48];
volatile int8	newPacket = FALSE;
volatile int8	packetIndex = 0;
char CORRECT = 0, PRIMERA = 1;



// -----------------------------------------------------------------------

#define CSH     P2OUT |= BIT0   //Pone a 0 P2.0
#define CSL     P2OUT &= ~BIT0  //pone a 1 P2.0

const char disp[]={0xE7, 0x21, 0xCB, 0x6B, 0x2D, 0x6E, 0xEE, 0x23, 0xEF, 0x2F, 0x00};
const char segment[]={BIT3, BIT2, BIT1, BIT4};

// -----------------------------------------------------------------------

unsigned char manda_spi(unsigned char data)
{
    UCB0TXBUF = data;
    while (UCB0STAT & UCBUSY);
    return UCB0RXBUF;
}

void display_dato (int PASS)
{
    P1OUT |= (BIT1|BIT2|BIT3|BIT4); // APAGO TODOS
    CSL;
    manda_spi(disp[PASS]);
    CSH;
    P1OUT &= ~(segment[j]); // ENCIENDO EL NUEVO
}

void conf_reloj(){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    if (CALBC1_8MHZ != 0xFF) {
        __delay_cycles(100000);
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_8MHZ;      /* Set DCO to 8MHz */
        DCOCTL = CALDCO_8MHZ;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}
// -----------------------------------------------------------------------
void main(void) {

    IFG1=0;                     // clear interrupt flag1
    WDTCTL=WDTPW+WDTHOLD;       // stop WD

    conf_reloj();

    P1SEL  &= ~BIT6;                        // Setup P1.6 as GPIO not XIN
    P1SEL2 &= ~BIT6;
    P1DIR &= ~BIT6;
    P1IFG &= ~BIT6;                     // Clear any interrupt flag
    P1IE  |= BIT6;                      // Enable PORT 1 interrupt on pin change

    // ------------------------------------------------------------ //
    // ENTRADA DEL SWITCH GENERAL QUE DA PASO A LA SIGUIENTE PRUEBA //
    // ------------------------------------------------------------ //
    P2DIR &= ~SWITCH;
    P2SEL &= ~SWITCH;
    P2SEL2 &= ~SWITCH;
    P2IES|= SWITCH;       /* P2.3 act. flanco de subida */
    P2IFG = 0;          /* Borra flags pendientes */
    P2IE |= SWITCH;       /*Activa interrupcion en P2.3*/

    // ------------------------------------------------------------ //
    //                SALIDA PARA LA SIGUIENTE PRUEBA               //
    // ------------------------------------------------------------ //
    P2SEL &= ~SALIDA;
    P2SEL2 &= ~SALIDA;
    P2DIR |= SALIDA;
    P2OUT &= ~SALIDA; //SALIDA APAGADA AL INICIO


    /*-------------------- PINES DEL 7segmentos:-----------------------*/
    P1SEL2 |= BIT5 | BIT7;  //P1.7: MOSI, P1.5: CLK
    P1SEL |= BIT5 | BIT7;   // 11 PERIFÉRICO SECUNDARIO
    P1DIR |= BIT5 | BIT7;   // P1.5 SALIDA P1.7
    P2DIR |= BIT0;   //P2.0: CS manejado manualmente
    // SALIDAS INVOLUCRADAS EN LA ELECCIÓN DEL BIT //
    P1SEL &= ~(BIT1|BIT2|BIT3|BIT4);
    P1SEL2 &= ~(BIT1|BIT2|BIT3|BIT4);
    P1DIR |= (BIT1|BIT2|BIT3|BIT4);
    P1OUT |= (BIT1|BIT2|BIT3|BIT4); //SON ACTIVOS A NIVEL BAJO, LOS INICIALIZAMOS A UNO PARA QUE NO SE VEA NADA

    /*------ Configuración de la USCI-A para modo SPI:----------*/
    UCB0CTL1 |= UCSWRST; // Resetea la USCI
    /* Registro de control 0:
     * UCCKPH – Fase 1, Polaridad 0 (flanco subida)
     * UCMSB – Empieza MSB
     * UCMST -- Master mode
     * UCMODE_0 - 3-Pin SPI
     * UCSYNC -- Synchronous Mode     */
    UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;
    /* Control Register 1
     * UCSSEL_2 --> SMCLK
     * UCSWRST --> Mantiene el reset mientras configuro */
    UCB0CTL1 = UCSSEL_2 | UCSWRST;
    UCB0BR0 = 2; /* Velocidad: SMCLK/2 */
    UCB0CTL1 &= ~UCSWRST; /* Quita el reset: habilita USCI */
    // ----------------------------------------------- //

    HIGH_1_LOW;

    TA0CCR0 = 0x8000;                   // create a 16mS roll-over period
    TACTL &= ~TAIFG;                    // clear flag before enabling interrupts = good practice
    TACTL = ID_3 | TASSEL_2 | MC_1;     // Use 1:1 presclar off MCLK and enable interrupts

    _enable_interrupt();

    while(1)  {
        if (ENABLE)
        {
            if (!CORRECT)
            {

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
                    else if (!( (PASSWORD[j-1]==irPacket2 && j>0) || (PASSWORD[3]==irPacket2 && j==0) ))
                    {
                        PASSWORD[j]=irPacket2;
                        display_dato(PASSWORD[j]);
                        j++;
                        if (j>3)
                        {
                            j = 0;
                            CORRECT = 1;
                            for (k=0; k<4; k++)
                            {
                                if (PASSWORD[k]!=PASSWORD_CORRECTA[k]) CORRECT = 0;
                            }
                        }

                    }


                }
                if (newPacket){
                    i=0;
                    newPacket = FALSE;
                }
            }
            while (CORRECT)
            {
                TA1CCTL0 = CM_0 | CCIS_0 | OUTMOD_0 | CCIE; // Modo OC e int. de CC
                TA1CTL = TASSEL_1 | ID_0 | MC_1;            // ACLK, sin divisor, Modo Up
                P2OUT |= BIT4; // SALIDA PARA EL MOTOR
                TA1CCR0 = 999;                            // periodo 999 ~ 100ms
                if (PRIMERA)
                {
                    display_dato(8);
                    PRIMERA=0;
                    P2OUT |= SALIDA;
                }
                else
                {
                    P1OUT |= (BIT1|BIT2|BIT3|BIT4); // APAGO TODOS
                    P1OUT &= ~(segment[j]); // ENCIENDO EL NUEVO
                    P2OUT |= SALIDA;
                }
                LPM3;
            }
        }
        else
        {
            CORRECT = 0;
            P2IE |= SWITCH;       /* Activa interrupcion en el SWITCH */
            LPM0;
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

#pragma vector = TIMER1_A0_VECTOR           // This is from the MSP430G2553.h file
__interrupt void timercorrect (void) {
    j++;
    if (j>3) j=0;
    LPM3_EXIT;
}


#pragma vector=PORT2_VECTOR
__interrupt void RUTINA_INTERRUPCIÓN_SWITCH_ON(void)
{
    if(P2IN & SWITCH)
    {
        ENABLE = 1;
        P2IFG &= ~SWITCH;      // Borra flag de interrupcion
        P2IE &= ~SWITCH;       // APAGA INTERRUPCIONES DEL SWITCH
        LPM0_EXIT;
    }
}
