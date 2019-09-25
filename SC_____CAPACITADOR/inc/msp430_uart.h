/*
 * msp430_uart.h
 *
 *  Created on: Nov 23, 2014
 *      Author: VyLong
 */

#ifndef _MSP430_UART_H_
#define _MSP430_UART_H_

#include <msp430.h>
#include "TypeDefinition.h"

#define UART_PxSEL  P1SEL
#define UART_PxSEL2  P1SEL2

#define TXD_PIN     BIT1
#define RXD_PIN     BIT2

// SMCLK = 1MHz
//#define UART_PRESCALE_BR1_SMCLK_9600    0
//#define UART_PRESCALE_BR0_SMCLK_9600    0x68
//#define UART_PRESCALE_RS_SMCLK_9600     (UCBRS0)

// SMCLK = 8MHz
#define UART_PRESCALE_BR1_SMCLK_9600    0x03
#define UART_PRESCALE_BR0_SMCLK_9600    0x41
#define UART_PRESCALE_RS_SMCLK_9600     (UCBRS1)

// SMCLK = 16MHz
//#define UART_PRESCALE_BR1_SMCLK_9600  0x06
//#define UART_PRESCALE_BR0_SMCLK_9600  0x82
//#define UART_PRESCALE_RS_SMCLK_9600   (UCBRS2 | UCBRS1)

void UART_init(void);

inline void UART0putChar(char data);

void SerialprintUInt16(uint16_t values);
void SerialprintInt16(int16_t values);
void SerialprintFloat(float values);
void Serialprint(char *string);
void Serialprintln(char *string);

#endif /* MSP430_UART_H_ */
