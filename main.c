/******************************************************************************
 * Project 1: Wave Period Histogram
 *
 * Description:
 *
 * Displays the distribution of values that were received
 * as input into port T-pin 1.
 * 
 * Author:
 *  Amedeo Cristillo (ajc6432@rit.edu)
 *  Lennard Streat (lgs8331@rit.edu)
 *  Christoffer Rosen (cbr4830@rit.edu)
 *
 *****************************************************************************/


// system includes
#include <hidef.h>      /* common defines and macros */
#include <stdio.h>      /* Standard I/O Library */

// project includes
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */

// Definitions

// Change this value to change the frequency of the output compare signal.
// The value is in Hz.
#define OC_FREQ_HZ    ((UINT16)10)

// Macro definitions for determining the TC1 value for the desired frequency
// in Hz (OC_FREQ_HZ). The formula is:
//
// TC1_VAL = ((Bus Clock Frequency / Prescaler value) / 2) / Desired Freq in Hz
//
// Where:
//        Bus Clock Frequency     = 2 MHz
//        Prescaler Value         = 2 (Effectively giving us a 1 MHz timer)
//        2 --> Since we want to toggle the output at half of the period
//        Desired Frequency in Hz = The value you put in OC_FREQ_HZ
//
#define BUS_CLK_FREQ  ((UINT32) 2)   
#define PRESCALE      ((UINT16)  2)         
#define TC1_VAL       ((UINT16)  (((BUS_CLK_FREQ / PRESCALE) / 2) / OC_FREQ_HZ))

#define NUM_READINGS (1000)
#define BUCKET_MAX (1050)
#define BUCKET_MIN (950)
#define NUM_BUCKETS (BUCKET_MAX - BUCKET_MIN)
#define MAX_PAUSE_LINES (1)

UINT16 timeValues[ NUM_READINGS + 1 ];
UINT16 bucketCounts[ NUM_BUCKETS ];
UINT16 numRecorded;

//--------------------------------------------------------------       
// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------
void InitializeSerialPort(void)
{
    // Set baud rate to ~9600 (See above formula)
    SCI0BD = 13;          
    
    // 8N1 is default, so we don't have to touch SCI0CR1.
    // Enable the transmitter and receiver.
    SCI0CR2_TE = 1;
    SCI0CR2_RE = 1;
}


//--------------------------------------------------------------       
// Initializes I/O and timer settings for the demo.
//--------------------------------------------------------------       
void InitializeTimer(void)
{
  // Set the timer prescaler to %2, since the bus clock is at 2 MHz,
  // and we want the timer running at 1 MHz
  TSCR2_PR0 = 1;
  TSCR2_PR1 = 0;
  TSCR2_PR2 = 0;
    
  // Enable input mode on Channel 1
  TIOS_IOS1 = 0;
  TCTL4_EDG1A = 1;
  TCTL4_EDG1B = 0;
  
  // Set up output compare action to toggle Port T, bit 1
  //TCTL2_OM1 = 0;
  //TCTL2_OL1 = 1;
  
  // Set up timer compare value
  TC1 = TC1_VAL;
  
  // Clear the Output Compare Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
  
  // Enable the output compare interrupt on Channel 1;
  //TIE_C1I = 1;  
  
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
   
  //
  // Enable interrupts via macro provided by hidef.h
  //
  EnableInterrupts;
}

//--------------------------------------------------------------       
// Output Compare Channel 1 Interrupt Service Routine
// Refreshes TC1 and clears the interrupt flag.
//          
// The first CODE_SEG pragma is needed to ensure that the ISR
// is placed in non-banked memory. The following CODE_SEG
// pragma returns to the default scheme. This is neccessary
// when non-ISR code follows. 
//
// The TRAP_PROC tells the compiler to implement an
// interrupt funcion. Alternitively, one could use
// the __interrupt keyword instead.
// 
// The following line must be added to the Project.prm
// file in order for this ISR to be placed in the correct
// location:
//		VECTOR ADDRESS 0xFFEC OC1_isr 
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------       
void interrupt 9 OC1_isr( void )
{
  // Record time value of rising edge:
  timeValues[numRecorded++] = (UINT16)TCNT;
  
  // Interrupt handling overhead:
  TC1     +=  TC1_VAL;      
  TFLG1   =   TFLG1_C1F_MASK;
  
  if( numRecorded > NUM_READINGS ){
    TIE_C1I = 0;
  }
}
#pragma pop


//--------------------------------------------------------------       
// This function is called by printf in order to
// output data. Our implementation will use polled
// serial I/O on SCI0 to output the character.
//
// Remember to call InitializeSerialPort() before using printf!
//
// Parameters: character to output
//--------------------------------------------------------------       
void TERMIO_PutChar(INT8 ch)
{
    // Poll for the last transmit to be complete
    do
    {
      // Nothing  
    } while (SCI0SR1_TC == 0);
    
    // write the data to the output shift register
    SCI0DRL = ch;
}


//--------------------------------------------------------------       
// Polls for a character on the serial port.
//
// Returns: Received character
//--------------------------------------------------------------       
UINT8 GetChar(void)
{ 
  // Poll for data
  do
  {
    // Nothing
  } while(SCI0SR1_RDRF == 0);
   
  // Fetch and return data from SCI0
  return SCI0DRL;
}

//--------------------------------------------------------------       
// Ensures that the timer is functioning properly by
// ensuring that the timer is counting up at the start of
// the program.
//
// Returns: 1 if the post timer is functioning properly.
//--------------------------------------------------------------       
UINT8 postTimer(){
  int TC1BUF = TCNT;
  int i = 0;
  for( ; i< 1000; i++ ){
    // do nothing  
  }
  return (UINT8)(TC1BUF != TCNT); 

}

//--------------------------------------------------------------       
// Prints the beautiful header for the project..
//
//--------------------------------------------------------------       
void printProjectHeader( UINT8 projNum, char* date ){
  (void)printf( "|=========================================================|\n\r" );
  (void)printf( "| Authors:\tAmedeo Cristillo, Lennard Streat, Christoffer Rosen \n\r" );
  (void)printf( "| Project %u:\tWave Period Histogram\n\r",projNum );
  (void)printf( "| Date:\t\t%s\n\r",date );
  (void)printf( "| Description:\tDisplays the distribution of values that\n\r|\t\twere received as input into port T-pin 1.\n\r" );
  (void)printf( "|=========================================================|\n\r" );
}

//--------------------------------------------------------------       
// Entry point of our application code
//--------------------------------------------------------------       
void main(void)
{
  // User input for program:
  UINT8 userInput;
  UINT16 measInRange;
  UINT16 minValue = 99999;
  UINT16 maxValue = 0;
  
  #if MAX_PAUSE_LINES != 0
    UINT8 pCounter;
  #endif
  
  // Initialize Peripherals:
  InitializeSerialPort();
  InitializeTimer();
  
  // Exuecute post functions:
  if( postTimer() ){
  
    printProjectHeader(1,"9/11/2013");
    
    for(;;){
      // Prompt user to start measurements:
      (void)printf( "Press any key to begin measurement> " );
      userInput = GetChar();
      (void)printf( "\n\r", userInput );
    
      // Enable the output compare interrupt on Channel 1:
      (void)printf( "Reading Values...\n\r" );
      TIE_C1I = 1;
      numRecorded = 0;
      measInRange = 0;
      
      // Measure 1,000 interarrival times:
      while( numRecorded < NUM_READINGS ){
        // Do Nothing
      }
      
      // Pause for user input:
      (void)printf( "Press a key to continue> " );
      userInput = GetChar();
      (void)printf( "\n\r" );
      
      // Calculate time differentials:
      for( numRecorded = 0; numRecorded<NUM_READINGS; numRecorded++ ){
        timeValues[numRecorded] = timeValues[numRecorded+1] - timeValues[numRecorded];
        if( timeValues[numRecorded] > maxValue ){
          maxValue = timeValues[numRecorded];
        }
        
        if( timeValues[numRecorded] < minValue ){
          minValue = timeValues[numRecorded];
        }
        
      }
      
      // Calculate bucket counts:
      for( numRecorded = 0; numRecorded<NUM_READINGS; numRecorded++ ){
        bucketCounts[ timeValues[numRecorded] - BUCKET_MIN ] += 1; 
      }
      
      // Display shortest arrival time on one line & samples:
      (void)printf( "Inter-arrivial Times:\n\r" );
      for( numRecorded = 0; numRecorded<NUM_BUCKETS; numRecorded++ ){
        if( bucketCounts[numRecorded] != 0 ){
          (void)printf("\t%u us: %u\n\r", numRecorded+BUCKET_MIN, bucketCounts[numRecorded]);
          
          // Clear bucket, so that the value will refresh when program is re-executed.
          measInRange += bucketCounts[numRecorded];
          bucketCounts[numRecorded] = 0;
          
          // Pause Printing Mode:
          #if MAX_PAUSE_LINES != 0
            if(( pCounter == MAX_PAUSE_LINES )){
              userInput = GetChar();
              pCounter = 0;  
            }
            pCounter++;   
          #endif
        }
      }
      (void)printf( "Minimum value: %u\n\r", minValue );
      (void)printf( "Maximum value: %u\n\r", maxValue );
      (void)printf( "Total Inter-arrivals Times: %u\n\r", measInRange );
      
      // Reset the minimum and maximum values
      minValue = 99999;
      maxValue = 0;
   }
  }else{
    (void)printf( "Timer Failed to Initialize Properly.\n" );
  }
}// main()
