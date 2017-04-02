/*This file contains the User Interface implementations for the Magnetic
  Field Track Robot controller*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>


unsigned volatile int cnt = 0;
//unsigned volatile int pwm = 2;

unsigned volatile int ModifyTimerLow = 19972; //2013 for 10khz, 19972 for 15khz
unsigned volatile int ModifyTimerHigh = 65534;
volatile int interrupt_flag = 0;


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
	
	if(!interrupt_flag)
	{
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

	while((*temp = getByte()) != 'X')
	{
		++temp;
	}

	return rxstr;
}

int main (void)
{

	char* ps;
	// Set PORTB 0 pin as output, turn it off
	DDRB = 0x01;
	PORTB = 0x00;

	//Turns on timer and sets it to interrupt on overflow
	TCCR1B |= _BV(CS10); //Can set different arrangements of bits for prescalers [see pg 173 of datasheet]
	TIMSK1 |= _BV(TOIE1); //see pg 184 of datasheet, setting this bit enables timer1 to interrupt from overflow
	
	
	initUART();
	sei();
	

	while (1) {
		
		ps=readString();
		
		if(strcmp(ps,"0") == 1){ // 1. Rotate 180°
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(10);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
		else if(strcmp(ps,"1") == 1){ // 2. Turn right
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(20);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
		else if(strcmp(ps,"2") == 1){ // 3. Turn Left
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(30);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
		else if(strcmp(ps,"3") == 1){ // 4. Forward
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(40);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
		else if(strcmp(ps,"4") == 1){ // 5. Backward
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(50);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
		else if(strcmp(ps,"5") == 1){ // 6. Stop
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
			_delay_ms(60);
			interrupt_flag=1;
			_delay_ms(30);
			interrupt_flag=0;
		}
	}
	return 0;
}
