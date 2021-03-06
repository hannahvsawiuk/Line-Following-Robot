//ELEC 291 Project 2
//Receiver/Car controller
//Hannah Sawiuk, Adrian Viquez, Jake Osborne, Sajan Rajdev, Gregor Morrison

//********************************//
//	   	Included Libraries		  //	
//********************************//
#include <C8051F38x.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//********************************//
//	   	Defined Constants		  //	
//********************************//
#define SYSCLK    48000000L // SYSCLK frequency in Hz
#define BAUDRATE  115200L   // Baud rate of UART in bps

//	ADC Pins	// 
#define backright_ADC 	LQFP32_MUX_P1_5
#define backleft_ADC 	LQFP32_MUX_P1_6
#define right_ADC 		LQFP32_MUX_P1_7
#define left_ADC    	LQFP32_MUX_P2_0
#define center_ADC  	LQFP32_MUX_P2_1
#define comm_ADC		LQFP32_MUX_P1_4

//	General Constants	//
#define pwm_constant	50 //Base PWM value
#define rotate_constant 45 //Rotate PWM value
#define Ithresh_voltage 1 //Approaching intersection voltage value
#define sensitivity 0.1 //maintain_d voltage sensitivity factor

//	MOSFET Pins	//
#define OUT_L1 P2_2
#define OUT_L0 P2_3
#define OUT_R1 P2_5
#define OUT_R0 P2_4
#define Receive P1_1

//********************************//
//	   Function Prototypes	      //	
//********************************//
void maintain_d (void);
void stop (void);
void forward (void);
void backward (void);
void rotate(void);
void turn_left(void);
void turn_right(void);
void bin2dec (void);
//********************************//
//	   	Volatile Variables		  //	note: type unsigned char or unsigned int only
//********************************//
		//	PWM	//
volatile unsigned char pwm_count = 0;
volatile unsigned char pwmr = 0;
volatile unsigned char pwml = 0;
volatile unsigned char direction = 1; //1=forward, 0 = backwards 
volatile unsigned char overflow_count = 0;
volatile unsigned char I_flag = 0; //1 = intersection detected, 0 = no intersection detected
volatile unsigned char pulse_count = 0;
volatile unsigned char decode_flag = 0;

		///	ADC	//
volatile float V_ADC[6];
volatile float dright = 0.0;
volatile float dleft = 0.0;

	//	Instructions //
volatile unsigned char action = 1;//stop
volatile unsigned char Instructions[3];

//********************************//
//	   	Main Initializations	  //	
//********************************//
char _c51_external_startup (void)
{
	PCA0MD&=(~0x40);    // DISABLE WDT: clear Watchdog Enable bit
	VDM0CN=0x80; // enable VDD monitor
	RSTSRC=0x02|0x04; // Enable reset on missing clock detector and VDD

	// CLKSEL&=0b_1111_1000; // Not needed because CLKSEL==0 after reset
	#if (SYSCLK == 12000000L)
		//CLKSEL|=0b_0000_0000;  // SYSCLK derived from the Internal High-Frequency Oscillator / 4 
	#elif (SYSCLK == 24000000L)
		CLKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
	#elif (SYSCLK == 48000000L)
		CLKSEL|=0b_0000_0011; // SYSCLK derived from the Internal High-Frequency Oscillator / 1.
	#else
		#error SYSCLK must be either 12000000L, 24000000L, or 48000000L
	#endif
	OSCICN |= 0x03; // Configure internal oscillator for its maximum frequency

	// Configure UART0
	SCON0 = 0x10;

#if (SYSCLK/BAUDRATE/2L/256L < 1)
	TH1 = 0x10000-((SYSCLK/BAUDRATE)/2L);
	CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
	CKCON |=  0x08;
#elif (SYSCLK/BAUDRATE/2L/256L < 4)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/4L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 01                  
	CKCON |=  0x01;
#elif (SYSCLK/BAUDRATE/2L/256L < 12)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/12L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 00
#else
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2/48);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 10
	CKCON |=  0x02;
#endif
	//	Timer1 for Communication	//
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit autoreload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
	
	// Configure the pins used for square output
	P2MDOUT|=0b_0000_1100;
	P0MDOUT |= 0x10; // Enable UTX as push-pull output
	XBR0     = 0x01; // Enable UART on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0x40; // Enable crossbar and weak pull-ups

	// Initialize timer 2 for periodic interrupt//
	TMR2CN=0x00;   // Stop Timer2; Clear TF2;
	CKCON|=0b_0001_0000;
	TMR2RL=(-(SYSCLK/(2*48))/(100L)); // Initialize reload value
	TMR2=0xffff;   // Set to reload immediately
	ET2=1;         // Enable Timer2 interrupts
	TR2=1;         // Start Timer2

	//External INT1 and INT0 interrupt configuration
	IT01CF|=0b_0001_1000; //(P0_1 for INT1 and P0_0 for INT0)->
	TCON&=0b_1111_1101;
	IT0 = 1;
	EX0 = 1;

	// Enable interrupts
	EA=1;
	
	return 0;
}
//********************************//
//	   Timer 3 Configuration      //	
//********************************//
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON:
	CKCON|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN & 0x80));  // Wait for overflow
		TMR3CN &= ~(0x80);         // Clear overflow indicator
		// Check overflow of Timer/Counter 0
		if (TF0==1)
		{
			TF0=0;
			overflow_count++;
		}
	}
	TMR3CN = 0 ;                   // Stop Timer3 and clear overflow flag
}
//********************************//
//	 	  WaitMilliSeconds	      //	
//********************************//
void waitms (unsigned int ms)
{
	unsigned int j;
	for(j=ms; j!=0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}
//********************************//
//	 External interrupt (INT0)	  //	
//********************************//
void External_Interrupt (void) interrupt 0 //P0_1 as trigger
{
	//MSB first
	if((pulse_count+1)==1)
	{
		waitms(30);
		pulse_count++;
		//This is the trigger
	}
	else if((pulse_count+1 > 1) && (pulse_count <=3))
	{
		decode_flag = 0; //don't decode instruction yet
		waitms(80);
		Instructions[pulse_count-1] = Receive;
		pulse_count++;	
	}	
	else
	{	
		waitms(80);
		pulse_count = 0;
		decode_flag = 1; //decode and read instruction
	}
}
//********************************//
//	 	  Timer2 interrupt	  	  //	
//********************************//
void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count++;
	if(pwm_count > 100) 
		pwm_count = 0;

	if(direction == 1)//forward
	{
		OUT_R0 = 0;
		OUT_R1 = pwm_count > (100-pwmr)?1:0;

		OUT_L0 = 0;
		OUT_L1 = pwm_count > (100-pwml)?1:0;
	}
	else if(direction == 0)//backward
	{		
		OUT_R0 = pwm_count > (100-pwmr)?1:0;
		OUT_R1 = 0;

		OUT_L0 = pwm_count > (100-pwml)?1:0;
		OUT_L1 = 0;
	}
	else if(direction == 2) 	//Left wheel forward, right wheel backwards
	{
		OUT_R0 = pwm_count > (100-pwmr)?1:0;
		OUT_R1 = 0;
		
		OUT_L0 = 0;
		OUT_L1 = pwm_count > (100-pwml)?1:0;
	}
	
	else 	//Right wheel forward, left wheel backwards
	{
		OUT_R0 = 0;
		OUT_R1 = pwm_count > (100-pwmr)?1:0;
		
		OUT_L0 = pwm_count > (100-pwml)?1:0;
		OUT_L1 = 0;
	}	
}
//********************************//
//	 	 Initialize ADC   		  //	
//********************************//
void InitADC (void)
{
	// Init ADC
	ADC0CF = 0xF8; // SAR clock = 31, Right-justified result
	ADC0CN = 0b_1000_0000; // AD0EN=1, AD0TM=0
  	REF0CN = 0b_0000_1000; //Select VDD as the voltage reference for the converter
}
//********************************//
//		 Initialize ADC Pins	  //	
//********************************//
void InitPinADC (unsigned char portno, unsigned char pinno)
{
	unsigned char mask;
	
	mask=1<<pinno;
	
	switch (portno)
	{
		case 0:
			P0MDIN &= (~mask); // Set pin as analog input
			P0SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 1:
			P1MDIN &= (~mask); // Set pin as analog input
			P1SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 2:
			P2MDIN &= (~mask); // Set pin as analog input
			P2SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 3:
			P3MDIN &= (~mask); // Set pin as analog input
			P3SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		default:
		break;
	}
}
//********************************//
//	 	 ADC Value at Pin  		  //	
//********************************//
unsigned int ADC_at_Pin(unsigned char pin)
{
	AMX0P = pin;             // Select positive input from pin
	AMX0N = LQFP32_MUX_GND;  // GND is negative input (Single-ended Mode)
	// Dummy conversion first to select new pin
	AD0BUSY=1;
	while (AD0BUSY); // Wait for dummy conversion to finish
	// Convert voltage at the pin
	AD0BUSY = 1;
	while (AD0BUSY); // Wait for conversion to complete
	return (ADC0L+(ADC0H*0x100));
}
//********************************//
//	 	 Voltage at Pin  		  //	
//********************************//
float Volts_at_Pin(unsigned char pin)
{
	 return ((ADC_at_Pin(pin)*3.30)/1024.0);
}
//********************************//
//	 	    Main Program   		  //	
//********************************//
void main (void)
{	
	int i = 0;
	int j = 0;
	InitPinADC(1,4); // Backright Inductor
	InitPinADC(1,5); // Backright Inductor
	InitPinADC(1,6); // Backleft inductor
	InitPinADC(1,7); // Right inductor
	InitPinADC(2,0); // Left Inductor
	InitPinADC(2,1); // Center Inductor
	InitADC(); // Initialize the ADC
	for(i = 0; i < 3; i++) //initialize instructions and voltage arrays
		Instructions[i]=0;
	for(j = 0; j < 5; j++)
		V_ADC[i]=0;
	pulse_count = 0;
	stop();
	
	//main system run
	while(1)
	{
		V_ADC[0]=Volts_at_Pin(right_ADC); //Right Inductor
		V_ADC[1]=Volts_at_Pin(left_ADC); //Left Inductor
		V_ADC[2]=Volts_at_Pin(center_ADC); //Center Inductor
		V_ADC[3]=Volts_at_Pin(backright_ADC); //Backright Inductor
		V_ADC[4]=Volts_at_Pin(backleft_ADC); //Backleft Inductor
		V_ADC[5]=Volts_at_Pin(comm_ADC); //Backleft Inductor
		
		if(decode_flag == 1) //if new instructions, decode and store into action
			bin2dec();

		if (direction == 1 || direction == 0) //only maintain distance when going forward or backward
			maintain_d();
			
		if(V_ADC[2] > Ithresh_voltage)
			I_flag = 1;        
		else 
			I_flag = 0;

		if( V_ADC[5] < sensitivity) //if the car is off track, stop
			stop();
		else
		{
			if(action == 1)
			{
				stop();
			}
			else if(action == 2)
			{
				if(I_flag == 1)
					turn_right();
				else
				forward();
			}
			else if(action == 3)
			{	
				if(I_flag == 1)
					turn_left();
				else
					forward();
			}
			else if(action == 4)
			{
				forward();
			}
			else if(action == 5)
			{
				backward();
			}
			else if(action == 6)
			{
				rotate();
			}
			else 
			{
				stop();
			}
		}
	}
}
//********************************//
//	 	Maintain Distance     	  //	
//********************************//
void maintain_d (void)
{
	if (direction == 1) //forwards
	{
		dright = V_ADC[0]; //right 
		dleft = V_ADC[1]; //left
		
		if (fabsf(dright-dleft) > sensitivity)//Constant adjusts sensitivity
		{
			if(dright > dleft)
			{
				pwmr = (unsigned char)(pwm_constant*fabsf(dleft/dright));
				pwml = pwm_constant;
			}
			else
			{
				pwml = (unsigned char)(pwm_constant*fabsf(dright/dleft));
				pwmr = pwm_constant;
			}
		}
		else
		{
			pwmr = pwm_constant;
			pwml = pwm_constant;
		}	
	}
	else if (direction == 0)
	{
		dright = V_ADC[3]; //backright
		dleft = V_ADC[4]; //backleft
		if (fabsf(dright-dleft) > sensitivity)//Constant adjusts sensitivity
		{
			if(dright > dleft)
			{
				pwmr = (unsigned char)(pwm_constant*fabsf(dleft/dright));
				pwml = pwm_constant;
			}
			else
			{
				pwml = (unsigned char)(pwm_constant*fabsf(dright/dleft));
				pwmr = pwm_constant;
			}
		}
		else
		{
			pwmr = pwm_constant;
			pwml = pwm_constant;
		}
	}
}
//********************************//
//	 		  Bin2Dec   	 	  //	
//********************************//
void bin2dec (void)
{
	action = (unsigned char)((Instructions[2] + 2*Instructions[1] + 4*Instructions[0])-1);
}
//********************************//
//	 	  	1:Stop    			  //	
//********************************//
void stop (void)
{
	pwml = 0;
	pwmr = 0;
}
//***************************************//
// 2:Turn right at the next intersection //	
//***************************************//
void turn_right(void)
{	
	direction = 3; //turn
	stop();
	waitms(100); //delay for short time
	pwmr = pwm_constant;
	pwml = 0;
	while(fabsf(V_ADC[0]-V_ADC[1]) > sensitivity) //keep turning while checking voltages until fairly centered
	{
		V_ADC[0]=Volts_at_Pin(right_ADC); //Right Inductor -> negative input of the peak detector
		V_ADC[1]=Volts_at_Pin(left_ADC); //Left Inductor
	}
	action = 4; //reset action
	forward(); //then follow the wire
}
//***************************************//
// 3:Turn left at the next intersection  //	
//***************************************//
void turn_left(void)
{	
	direction = 2;
	stop();
	waitms(100);
	pwml = pwm_constant;
	pwmr = 0;
	while(fabsf(V_ADC[0]-V_ADC[1]) > sensitivity)
	{
		V_ADC[0]=Volts_at_Pin(right_ADC); //Right Inductor -> negative input of the peak detector
		V_ADC[1]=Volts_at_Pin(left_ADC); //Left Inductor
	}
	action = 4;
	forward(); //go forward after
}
//********************************//
//	 	  4:Forward       		  //	
//********************************//
void forward(void)
{	
	direction = 1;//forward
	pwml = pwm_constant;
	pwmr = pwm_constant;
	maintain_d();
}
//********************************//
//	 	  5:Backward              //	
//********************************//
void backward(void)
{	
	direction = 0;//backward
	pwml = pwm_constant;
	pwmr = pwm_constant;
	maintain_d();
}
//********************************//
//	 	  6:Rotate        		  //	
//********************************//
void rotate(void)
{
	direction = 2;
	pwml = rotate_constant; 
	pwmr = rotate_constant;
	waitms(500);
	while(fabsf(V_ADC[0]-V_ADC[1]) > sensitivity)
	{
		V_ADC[0]=Volts_at_Pin(right_ADC); //Right Inductor -> negative input of the peak detector
		V_ADC[1]=Volts_at_Pin(left_ADC); //Left Inductor
	}
	stop();
	action = 4;
	forward(); //go forward after
}

