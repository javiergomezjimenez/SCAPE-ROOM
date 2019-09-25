
                                                    // CONEXIONADO //
                                              // POTENCIÓMETRO -> PIN 1.0 // Verde
                                              // BUZZER ACTIVO -> PIN 2.2 //
                                            //LED JUGADOR 1 -> PIN 2.1//
                                            //LED JUGADOR 2 -> PIN 2.3//
                                            //LED JUGADOR 3 -> PIN 2.4//
                                            //LED JUGADOR 4 -> PIN 2.5//
                                            //PULSADOR JUGADOR 1 -> PIN 1.1//
                                            //PULSADOR JUGADOR 2 -> PIN 1.2//
                                            //PULSADOR JUGADOR 3 -> PIN 1.3//
                                            //PULSADOR JUGADOR 4 -> PIN 1.4//
                                            //SWITCH JUEGO FINAL -> PIN 2.6//
                                                  //SALIDA 1 -> PIN 1.6//
                                                  //SALIDA 2 -> PIN 1.7//
                                                  // FIN CONEXIONADO //


//#include <msp430.h>
#include  "msp430g2553.h"
#include <stdio.h>

#define BUZZER  BIT2
#define SALIDA1  BIT6
#define SALIDA2  BIT7
#define DENTRO  ((frec_min<frec)&&(frec<frec_max))

/*PULSADORES DEL JUEGO FINAL*/
#define J1  BIT1
#define J2  BIT2
#define J3  BIT3
#define J4  BIT4

/*LEDS PARA EL JUEGO FINAL*/
#define L1  BIT1
#define L2  BIT3
#define L3  BIT4
#define L4  BIT5

volatile unsigned long count=1001;          //CONTADOR PARA CORTAR LAS NOTAS EN LA DURACION DADA
volatile char FINAL = 0; //SEÑAL DE HABILITACIÓN DEL JUEGO FINAL
volatile int frec=0;
volatile char temp=0, temp_cero=0;
volatile int cont=0;//contador para el temporizador del juego final
volatile char nivel=1; // Nivel del juego final
volatile char aux=0; // Variable para ver si has pulsado los botones antes de que acabe el tiempo

const char ALARMA1[]={4,8,1,6};
const char ALARMA2[]={3,9,5,7};
const char ALARMA3[]={8,8,4,2};
const char ALARMA4[]={2,2,2,7};
const char ALARMA5[]={5,1,9,9};

//NIVELES DEL JUEGO FINAL
const char JUGADORES_NIVEL1[]={L1,L3,L2,L4};
const char JUGADORES_NIVEL2[]={L1,L3,L2,L4,L1};
const char JUGADORES_NIVEL3[]={L1,L3,L2,L4,L1,L3};
const char JUGADORES_NIVEL4[]={L1,L3,L2,L4,L1,L3,L2};

//MARCADOR DE MUERTE EN EL JUEGO FINAL
volatile int MUERTE=0;


void conf_reloj(void){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
        DCOCTL = CALDCO_1MHZ;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
} // REVISADO


void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;      //deshabilita ADC
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0|ADC10IE; //enciende ADC, S/H lento, REF:VCC, con INT
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    //Modo simple, reloj ADC, sin subdivision, Disparo soft, Canal 0
    ADC10AE0 = canales; //habilita los canales indicados
    ADC10CTL0 |= ENC; //Habilita el ADC
} // REVISADO

int lee_frec(void){
    ADC10CTL0 &= ~ENC;                  //deshabilita el ADC
    ADC10CTL1&=(0x0fff);                //Borra canal anterior
    ADC10CTL1|=BIT0;                    //selecciona nuevo canal
    ADC10CTL0|= ENC;                    //Habilita el ADC
    ADC10CTL0|=ADC10SC;                 //Empieza la conversión                        //Espera fin en modo LPM0
    return(ADC10MEM);                   //Devuelve valor leído
} // REVISADO


void playString(const char ALARMA[], const int frec_min, const int frec_max)
{
    unsigned int i, j;
    temp=1;
    count=801;
    while (temp && DENTRO)
    {
        P2OUT |= BUZZER; // SONIDO
    }
    for(i = 4; i > 0; i--)
    {
        temp_cero=1;
        count=2500;
        while (temp_cero && DENTRO)
        {
            P2OUT &= ~BUZZER; // SILENCIO
        }
        for (j=ALARMA[4-i]; j > 0; j--)
        {
            temp=1;
            count=2801;
            while (temp && DENTRO)
            {
                P2OUT |= BUZZER; // SONIDO
            }
            temp_cero=1;
            count=2851;
            while (temp_cero && DENTRO)
            {
                P2OUT &= ~BUZZER; // SILENCIO
            }
        }
        temp_cero=1;
        count=2500;
        while (temp_cero && DENTRO)
        {
            P2OUT &= ~BUZZER; // SILENCIO
        }
    }
}


void espera(int tiempo){
    cont=tiempo;
    aux=0;
    TA1CCTL0 =CCIE;//activamos las interrupciones del timer 1
    LPM0;
    TA1CCTL0 &=~CCIE;//DESactivamos las interrupciones del timer 1

}


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    // INICIO CONFIGURACIÓN RELOJ Y SUS INTERRUPCIONES //
    conf_reloj();               //reloj configurado a 1 Mhz
    // CONFIGURACIÓN DEL TIMER 0 PARA FRECUENCIA DE MUESTREO DEL POTENCIÓMETRO //
    TA0CTL = TASSEL_2 | ID_0 | MC_1;         //SMCLK, DIV=1 (1MHz), UP TO TA0CCR0
    TA0CCR0 = 999;       //periodo=1000: 1ms
    TA0CCTL0 = CCIE;      // CCIE=1 --> ACTIVO INTERRUPCIÓN

    // FIN CONFIGURACIÓN RELOJ Y SUS INTERRUPCIONES //


    // INICIO CONFIGURACIÓN DE LOS PINES //
    P2DIR &= ~BIT7; //ENTRADA DEL SWITCH GENERAL QUE DA PASO A LA SIGUIENTE PRUEBA

    P2DIR &= ~BIT6; //ENTRADA DEL SWITCH DEL JUEGO FINAL

    P2DIR |= BUZZER; // SALIDA EL PIN DEL BUZZER
    P2OUT = 0;

    P2SEL &= ~(BIT7|BUZZER|BIT6);
    P2SEL2 &= ~(BIT7|BUZZER|BIT6);

    P1DIR |= (SALIDA1|SALIDA2); // SEÑALA QUE LA PRIMERA PRUEBA DE LA RADIO HA CONCLUIDO
    P1OUT &= ~(SALIDA1|SALIDA2);
    P1SEL &= ~(SALIDA1|SALIDA2);
    P1SEL2 &= ~(SALIDA1|SALIDA2);



    // INICIO ENTRADAS BOTONES Y SALIDAS LEDS PARA EL JUEGO FINAL //
    P1SEL &= ~(J1|J2|J3|J4);    // PULSADORES
    P1DIR &= ~(J1 | J2 | J3 | J4);  //Activación de puertos de entrada para los pulsadores con pull up
    P1OUT |= J1 | J2 | J3 | J4;
    P1REN |= J1 + J2 + J3 + J4;
    P2SEL &= ~(L1|L2|L3|L4);    // LEDS
    P2DIR |= L1 | L2 | L3 | L4;     //Activación de las salidas de los leds inicialmente apagados
    P2OUT &=~(L1 | L2 | L3 | L4);
    // FIN ENTRADAS BOTONES Y SALIDAS LEDS PARA EL JUEGO FINAL //


    //CONFIGURACION DEL TIMER 1 A 12kHz PARA CONTAR LOS TIEMPOS DEL JUEGO
    TA1CTL = TASSEL_1 | ID_0 | MC_1;         //ACLK, DIV=1 (12KHz), UP TO TA1CCR0
    TA1CCR0 = 12;       //periodo=1000: 1ms
    TA1CCTL0 &=~CCIE;      // LA INTERRUPCIÓN ESTÁ INICIALMENTE DESACTIVADA
    // LA ACTIVAREMOS CUANDO LO USEMOS PARA CONTAR, PARA AHORRAR ENERGÍA



    P2IES |= BIT7; /* P2.7 act. flanco de subida --------->SWITCH GENERAL Y DE LA RADIO*/
    P2IE |= BIT7;     /*Activa interrupcion en P2.7*/

    P2IES |= BIT6; /* P2.6 act. flanco de subida---------->SWITCH DEL JUEGO FINAL */
    P2IE |= BIT6;     /*Activa interrupcion en P2.7*/

    P2IFG = 0;   /* Borra flags pendientes */
    // FIN CONFIGURACIÓN DE LOS PINES //


    // INICIO CONFIGURACIÓN ADC //
    inicia_ADC (BIT0);   //INICIA EL ADC EN EL PIN 0 DE SU PUERTO
    // FIN CONFIGURACIÓN ADC//


    __bis_SR_register(GIE); // ACTIVO LAS INTERRUPCIONES





    while(1){
        if (~FINAL)
        {
            if     (frec>100 && frec<200)   playString(ALARMA1, 100, 200);      //4816
            else if(frec>300 && frec<400)
            {
                playString(ALARMA2, 300, 400);         //3957  CORRECTO
                P1OUT |= SALIDA1;               // ACTIVA LA SIGUIENTE PRUEBA
            }
            else if(frec>500 && frec<600) playString(ALARMA3, 500, 600);        //8842
            else if(frec>700 && frec<800) playString(ALARMA4, 700, 800);        //2227
            else if(frec>900 && frec<1000) playString(ALARMA5, 900, 1000);       //5199
            else P2OUT &= ~BUZZER;                                    // SILENCIO

        }

        /*JUEGO FINAL CON ACTIVACION PRIORITARIA*/
        while (FINAL)
        {
            ADC10CTL0 &= ~ENC;      //Deshabilita ADC
            TA0CCTL0 &= ~CCIE;      // Deshabilita Interrupción del timer 0
            P1IFG = 0;//BORRAMOS LOS FLAGS ANTERIORES
            P1IES |= (J1 | J2 | J3 | J4);   /* P2.7 act. ---------> BOTONES */
            P1IE |= (J1 | J2 | J3 | J4);     /*Activa interrupcion en P2.7*/


            char j=0;

            switch(nivel){
            case 1:
                P2OUT=BUZZER;   //SUENA EL BUZZER PARA EL INICO DEL JUEGO
                espera(1000);
                P2OUT=0x0000;
                espera(1000);

                MUERTE=0;

                for(j=0;j<4;j++){
                    P2OUT|=JUGADORES_NIVEL1[j];
                    espera(1000);
                    P2OUT&=~(JUGADORES_NIVEL1[j]);//APAGAMOS EL LED QUE ESTUVIERA ENCENDIDO
                    if(MUERTE==1)
                    {
                        nivel=1;
                        j=8;
                    }
                    else nivel=2;
                }
                break;
            case 2:
                P2OUT=BUZZER;   //SUENA EL BUZZER 2 VECES(SEGUNDO NIVEL)
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(1000);

                MUERTE=0;

                for(j=0;j<5;j++){
                    P2OUT|=JUGADORES_NIVEL2[j];
                    espera(1000);
                    P2OUT&=~(JUGADORES_NIVEL2[j]);//APAGAMOS EL LED QUE ESTUVIERA ENCENDIDO
                    if(MUERTE==1)
                    {
                        nivel=1;
                        j=10;
                    }
                    else nivel=3;
                }
                break;
            case 3:
                P2OUT=BUZZER;   //SUENA EL BUZZER 3 VECES(TERCER NIVEL)
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(1000);

                MUERTE=0;

                for(j=0;j<6;j++){
                    P2OUT|=JUGADORES_NIVEL3[j];
                    espera(1000);
                    P2OUT&=~(JUGADORES_NIVEL3[j]);//APAGAMOS EL LED QUE ESTUVIERA ENCENDIDO
                    if(MUERTE==1)
                    {
                        nivel=1;
                        j=14;
                    }
                    else nivel=4;
                }
                break;
            case 4:
                P2OUT=BUZZER;   //SUENA EL BUZZER 4 VECES(CUARTO NIVEL)
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(100);
                P2OUT=BUZZER;
                espera(200);
                P2OUT=0x0000;
                espera(1000);

                MUERTE=0;

                for(j=0;j<7;j++){
                    P2OUT|=JUGADORES_NIVEL4[j];
                    espera(1000);
                    P2OUT&=~(JUGADORES_NIVEL4[j]);//APAGAMOS EL LED QUE ESTUVIERA ENCENDIDO
                    if(MUERTE==1)
                    {
                        nivel=1;
                        j=18;
                    }
                    else nivel=5;
                }
                break;
            case 5:                            //VICTORIA: LEDS ENCENDIDOS Y BUZZER PARPADEANDO EN CONTINUO
                P1IE = 0;       // APAGA INTERRUPCIONES DEL PUERTO 1
                while(1){
                    P2OUT |= (L1|L2|L3|L4);//encendemos todos los leds
                    P2OUT |= BUZZER;//buzzer parpadeante
                    espera(100);
                    P2OUT &= ~(L1|L2|L3|L4);//encendemos todos los leds
                    P2OUT=0x0000;
                    espera(100);
                    P1OUT |= SALIDA2;               // ACTIVA LA SIGUIENTE PRUEBA

                }
            }


        }

        LPM0;
    }
}


#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void)
{
    LPM0_EXIT;  //Despierta al micro al final de la conversión
}


#pragma vector=TIMER0_A0_VECTOR //CADA 1ms
__interrupt void Interrupcion_T00 (void)
{
    // POTENCIÓMETRO //
    frec = lee_frec(); // LEE CADA 1ms LA MEDIDA DEL POTENCIÓMETRO LLAMANDO AL ADC

    // DURACIÓN NOTAS DE LAS ALARMAS //
    count++;
    if (count==3000)
    {
        temp = 0;
        temp_cero = 0;
    }
}

//interrupcion del timer 1, solo para el juego final
#pragma vector=TIMER1_A0_VECTOR //CADA 1ms
__interrupt void Interrupcion_T10 (void)
{
    if(cont>0){
        cont--;
    }
    else {
        if(aux==0) MUERTE=1;
        LPM0_EXIT;
    }
}


#pragma vector=PORT2_VECTOR
__interrupt void RUTINA_INTERRUPCIÓN_SWITCH_ON(void)
{
    if(P2IN & BIT6)
    {
        FINAL = 1;
        P2IFG &= ~BIT6;      // Borra flag de interrupcion
        P2IE &= ~BIT6;       // APAGA INTERRUPCIONES DE FINAL
    }
    P2IE = 0;
}

//EL PUERTO 1 CONTIENE LOS BOTONES DEL JUEGO FINAL
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR_HOOK(void)
{


    if((~P1IN & J1) && ~MUERTE){
        cont=0;
        aux=1;
        if(P2OUT&L1){    //comprobamos que el led del jugador estaba encendido
            MUERTE=0;
        }
        else{   //SI EL JUGADOR PULSA CUANDO NO DEBÍA TAMBIÉN PIERDEN
            MUERTE=1;
        }
    }
    if((~P1IN & J2) && ~MUERTE){
        cont=0;
        aux=1;
        if(P2OUT&L2){//comprobamos que el led del jugador estaba encendido
            MUERTE=0;
        }
        else{//SI EL JUGADOR PULSA CUANDO NO DEBÍA TAMBIÉN PIERDEN
            MUERTE=1;
        }
    }
    if((~P1IN&J3) && ~MUERTE){
        cont=0;
        aux=1;
        if(P2OUT&L3){//comprobamos que el led del jugador estaba encendido
            MUERTE=0;
        }
        else{//SI EL JUGADOR PULSA CUANDO NO DEBÍA TAMBIÉN PIERDEN
            MUERTE=1;
        }
    }
    if((~P1IN & J4) && ~MUERTE){
        cont=0;
        aux=1;
        if(P2OUT&L4){//comprobamos que el led del jugador estaba encendido
            MUERTE=0;
        }
        else{//SI EL JUGADOR PULSA CUANDO NO DEBÍA TAMBIÉN PIERDEN
            MUERTE=1;
        }
    }
    P1IFG &= ~(J1|J2|J3|J4); // Borra flag
}
