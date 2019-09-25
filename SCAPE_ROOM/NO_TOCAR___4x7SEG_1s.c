
//  CONEXIONADO //
// P2.0-> LATCH (P12 DEL 74HC595 / NARANJA)//
// P1.5-> CLOCK (VERDE) //
// P1.7-> DATA (AMARILLO) //





#include <msp430.h>

/*
 * main.c
 */

#define CSH     P2OUT |= BIT0   //Pone a 0 P2.0
#define CSL     P2OUT &= ~BIT0  //pone a 1 P2.0

const char disp[]={0xE7, 0x21, 0xCB, 0x6B, 0x2D, 0x6E, 0xEE, 0x23, 0xEF, 0x2F, 0x00};
const char segment[]={BIT3, BIT2, BIT1, BIT4};
const int numero[]={1, 9, 2, 8};
volatile int bit = 0;

///// FUNCIONES /////
unsigned char manda_spi(unsigned char data)
{
    UCB0TXBUF = data;
    while (UCB0STAT & UCBUSY);
    return UCB0RXBUF;
}

void display_dato ()
{
    P1OUT |= (BIT1|BIT2|BIT3|BIT4); // APAGO TODOS
    CSL;
    manda_spi(disp[numero[bit]]);
    CSH;
    P1OUT &= ~(segment[bit]); // ENCIENDO EL NUEVO
}


#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
    bit--;
    if (bit<0) bit=3;
    display_dato();
}

void conf_reloj(){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
        DCOCTL = CALDCO_1MHZ;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}


// FUNCIÓN MAIN //
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
    conf_reloj();

    /*------ Pines de E/S involucrados:------------------------*/
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

    /*Configuración del Timer0 para generar la interrupción de 1s--*/

        TA0CCTL0 = CM_0 | CCIS_0 | OUTMOD_0 | CCIE; // Modo OC e int. de CC
        TA0CTL = TASSEL_1 | ID_0 | MC_1;            // ACLK, sin divisor, Modo Up
        TA0CCR0 = 12000;                            // periodo 12000: 1s

    __bis_SR_register(GIE); // Habilita las interrupciones globalmente
    /*while(1){

        }*/

}


