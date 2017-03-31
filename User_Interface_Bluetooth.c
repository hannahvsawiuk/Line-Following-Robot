/*This file contains the User Interface implementations for the Magnetic
  Field Track Robot controller*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include<avr/hc06.h>

unsigned volatile int cnt = 0;
//unsigned volatile int pwm = 2;

unsigned volatile int ModifyTimerLow = 19972; //2013 for 10khz, 19972 for 15khz
unsigned volatile int ModifyTimerHigh = 65534;


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
//        Interrupts           //
//                             //
/////////////////////////////////
ISR(TIMER1_OVF_vect)
{
	//These values can be modified by the user to change the interrupt rate of the timer, and thus the frequency [pg 176]
	//set between 0 and 65535 for different results
	TCNT1L = ModifyTimerLow;
	TCNT1H = ModifyTimerHigh;
	
	cnt++;
	if(cnt>1)
	{
		cnt=0;
	}
	if(cnt==1)
	{
		PORTB = 0b00000001; // Toggle pin 14 (PB0)
	}
	else
	{
		PORTB = 0b00000000; // Turn off PB0
	}
	
}


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
	// Set PORTB 0 pin as output, turn it off
	DDRB = 0x01;
	PORTB = 0x00;

	//Turns on timer and sets it to interrupt on overflow
	TCCR1B |= _BV(CS10); //Can set different arrangements of bits for prescalers [see pg 173 of datasheet]
	TIMSK1 |= _BV(TOIE1); //see pg 184 of datasheet, setting this bit enables timer1 to interrupt from overflow
	
	char *ps;
	initUART();
	sei();
	writeString("Robot Controller\r\n\n");

	while (1) {
		writeString("1.- Rotate 180 degrees\n\r");
		writeString("2.- Next intersection, turn right\n\r");
		writeString("3.- Next intersection, turn left \n\r");
		writeString("4.- Move forward \n\r");
		writeString("5.- Move backward \n\r");
		writeString("6.- Stop\n\r");
		writeString("Enter a command: ");
		ps=hc_06_bluetooth_receive_byte();
		
		writeString(ps);
		putByte('\r');
		putByte('\n');

		
		if(strcmp(ps,"0") == 1){
			writeString("\r\nRotate 180°\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(1000);
			sei();
			
			//PORTB = B1;
		}
		else if(strcmp(ps,"1") == 1){
			writeString("\r\nNext intersection, turn right\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(2000);
			sei();
		}
		else if(strcmp(ps,"2") == 1){
			writeString("\r\nNext intersection, turn left\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(3000);
			sei();
		}
		else if(strcmp(ps,"3") == 1){
			writeString("\r\nMove forward\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(4000);
			sei();
		}
		else if(strcmp(ps,"4") == 1){
			writeString("\r\
			nMove backward\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(5000);
			sei();
		}
		else if(strcmp(ps,"5") == 1){
			writeString("\r\nStop\n\n\r");
			cli();
			TCNT1L = ModifyTimerLow;
			TCNT1H = ModifyTimerHigh;
			_delay_ms(6000);
			sei();
		}
		else 
			writeString("\r\nWrong Input\n\n\r");
	}
	return 0;
}