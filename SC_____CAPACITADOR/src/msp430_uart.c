/*
 * msp430_uart.c
 *
 *  Created on: Nov 23, 2014
 *      Author: VyLong
 */

#include "inc\msp430_uart.h"

void UART_init(void)
{
    // Configure Pins
    UART_PxSEL = TXD_PIN | RXD_PIN;
    UART_PxSEL2 = TXD_PIN | RXD_PIN;

    // Configure UART
    UCA0CTL1 |= UCSSEL_2;   // SMCLK
    UCA0BR0 = UART_PRESCALE_BR0_SMCLK_9600;
    UCA0BR1 = UART_PRESCALE_BR1_SMCLK_9600;
    UCA0MCTL = UART_PRESCALE_RS_SMCLK_9600;      // Modulation UCBRSx
    UCA0CTL1 &= ~UCSWRST;   // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;        // Enable USCI_A0 RX interrupt
}

inline void UART0putChar(char data) {
    while (!(IFG2 & UCA0TXIFG));                // USCI_A0 TX buffer ready?
    UCA0TXBUF = data;
}

void SerialprintUInt16(uint16_t values)
{
    char stringOutput[6] = {0};

    int i;
    for(i = 4; i >= 0; i--)
    {
        stringOutput[i] = (values % 10) + '0';
        values /= 10;
    }
    stringOutput[5] = '\0';

    Serialprint(stringOutput);
}

void SerialprintInt16(int16_t values)
{
    int i;
    char valueSigned;
    int32_t data =  values;

    char stringOutput[7] = { ' ', ' ', ' ', ' ', ' ', '0', 0};

    if (data != 0)
    {
        if (data < 0)
        {
            valueSigned = '-';
            data *= -1;
        }
        else
            valueSigned = ' ';

        for(i = 5; i >= 0; i--)
        {
            if (data != 0)
            {
                stringOutput[i] = (data % 10) + '0';
                data /= 10;
            }
            else
            {
                stringOutput[i] = valueSigned;
                break;
            }
        }
    }

    Serialprint(stringOutput);
}

void SerialprintFloat(float values)
{

}

void Serialprint(char *string)
{
    int i = 0;
    while((i <= 255) && (*(string + i) != '\0')) {
        UART0putChar(string[i++]);
    }
}

void Serialprintln(char *string) {
    int i = 0;
    while((i <= 255) && (*(string + i) != '\0')) {
        UART0putChar(string[i++]);
    }
    UART0putChar('\n');
    UART0putChar('\r');
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
    if (UCA0RXBUF == '*') {
        Serialprintln("Detected [*]\n\r\0");
    } else {
        while (!(IFG2 & UCA0TXIFG));                // USCI_A0 TX buffer ready?
        UART0putChar(UCA0RXBUF);  // TX -> RXed character
    }
}

