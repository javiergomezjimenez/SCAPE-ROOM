

                                                    // CONEXIONADO //
                                               // LED NORMAL -> PIN 1.0 //
                                               // CAPACITIVO -> PIN 2.4 //
                                             // SWITCH GENERAL -> PIN 2.7 //
                                                 // SALIDA -> PIN 2.6 //
                                                  // FIN CONEXIONADO //


#include  "msp430g2553.h"


/* RELOJ DEL WDT PARA LOS INTERVALOS ENTRE LAS MEDIDAS*/
#define WDT_meas_setting (DIV_SMCLK_8192)
/* DEFINICION DEL TIEMPO DE DELAY QUE EL WATCHDOG TIMER VA A ESPERAR ENTRE MEDIDAS*/
#define WDT_delay_setting (DIV_ACLK_512)

/* Configuracion del sensor capacitivo*/
#define PROXIMITY_LVL     40                   // valor límite para considerar PRESENCIA BUENO 110

///*DEFINICIONES PARA EL USO DEL WDT*/

#define DIV_SMCLK_8192  (WDT_MDLY_8)        // SMCLK/8192

#define LED_1   (0x01)                      // P1.0 LED output
#define     LED_2    BIT6                      // P1.6 LED (FUNCIONA DIGITALMENTE, SE ENCIENDE O SE APAGA EN UN RANGO)

#define SENSOR_DIR  P2DIR
#define SENSOR_PIN  BIT4
#define SENSOR_SEL  P2SEL
#define SENSOR_SEL2 P2SEL2

#define AVERAGE_TIMES       50      //NUMERO DE VECES QUE SE VA A CALCULAR LA MEDIA DE CAP, PRECISION
#define FAST_SCAN_TIMEOUT   100     //NUMERO DE CICLOS QUE EL LED ROJO SE VA A QUEDAR ENCENDIDO TRAS RETIRAR LA MANO (TIEMPO DE SEGURIDAD)

#define SWITCH BIT7                 // ENTRADA PARA ACTIVAR LA PRUEBA
#define SALIDA  BIT6                // SALIDA PARA ACTIVAR LA SIGUIENTE PRUEBA

// VARIABLES GLOBALES
volatile int ENABLE = 0;    //SWITCH GENERAL
int base_cnt, meas_cnt;     //CAPACITANCIA DE REFERENCIA, CAPACITANCIA MEDIDA
int delta_cnt;              //DIFERENCIA ENTRE MEDIDA Y REFERENCIA
char key_pressed;           //BANDERA QUE INDICA PRESENCIA
int cycles;                 //CICLOS ACTIVOS DE ENCENDIDO DEL LED QUE INDICA EL NIVEL DE PROXIMIDAD


void measure_count(void);                   // RUTINA DE MEDIDA DE LAS CAPACITANCIAS


void main(void)
{
    int i;

    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    // Configure Clocks
    BCSCTL1 = 0x008D;                         // Set DCO to 8MHz
    DCOCTL = 0x009B;

    BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO

    IE1 |= WDTIE;                             // enable WDT interrupt

    // Configure Ports
    P2SEL = 0x00;                             // No XTAL
    P1DIR = LED_1 | LED_2;                    // P1.0 & P1.6 = LEDs
    P1OUT = 0x00;                       // INICIALIZAMOS A CERO PARA REDUCIR GASTO Y PARA QUE ESTÉ AL INICIO APAGADO EL LED
    P2OUT = 0x00;

    // ---------------------------------------------------- //
    // ENTRADA DEL SWITCH GENERAL QUE DA PASO A ESTA PRUEBA //
    // ---------------------------------------------------- //
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

    measure_count();                          // Establish baseline capacitance
    base_cnt = meas_cnt;

    for (i = AVERAGE_TIMES; i > 0; i--)       // Repeat and avg base measurement
    {
        measure_count();
        base_cnt = (meas_cnt + base_cnt) >> 1;
    }

    /* Main loop starts here*/
    while (1)
    {
        if (ENABLE)
        {

            key_pressed = 0;                        // SUPONEMOS QUE NADA TOCA EL SENSOR

            measure_count();                        // HACEMOS UNA PRIMERA MEDIDA

            delta_cnt = base_cnt - meas_cnt;        // CALCULA LA DIFERENCIA ENTRE EL DATO MEDIDO ALMACENADO EN MEAS_CNT Y EL BASE ESPECIFICADO

            /* RECALCULAMOS LA BASE ESTABLECIDA EN CASO DE DISMINUIR LA CAPACIDAD POR ALGÚN CASUAL*/

            if (delta_cnt < 0)                      // SI LA DIFERENCIA CON LA MEDIA ANTERIOR ES NEGATIVA, HAY QUE RECTIFICARLA
            {                                       // beyond baseline, i.e. cap dec
                base_cnt = (base_cnt + meas_cnt) >> 1; // HACEMOS UNA NUEVA MEDIA USANDO EL DESPLAZAMIENTO <<1 PARA DIVIDIR ENTRE DOS
                delta_cnt = 0;                    // DEVOLVEMOS 0 A LA DIFERENCIA PARA QUE NO SE TENGA EN CUENTA LA MEDIDA EN LOS PRÓXIMOS BUCLES
            }

            // LED2 INDICA EL LÍMITE DEFINIDO DE CAPACIDAD
            if (delta_cnt > PROXIMITY_LVL)  //LA DIFERENCIA SUPERA AL LÍMITE MARCADO
            {
                P1OUT |= LED_2;
                P2OUT |= SALIDA;    // NADA MÁS TOCAR EL OCHO SE ACTIVA LA SIGUIENTE PRUEBA
                BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
                cycles = FAST_SCAN_TIMEOUT; //PONEMOS ESTA VARIABLE AL VALLOR PARA DAR UN MARGEN ANTES DEL APAGADO DEL LED EN CASO DE DISMINUIR LA CAPACIDAD
                for (i = PROXIMITY_LVL; i < 150; i++) // i = 10 to strim out delta_cnt noise
                {

                    if (i < delta_cnt)
                        P1OUT |= LED_1;
                    else
                        P1OUT &= ~LED_1;
                    //_delay_cycles(15);
                }
            }
            else    // EN CASO DE QUE NO LLEGUEMOS AL LÍMITE, SI HEMOS ESTADO ENCENDIDOS ESPERAMOS A QUE NUESTRO CONTADOR LLEGUE A CERO ANTES DE APAGAR
            {
                cycles--;
                if (cycles > 0)
                {
                    P1OUT |= LED_2;     //MANTIENE ENCENDIDO LED 2
                    P2OUT |= SALIDA;    // NADA MÁS TOCAR EL OCHO SE ACTIVA LA SIGUIENTE PRUEBA
                    BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_0; // ACLK/(0:1,1:2,2:4,3:8)
                }
                else
                {
                    P1OUT &= ~LED_2;//SE APAGA LED 2
                    BCSCTL1 = (BCSCTL1 & 0x0CF) + DIVA_3; // ACLK/(0:1,1:2,2:4,3:8)
                    cycles = 0;
                }
            }
        }
    }
}

//RUTINA DE MEDIDA DE CAPACITANCIA

void measure_count(void)
{

    TA0CTL = TASSEL_3 + MC_2;                   // TACLK, cont mode
    TA0CCTL1 = CM_3 + CCIS_2 + CAP;             // Pos&Neg,GND,Cap

    /*CONFIGURACION DE LOS PUERTOS*/
    /*P2SEL2 PERMITE AL TIMER RECIBIR UNA SEÑAL DE RELOJ EXTERNA DE LOS PUERTOS*/
    SENSOR_DIR &= ~ SENSOR_PIN; // P2.2 ENTRADA DEL SENSOR0
    SENSOR_SEL &= ~ SENSOR_PIN; // PxSEL.y = 0 & PxSEL2.y = 1
    SENSOR_SEL2 |= SENSOR_PIN;

    /*ACTIVACION DEL WATCHDOG COMO TIMER*/
    WDTCTL = WDT_meas_setting; // WDT, SMCLK, interval timer - SCMK/512 = 1MHz/512 = 512us interval
    TA0CTL |= TACLR;                            // Clear Timer_A TAR
    __bis_SR_register(LPM0_bits + GIE);         // ESPERAMOS AL WATCHDOG TIMER MIENTRAS SE REALIZA LA MEDIDA
    TA0CCTL1 ^= CCIS0; // CAPTURAMOS LA MEDIDA DEL REGISTRO
    meas_cnt = TACCR1;                      // INTRODUCIMOS LA MEDIDA EN LA VARIABLE GLOBAL
    WDTCTL = WDTPW + WDTHOLD;               // Stop watchdog timer
    SENSOR_SEL2 &= ~SENSOR_PIN; //DESACTIVAMOS LA ENTRADA PAARA TERMINAR

    TA0CTL = 0;                                 // DESACTIVAMOS EL TIMER A0
}



/* Watchdog Timer interrupt service routine*/
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    TA0CCTL1 ^= CCIS0;                        // Create SW capture of CCR1
    __bic_SR_register_on_exit(LPM0_bits);     // Exit LPM0 on reti
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
