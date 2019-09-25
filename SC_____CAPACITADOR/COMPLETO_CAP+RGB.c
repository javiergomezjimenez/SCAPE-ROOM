
                                                    // CONEXIONADO //
                                        // RGB (NIVEL ALTO) -> PIN 2.2/2.3/2.5 //
                                               // CAPACITIVO -> PIN 2.4 //
                                             // SWITCH GENERAL -> PIN 2.7 //
                                                 // SALIDA -> PIN 2.6 //
                                                  // FIN CONEXIONADO //




#include  "msp430g2553.h"

/* Define User Configuration values */
/*----------------------------------*/
/* Defines WDT SMCLK interval for sensor measurements*/
//#define WDT_meas_setting (DIV_SMCLK_512)
#define WDT_meas_setting (DIV_SMCLK_8192)
/* Defines WDT ACLK interval for delay between measurement cycles*/
#define WDT_delay_setting (DIV_ACLK_512)

/* Sensor settings*/
#define PROXIMITY_LVL     10                     // ESTA ES LA MEDIDA A PARTIR DE LA CUAL CONSIDERAREMOS QUE HEMOS TOCADO
/*Set to ~ half the max delta expected*/
#define LIMITE_CODIGO     70

/* Definitions for use with the WDT settings*/
#define DIV_ACLK_32768  (WDT_ADLY_1000)     // ACLK/32768
#define DIV_ACLK_8192   (WDT_ADLY_250)      // ACLK/8192
#define DIV_ACLK_512    (WDT_ADLY_16)       // ACLK/512
#define DIV_ACLK_64     (WDT_ADLY_1_9)      // ACLK/64
#define DIV_SMCLK_32768 (WDT_MDLY_32)       // SMCLK/32768
#define DIV_SMCLK_8192  (WDT_MDLY_8)        // SMCLK/8192
#define DIV_SMCLK_512   (WDT_MDLY_0_5)      // SMCLK/512
#define DIV_SMCLK_64    (WDT_MDLY_0_064)    // SMCLK/64

#define SENSOR_DIR  P2DIR
#define SENSOR_PIN  BIT4
#define SENSOR_SEL  P2SEL
#define SENSOR_SEL2 P2SEL2

#define AVERAGE_TIMES       20
#define FAST_SCAN_TIMEOUT   10

#define SWITCH BIT7                         // ENTRADA PARA ACTIVAR LA PRUEBA
#define SALIDA  BIT6                // SALIDA PARA ACTIVAR LA SIGUIENTE PRUEBA


#define CONTEO_COLORES   500 //VALOR DEL CONTADOR QUE CAMBIA DE COLORES

// VARIABLES GLOBALES
volatile int ENABLE = 0;    //SWITCH GENERAL
int base_cnt, meas_cnt;
int delta_cnt;
char key_pressed;
int cycles;
int cont_cod=0;
char estado=0;//para el cambio de colores del codigo

volatile char CODIGO=0; //FLAG QUE INDICA QUE NOS HEMOS ACERCADO LO SUFICIENTE PARA MOSTRAR EL CODIGO DE COROLES

/* System Routines*/
void measure_count(void);                   // Measures each capacitive sensor
void rgb(unsigned char r,unsigned char g, unsigned char b);//rutina de cambio de colores


/* Main Function*/
void main(void)
{
    int i;

    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    // Configure Clocks
    BCSCTL1 = 0x008D;                         // Set DCO to 8MHz
    DCOCTL = 0x009B;
//  f           CALBC1      CALDCO
//  1MHz        0x0087      0x0043
//  8MHz        0x008D      0x009B
//  12MHz       0x008F      0x0006
//  16MHz       0x008F      0x00A4
    BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO

    IE1 |= WDTIE;                             // enable WDT interrupt

    P1OUT = 0x00;               // INICIALIZAMOS A CERO PARA REDUCIR GASTO

    P2OUT = 0x00;

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



    __bis_SR_register(GIE);                   // Enable interrupts

    measure_count();                          // REALIZAMOS UNA PRIMERA MEDIDA DE LA CAPACIDAD
    base_cnt = meas_cnt;

    for (i = AVERAGE_TIMES; i > 0; i--)       // CALCULAMOS LA MEDIA DE (AVERAGE_TIMES) MEDIDAS DEL CAPACITIVO PARA MARCAR LA BASE
    {
        measure_count();                      //MEDIDA NUEVA
        base_cnt = (meas_cnt + base_cnt) >> 1;//SUMAMOS LA MEDIDA NUEVA A LA BASE Y DIVIDIMOS POR 2 CON UN DESPLAZAMIENTO >>1
    }

    //CONFIGURACIÓN DEL TIMER 1 PARA LA SALIDA AL LED TRICOLOR
    /*  TA1: CON SMCLK*/
    TA1CTL=TASSEL_2|ID_0| MC_1;         //SMCLK, DIV=1 (2MHz), UP
    TA1CCTL0=OUTMOD_7;     //CCIE=1
    TA1CCTL1=OUTMOD_7;
    TA1CCTL2=OUTMOD_7;
    TA1CCR0=100;
    TA1CCR1=100;
    TA1CCR2=100;

    /* Main loop starts here*/
    while (1)
    {
        if (ENABLE)
        {

            P2DIR|= (BIT2+BIT5+BIT3);          // RECONFIGURACIÓN DE LAS SALIDAS P2.2 VERDE P2.1 ROJO P2.5 AZUL

            P2SEL |= (BIT2+BIT5+BIT3);
            P2SEL2 &= ~(BIT2+BIT5+BIT3);

            key_pressed = 0;                        // PARTIMOS DE QUE NADA ESTA PULSADO

            measure_count();                        // NUEVA MEDIDA

            delta_cnt = base_cnt - meas_cnt;        // CALCULA LA DIFERENCIA ENTRE LA NUEVA MEDIDA Y LA BASE

            /* RECALCULAMOS LA BASE EN CASO DE QUE LA CAPACIDAD DISMINUYA*/
            if (delta_cnt < 0)                      // SI LA DIFERENCIA ES NEGATIVA
            {                                       // beyond baseline, i.e. cap dec
                base_cnt = (base_cnt + meas_cnt) >> 1; // HACE UNA NUEVA MEDIA IGUAL QUE ANTES
                delta_cnt = 0;                    // RESETEA LA DIFERENCIA
            }

            // LED2 indicator proximity threshold
            /*if (delta_cnt > PROXIMITY_LVL)          //CASO DE QUE LA DIFERENCIA SUPERE NUESTRO VALOR LÍMITE
            {
                BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0;// ACLK/(0:1,1:2,2:4,3:8)
                cycles = FAST_SCAN_TIMEOUT;
            }
            else            //CUANDO YA HEMOS LLEGADO A UN VALOR INFERIOR, NOS MANTENEMOS ENCENDIDOS DURANTE UNA SERIE DE CICLOS POR SI ACASO
            {
                cycles--;
                if (cycles > 0)
                {
                    BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
                }
                else
                {
                    BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_3; // ACLK/(0:1,1:2,2:4,3:8)
                    cycles = 0;
                }
            }*/

            //Reproduccion del codigo de colores

            if(delta_cnt>LIMITE_CODIGO){
                P2DIR|= (BIT2+BIT5+BIT3);          // RECONFIGURACIÓN DE LAS SALIDAS P2.2 VERDE P2.1 ROJO P2.5 AZUL

                P2SEL |= (BIT2+BIT5+BIT3);
                P2SEL2 &= ~(BIT2+BIT5+BIT3);

                //CONFIGURACION DEL TIMER 1

                TA1CTL=TASSEL_2|ID_0| MC_1;         //SMCLK, DIV=1 (2MHz), UP
                TA1CCTL0=OUTMOD_7;
                TA1CCTL1=OUTMOD_7;
                TA1CCTL2=OUTMOD_7;

                switch(estado){     //CADA ESTADO SERÁ UN COLOR
                case 0:     //
                    rgb(80,0,0);

                    break;
                case 1:     //
                    rgb(40,40,0);
                    break;
                case 2:     //
                    rgb(40,0,40);
                    break;
                case 3:     //
                    rgb(40,40,40);
                    P2OUT |= SALIDA;    // EN EL ÚLTIMO COLOR DE LA SERIE SE ACTIVA LA SIGUIENTE PRUEBA
                    break;
                }

            }
            else
            {
                P2SEL &= ~(BIT2+BIT5+BIT3);// En caso de no estar en la zona del código desconectamos las salidas en pwm
            }

            if(cont_cod<CONTEO_COLORES){
                cont_cod++;
            }
            else{
                cont_cod=0;
                if(estado<3) estado++;
                else          estado=0;
            }
        }
    }
} //FIN DEL MAIN

/* Measure count result (capacitance) of each sensor*/
/* Routine setup for four sensors, not dependent on NUM_SEN value!*/

void measure_count(void)
{

    TA0CTL = TASSEL_3 + MC_2;                   // TACLK, cont mode
    TA0CCTL1 = CM_3 + CCIS_2 + CAP;             // Pos&Neg,GND,Cap


    /*Configure Ports for relaxation oscillator*/
    /*The P2SEL2 register allows Timer_A to receive it's clock from a GPIO*/
    /*See the Application Information section of the device datasheet for info*/
    SENSOR_DIR &= ~ SENSOR_PIN; // P2.2 is the input used here to get sensor signal
    SENSOR_SEL &= ~ SENSOR_PIN; // PxSEL.y = 0 & PxSEL2.y = 1 to enable Timer_A clock source form sensor signal
    SENSOR_SEL2 |= SENSOR_PIN;

    /*Setup Gate Timer*/
    WDTCTL = WDT_meas_setting; // WDT, SMCLK, interval timer - SCMK/512 = 1MHz/512 = 512us interval
    TA0CTL |= TACLR;                            // Clear Timer_A TAR
    __bis_SR_register(LPM0_bits + GIE);         // Wait for WDT interrupt
    TA0CCTL1 ^= CCIS0; // Create SW capture of CCR1. Because capture mode set at Pos&Neg so that switching input signal between VCC & GND will active capture
    meas_cnt = TACCR1;                      // Save result
    WDTCTL = WDTPW + WDTHOLD;               // Stop watchdog timer
    SENSOR_SEL2 &= ~SENSOR_PIN; // Disable sensor signal line to Timer_A clock source

    TA0CTL = 0;                                 // Stop Timer_A
}


void rgb(unsigned char r,unsigned char g, unsigned char b)
{
    TA1CCR0=r*39;       //rojo
    TA1CCR1=g*39;       //verde
    TA1CCR2=b*39;       //azul
}

/* Watchdog Timer interrupt service routine*/
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    TA0CCTL1 ^= CCIS0;                        // Create SW capture of CCR1
    __bic_SR_register_on_exit(LPM3_bits);     // Exit LPM3 on reti
}

#pragma vector=PORT2_VECTOR
__interrupt void RUTINA_INTERRUPCIÓN_SWITCH_ON(void)
{
    if(P2IN & SWITCH)
    {
        ENABLE = 1;
        P2IFG &= ~SWITCH;      // Borra flag de interrupcion
        P2IE &= ~SWITCH;       // APAGA INTERRUPCIONES DEL SWITCH
    }
}
