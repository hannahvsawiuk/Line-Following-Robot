//Code adapted by Jake Osborne from Dr.Calvino-Fraga's base code

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

unsigned volatile int cnt = 0;
unsigned volatile int pwm = 50;

unsigned volatile int ModifyTimerLow = 65534; 
unsigned volatile int ModifyTimerHigh = 65534;



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

ISR(TIMER1_OVF_vect)
{
	//These values can be modified by the user to change the interrupt rate of the timer, and thus the frequency [pg 176]
	//set between 0 and 65535 for different results
	TCNT1L = ModifyTimerLow;
	TCNT1H = ModifyTimerHigh;
	
	cnt++;
	if(cnt>100)
	{
		cnt=0;
	}
	if(cnt>pwm)
	{
		PORTB = 0b00000001; // Toggle pin 14 (PB0)
	}
	else
	{
		PORTB = 0b00000000; // Turn off PB0
	}
	
}

int main(void)
{
	char *ps;
	unsigned int *intpointer;
	
	// Set PORTB 0 pin as output, turn it off
	DDRB = 0x01;
	PORTB = 0x00;

	//Turns on timer and sets it to interrupt on overflow
	TCCR1B |= _BV(CS10); //Can set different arrangements of bits for prescalers [see pg 173 of datasheet]
	TIMSK1 |= _BV(TOIE1); //see pg 184 of datasheet, setting this bit enables timer1 to interrupt from overflow
	
	initUART();
	sei(); // Turn interrupts on.

	while (1)
	{
		
		writeString("Enter Modifier (L for low bits, H for high):  \r\n");
		ps = readString();
		writeString("Entered: ");
		writeString(ps);
		putByte('\r');
		putByte('\n');
		
		if(ps == "l" || ps == 76)
		{
			writeString("Enter Low bits value (in decimal):  \r\n");
			intpointer = (int*)readString();	
			writeString("Entered: ");
			writeString(ps);
			putByte('\r');
			putByte('\n');
			ModifyTimerLow = &intpointer;
		}
		else if (&ps == "h" || &ps == "H")
		{
			writeString("Enter High bits value (in decimal):  \r\n");
			intpointer = (int*)readString();
			writeString("Entered: ");
			writeString(ps);
			putByte('\r');
			putByte('\n');
			ModifyTimerHigh = &intpointer;
		}
		else
		{
			writeString("Invalid modifier\n\r");
		}
	}
}