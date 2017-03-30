/*This file contains the User Interface implementations for the Magnetic
  Field Track Robot controller*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
//#include <strlib.h>
#include "usart.h"

#define B0 0x01
#define B1 0x02
#define B2 0x04
#define B3 0x08
#define B4 0x10
#define B5 0x20
#define B6 0x40
#define B7 0x80

/////////////////////////////////
//                             //
//        UART Functions       //
//                             //
/////////////////////////////////

void initUART(void)
{
	// Not necessary; initialize anyway
	DDRD |= _BV(PD1);
	DDRD &= ~_BV(PD0);

	// Set baud rate; lower byte and top nibble
	UBRR0H = ((_UBRR) & 0xF00);
	UBRR0L = (uint8_t) ((_UBRR) & 0xFF);

	TX_START();
	RX_START();

	// Set frame format = 8-N-1
	UCSR0C = (_DATA << UCSZ00);
}

uint8_t getByte(void)
{
	// Check to see if something was received
	while (!(UCSR0A & _BV(RXC0)));
	return (uint8_t) UDR0;
}

void putByte(unsigned char data)
{
	// Stay here until data buffer is empty
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = (unsigned char) data;
}

void writeString(char *str)
{
	while (*str != '\0')
	{
		putByte(*str);
		++str;
	}
}

char * readString(void)
{
	static char rxstr[RX_BUFF];
	static char* temp;
	temp = rxstr;

	while((*temp = getByte()) != '\r')
	{
		++temp;
	}

	return rxstr;
}

int main (void)
{
	/* set PORTB for output*/
	//DDRB = 0xFF;
	
	char *ps;
	initUART();
	writeString("Robot Controller\r\n\n");

	while (1) {
		writeString("1.- Rotate 180 degrees\n\r");
		writeString("2.- Next intersection, turn right\n\r");
		writeString("3.- Next intersection, turn left \n\r");
		writeString("4.- Move forward \n\r");
		writeString("5.- Move backward \n\r");
		writeString("6.- Stop\n\r");
		writeString("Enter a command: ");
		ps=readString();
		
		writeString(ps);
		putByte('\r');
		putByte('\n');

		
		if(strcmp(ps,"0") == 1){
			writeString("\r\nRotate 180°\n\n\r");
			PORTB = B1;
		}
		else if(strcmp(ps,"1") == 1){
			writeString("\r\nNext intersection, turn right\n\n\r");
			PORTB = B2;
		}
		else if(strcmp(ps,"2") == 1){
			writeString("\r\nNext intersection, turn left\n\n\r");
			PORTB = B0;
		}
		else if(strcmp(ps,"3") == 1){
			writeString("\r\nMove forward\n\n\r");
			PORTB = B1;
		}
		else if(strcmp(ps,"4") == 1){
			writeString("\r\nMove backward\n\n\r");
			PORTB = B2;
		}
		else if(strcmp(ps,"5") == 1){
			writeString("\r\nStop\n\n\r");
			PORTB = B0;
		}
		else 
			writeString("\r\nWrong Input\n\n\r");
	}
	return 0;
}
