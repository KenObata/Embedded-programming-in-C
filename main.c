/*
 * main.c
 *
 * Created: 12/1/2019 12:19:22 PM
 * Author : Ken Obata
 */ 

#include <avr/io.h>


#include "CSC230.h"
#include <string.h>
#include <stdio.h>

//define ADC upper bounds
#define  ADC_BTN_RIGHT 0x032
#define  ADC_BTN_UP 0x0C3
#define  ADC_BTN_DOWN 0x17C
#define  ADC_BTN_LEFT 0x22B
#define  ADC_BTN_SELECT 0x316

//define timer setting for speed change
#define TIMER4_MAX_COUNT   0xFFFF
#define TIMER4_DELAY_SPEED1   977
#define TIMER4_DELAY_SPEED2   1954
#define TIMER4_DELAY_SPEED3   3907
#define TIMER4_DELAY_SPEED4   7813
#define TIMER4_DELAY_SPEED5   15625
#define TIMER4_DELAY_SPEED6   23438
#define TIMER4_DELAY_SPEED7   31250
#define TIMER4_DELAY_SPEED8   39063
#define TIMER4_DELAY_SPEED9   46875


	
/* Function declaration */
void button_flag_update(unsigned short );
void right_handler();
void up_handler();
void down_handler();
void left_handler();
void start_function();
void print_current();
void print_blink();
void collatz_seq();
void select_handler();

/*
Function poll_adc(): taken from lab9. 
Detects ADCL and ADCH
*/
//A short is 16 bits wide, so the entire ADC result can be stored
//in an unsigned short.
unsigned short poll_adc(){
	unsigned short adc_result = 0; //16 bits
	
	ADCSRA |= 0x40;
	while((ADCSRA & 0x40) == 0x40); //Busy-wait
	
	unsigned short result_low = ADCL;//this is 8 bit although assign short
	unsigned short result_high = ADCH;//this is 8 bit
	
	adc_result = (result_high<<8)|result_low; //shift ADCH to the left by 8 bit
	return adc_result;
}


//global variable 
volatile uint8_t column = 0;
volatile uint8_t position = 0;
volatile uint8_t collatz_flag_01 = 0;
volatile int button_flag = 0;
volatile int button_updated=0;
volatile char blink = ' ';
volatile char input_n[] = "000*0";
volatile char collatz_6_byte[] = "     0";
volatile int collatz_counter_int =0;
volatile char collatz_counter_string[] = "  0";
volatile long int collatz_int =0;
volatile int blink_flag = 0b00000001;
volatile char select_speed = '0';

int main(){
	
	//ADC Set up
	ADCSRA = 0x87;
	ADMUX = 0x40;
	
	//initialize lcd
	lcd_init();
	
	// Set up Timer 1 for button check
	TCCR1A = 0;
	TCCR1B = (1<<CS12)|(1<<CS10);	// Prescaler of 1024
	TCNT1 = 0xFFFF - 15626;			// Initial count (1 second)
	TIMSK1 = 1<<TOIE1;
	sei();
	// Set up Timer 3 for blink
	TCCR3A = 0;
	TCCR3B = (1<<CS32)|(1<<CS30);	// Prescaler of 1024
	TCNT3 = 0xFFFF - 7813;// Initial count (1 second)
	TIMSK3 = 1<<TOIE3;
	sei();
	// Set up Timer 4
	TCCR4A = 0;
	TCCR4B = (1<<CS42)|(1<<CS40);	// Prescaler of 1024
	TCNT4 = 0xFFFF - 7813;			// Initial count (0.5 second)
	TIMSK4 = 1<<TOIE4;
	sei();
	
	/* first output credit  */
	char credit1[]="Kenichi Obata";
	lcd_xy(0,0);
	lcd_puts(credit1);
	char credit2[]="CSC230-Fall2019";
	lcd_xy(0,1);
	lcd_puts(credit2);
	_delay_ms(1000);
	
	/* Second  print start screen*/
	char line1[]=" n=000*   SPD:0 ";
	lcd_xy(0,0);
	lcd_puts(line1);
	char line2[]="cnt:  0 v:     0";
	lcd_xy(0,1);
	lcd_puts(line2);
	
	/* check button and go to appropriate action*/
	while(1)
	{
		print_current();
		print_blink();
		_delay_ms(30);
		if(button_flag==1 && button_updated==1)
		{
			right_handler();
		}
		if(button_flag==2 && button_updated==1)
		{
			up_handler();
		}
		if(button_flag==3 && button_updated==1)
		{
			down_handler();
		}
		if(button_flag==4 && button_updated==1)
		{
			left_handler();
		}
		if(button_flag==5 && button_updated==1)
		{
			select_handler();
		}
		
		
	}

	return 0;
}


/*
Function button_flag_update:
This function updates button flag. This flag will be used later in main loop.
This function also avoids the situation when user holds button for a while.
*/
void button_flag_update(unsigned short v)
{
	if(v < ADC_BTN_RIGHT )
	{
		if(button_flag == 1)
		{
			button_updated=0;
			return;
		}
		else
		{
			button_flag=1;
			button_updated=1;
		}
	}
	else if(v < ADC_BTN_UP 	)
	{
		if(button_flag == 2)
		{
			button_updated=0;
			return;
		}
		else
		{
			button_flag=2;
			button_updated=1;
		}
	}
	else if(v < ADC_BTN_DOWN  )
	{
		if(button_flag == 3)
		{
			button_updated=0;
			return;
		}
		else
		{
			button_flag=3;
			button_updated=1;
		}
	}
	else if(v < ADC_BTN_LEFT)
	{
		if(button_flag == 4)
		{
			button_updated=0;
			return;
		}
		else
		{
			button_flag=4;
			button_updated=1;
		}
	}
	else if(v < ADC_BTN_SELECT)
	{
		if(button_flag == 5)
		{
			button_updated=0;
			return;
		}
		else
		{
			button_flag=5;
			button_updated=1;
		}
	}
	else
	{
		button_updated=0;
		button_flag=0;//no button is pressed.
	}
	
}

/* Interruption1: this interruption detects if any button is pressed.
   If any button is pressed, update button flag declared on top.

*/
ISR(TIMER1_OVF_vect) 
{
	// set the initial count
	TCNT1 = 0xFFFF - 625;//nterrupt 20 times per sec.

	// Update button flag
	unsigned short adc_result = poll_adc(); //check_button
	button_flag_update(adc_result);
	
}

/*
ISR3: Blinking cursor
*/
ISR(TIMER3_OVF_vect) 
{
	// Update a variable
	blink_flag = blink_flag^ 0b00000001;
	// reset the initial count
	TCNT3 = 0xFFFF - 7813;		
}

/*
ISR4: Update collatz sequence value and counter
*/
ISR(TIMER4_OVF_vect) 
{
	// Reset the initial count
	if(input_n[4]=='0')
	{
		//collatz_flag_01 = 0; //debug
		return;
	}
	if(input_n[4]=='1')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED1;
	}
	if(input_n[4]=='2')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED2;
	}
	if(input_n[4]=='3')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED3;
	}
	if(input_n[4]=='4')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED4;
	}
	if(input_n[4]=='5')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED5;
	}
	if(input_n[4]=='6')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED6;
	}
	if(input_n[4]=='7')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED7;
	}
	if(input_n[4]=='8')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED8;
	}
	if(input_n[4]=='9')
	{
		TCNT4 = TIMER4_MAX_COUNT - TIMER4_DELAY_SPEED9;
	}
	
	//stop counting if sequence is 000001
	if(collatz_int == 1)
	{
		collatz_flag_01 = 0;
		return;
	}
	//check if collatz flag is 1
	else if(collatz_flag_01 == 1)
	{
		collatz_seq();// Update a variable
	}
	
	
}

/* Function right_handler:
If right button is pressed, move cursor
*/
void right_handler()
{

	if(position==4)
	{
		return;
	}
	else
	{
		position++;
		
	}
}

/* Function up_handler:
If up button is pressed, update input number
If cursor is at start button and speed is not 0, then start counting collatz
*/
void up_handler()
{
	if(position==3)//if position is at '*'
	{
		if(input_n[4]=='0')
		{
			return; //if speed is 0, do nothing
		}
		else
		{
			//start  funtion
			start_function();
		}
		
	}
	else
	{
		if(input_n[position]=='9')
		{
			input_n[position]='0';
		}
		else
		{
			//update input
			input_n[position]= input_n[position] + 1 ;
			
		}
	}
}

/* Function down_handler:
If down button is pressed, update input number
If cursor is at start button and speed is not 0, then start counting collatz
*/
void down_handler()
{
	if(position==3)
	{
		//start  function
		start_function();
	}
	else
	{
		if(input_n[position]=='0')
		{
			input_n[position]='9';
		}
		else
		{
			input_n[position]= input_n[position] - 1;
		}
	}
}

/* Function left_handler:
If left button is pressed, move cursor left

*/
void left_handler()
{
	if(position==0)//if cursor is at the most left, do nothing
	{
		return;
	}
	else
	{
		position--; //move cursor left 
	}
}

/* Function select_handler:
If select button is pressed, remember current speed and swap previous speed.
*/
void select_handler()
{
	//copy the select speed to temp
	char temp_speed =select_speed ;
	//remember current speed
	select_speed=input_n[4];
	input_n[4] = temp_speed ;
}

/* Function start_function:
This function is called from up or down_handler only when start button is pressed.

*/
void start_function()
{
	//if speed is 0, stop
	if(input_n[4]=='0')
	{
		//collatz_flag_01 = 0; //debug
		return;
	}
	
	
	//reset counter to 0
	collatz_counter_int =0;
	snprintf(collatz_counter_string, 4, "%03d", collatz_counter_int);
	
	
	
	//get first 3 char from input_n
	char temp[4];
	strncpy(temp, input_n, 3);
	//convert temp into int
	sscanf(temp, "%ld",&collatz_int);
	
	//store into collatz_6_byte
	snprintf(collatz_6_byte, 7, "%06ld", collatz_int);
	
	//if input is 0, do nothing
	if(collatz_int==0)
	{
		return;
	}
	
	//collatz can start
	collatz_flag_01=1;
	
	return;
}

/* Function print_current:
This function is called from main function.
Simply print all current values of input, counter, and collatz value

*/
void print_current()
{

	//print 100 digit
	char temp_char = input_n[0];
	lcd_xy(3,0);
	lcd_putchar(temp_char);
	//print 10  digit
	temp_char = input_n[1];
	lcd_xy(4,0);
	lcd_putchar(temp_char);
	//print 1   digit
	temp_char = input_n[2];
	lcd_xy(5,0);
	lcd_putchar(temp_char);
	//print * button 
	temp_char = input_n[3];
	lcd_xy(6,0);
	lcd_putchar(temp_char);
	//print speed   
	temp_char = input_n[4];
	lcd_xy(14,0);
	lcd_putchar(temp_char);
	
	//print counter 100digit
	temp_char = collatz_counter_string[0];
	lcd_xy(4,1);
	lcd_putchar(temp_char);
	
	//print counter 10 digit
	temp_char = collatz_counter_string[1];
	lcd_xy(5,1);
	lcd_putchar(temp_char);
	
	//print counter 1  digit
	temp_char = collatz_counter_string[2];
	lcd_xy(6,1);
	lcd_putchar(temp_char);
	
	//print collatz 100000 digit
	temp_char =collatz_6_byte[0];
	lcd_xy(10,1);
	lcd_putchar(temp_char);
	
	//print collatz 10000  digit
	temp_char =collatz_6_byte[1];
	lcd_xy(11,1);
	lcd_putchar(temp_char);
	
	//print collatz 1000   digit
	temp_char =collatz_6_byte[2];
	lcd_xy(12,1);
	lcd_putchar(temp_char);
	
	//print collatz 100    digit
	temp_char =collatz_6_byte[3];
	lcd_xy(13,1);
	lcd_putchar(temp_char);
	
	//print collatz 10     digit
	temp_char =collatz_6_byte[4];
	lcd_xy(14,1);
	lcd_putchar(temp_char);
	
	//print collatz 1      digit
	temp_char =collatz_6_byte[5];
	lcd_xy(15,1);
	lcd_putchar(temp_char);
	
}

/* Function print_blink:
This function is called from main and prints blank char to current cursor.
Blink flag is updated via ISR3
*/
void print_blink()
{
	if (blink_flag ==0b00000001)
	{
		if(position==0)
		{
			lcd_xy(3,0);
			lcd_putchar(blink);
		}
		if(position==1)
		{
			lcd_xy(4,0);
			lcd_putchar(blink);
		}
		if(position==2)
		{
			lcd_xy(5,0);
			lcd_putchar(blink);
		}
		if(position==3)
		{
			lcd_xy(6,0);
			lcd_putchar(blink);
		}
		if(position==4)
		{
			lcd_xy(14,0);
			lcd_putchar(blink);
		}
		
	}
}

/*
when collatz flag is 1, continue updating collatz sequence
this is soly integer process
*/
void collatz_seq()
{
	collatz_counter_int++;
	//store into collatz_6_byte
	snprintf(collatz_counter_string, 4, "%03d", collatz_counter_int);
	
		if(collatz_int %2 ==0)
		{
			collatz_int = collatz_int /2;
		}
		else
		{
			collatz_int = 3*collatz_int +1;
		}
	
	//store into collatz_6_byte
	snprintf(collatz_6_byte, 7, "%06ld", collatz_int);
}
