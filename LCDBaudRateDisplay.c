#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */


/* variable type definitions */

   static char seven_seg[]= {0x3f, 0x06,0x5b,0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x80};
   //                         0     1    2    3     4     5     6     7    8     9      DP
   volatile static unsigned char seven_baud[4];
   volatile static unsigned char baud_sel;
   char BaudLabel[]="Baud = ";
   volatile static unsigned int falling_edge;
   volatile static unsigned int rising_edge;
   volatile static unsigned int total_time;
   static unsigned char first_line = 0x80; // 1|line|X|X|Char3|Char2|Char1|Char0| 
   static unsigned char second_line = 0xC0;
   volatile static signed int last_baud_rate = 0;



/* function prototypes */
  void LCD_init(void);
  void delay(int ms);       //delays for  X * 1.0ms
  void delay2(int ms);      // delays for X * 0.1ms
  void WriteInstrToLCD(char Instr);
  void WriteDataToLCD(char LCDdata);
  void LCDmsg(char *sptr);
  void ConfigureSCI(void);
  void TransmitChar(char TxD);
  char ReceiveChar(void);
  void InitPorts(void);
  void ConfigureInputCapture(void);
  void UpdateBaudRate(void);
  void DisplayTxBaudRate(void);
  void DisplayRxBaudRate(void);
  
  
void main(void) {
  /* put your own code here */
  
  InitPorts();
  ConfigureSCI();         //Use a default of 9600 Baud until told differently
  ConfigureInputCapture();
  LCD_init();
  LCDmsg(BaudLabel);
  EnableInterrupts; 
  TransmitChar(0x55);
  
  
  while(1){
    UpdateBaudRate();
    DisplayTxBaudRate();
    TransmitChar(0x55);
    DisplayRxBaudRate();   
  }
}

////////////////////////////////////////////////////////
/////////////// SUBROUTINES ////////////////////////////
////////////////////////////////////////////////////////

void InitPorts(){
  
// This subroutine initializes some of the Ports so that you are reading from 
// the switches connected to PortH, and whatever is connected to Port T.
// Initially the 7seg displays are all off.


		PTP = 0xFF;         // first make sure that PP0-3 are HIGH so all four seven segment displays will be Off
		DDRP = 0xFF;        // make PortP which controls the 7seg display an output

		
		DDRB = 0xFF;        // PortB is now an output (LEDs and 7Seg connected here)
		PORTB = 0;

		PTJ_PTJ1 = 1;       // Disable the LEDs (low will enable)
		DDRJ = 0b00000010;  // PortJ1 is now an output

		DDRH = 0;           // Make PortH an input so you can read switches
		DDRT = 0;           // Make PortT an input so the timer can watch Rx

}
		
  void ConfigureSCI(void){
    SCI1CR1 = 0;            //Full Duplex, 1 stop bit, No Parity
    SCI1CR2 = 0b00001100;   //Enable Tx and Rx but not Interrupts
    
    // Set BAUD to 9600
    if((CLKSEL & 0b10000000) == 0x80){
      SCI1BDL = 156;      // PLL so N = 156
      SCI1BDH = 0 ;
    }
    else{
      SCI1BDL = 26;       // Crystal so N = 26
      SCI1BDH = 0;
    }      
  }
  
  void ConfigureInputCapture(void){
   // This function will enable the Timer subsystem to use Input Capture
   // The TCNT will be run at 1.5Mhz in Debug mode
   // T0 will capture the TCNT value on a falling edge
   // T1 will capture the TCNT value on a rising edge
   // we will enable interrupts for PortT0 and PortT1




    TFLG1 = 0b00000011;   // Reset any Input Capture flags

    
  }
  
  void UpdateBaudRate(void){
    // This function will read the settings on PortH.  Based upon these, the
    // BAUD rate will be modified.  The assumtion is that we are in Debug mode
    //
    // XXXX1111	19200
    // XXXX0111	9600
    // XXXX0011	4800
    // XXXX0001	2400
    // XXXX0000	1200

    baud_sel = PTH & 0b00001111;
    
    if(baud_sel == 0x0F){
      SCI1BD = ;
    }else if(baud_sel == 0x07){
      SCI1BD = 156;
    }else if(baud_sel == 0x03){
      SCI1BD = ;
    }else if(baud_sel == 0x01){
      SCI1BD =  ;
    }else {
      SCI1BD = ;
    } 
  }

  void DisplayTxBaudRate(void){
    
    // This function will display the selected BAUD rate on the four 7-segment displays
    //
    // XXXX1111	19.2
    // XXXX0111	9600
    // XXXX0011	4800
    // XXXX0001	2400
    // XXXX0000	1200

    // Update the array of values to display on the four 7-segment displays
    baud_sel = PTH & 0b00001111;
    if(baud_sel == 0x0F){       //      seven_baud[]={1,9,10,2};
        seven_baud[0]=1;
        seven_baud[1]=9;
        seven_baud[2]=10;
        seven_baud[3]=2;
    }else if(baud_sel == 0x07){ //      seven_baud=[9,6,0,0];
        seven_baud[0]=9;
        seven_baud[1]=6;
        seven_baud[2]=0;
        seven_baud[3]=0;
    }
    else if(baud_sel == 0x03){ //      seven_baud[]={4,8,0,0};
        seven_baud[0]=4;
        seven_baud[1]=8;
        seven_baud[2]=0;
        seven_baud[3]=0;
    }else if(baud_sel == 0x01){ //      seven_baud[]={2,4,0,0};
        seven_baud[0]=2;
        seven_baud[1]=4;
        seven_baud[2]=0;
        seven_baud[3]=0;
    }else{                      //      seven_baud[]={1,2,0,0};
        seven_baud[0]=1;
        seven_baud[1]=2;
        seven_baud[2]=0;
        seven_baud[3]=0;
    }
      
    // Put the proper value on each of the four 7-segment displays and then leave.
    // Displays should be blank at the end of this function.

  
  }
    
  void DisplayRxBaudRate(void){
    
    // This function uses the total time of a single character to determine the BAUD rate
    // total_time is the number of clock cycles on TCNT.  We know that it is running at 1.5Mhz
    // So the BAUD rate is 1500000/total_time
    
    volatile unsigned int baud_rate;
    volatile unsigned char nxt_value;
    volatile unsigned int allowed_change;
    volatile signed int change;
   
    baud_rate = ;  //Use the number of counts on TCNT to calculate Baud rate
    
    // Writing to the LCD takes a long time so we only want to do it 
    // when things change
    change = baud_rate - last_baud_rate;
    if(change < 0){
      change = -change;
    }
    allowed_change = baud_rate/10;  // we don't bother displaying changes of less than 10%
    if(change > allowed_change){
      last_baud_rate = baud_rate;
    
      if(baud_rate > 0){
        WriteInstrToLCD(???);     // Need to define where on the LCD we are writing out value
        if(baud_rate > 9999){
          nxt_value = baud_rate/10000;
          WriteDataToLCD(nxt_value +0x30);
          baud_rate = baud_rate - (10000*nxt_value);
        } else{
          WriteDataToLCD(' ');
        }
      
        nxt_value = baud_rate/1000;
        WriteDataToLCD(nxt_value +0x30);
        baud_rate = baud_rate - (1000*nxt_value);
      
        // Need some more here, we are not done
      }
    }
    
  }
  
  void TransmitChar(char TxD){
    unsigned char flags;
    flags = SCI1SR1;
//    while((flags & 0b10000000)== 0){
//      flags=SCI1SR1;
//    }
//    SCI1DRL = TxD;
    if((flags & 0b10000000) != 0){
      SCI1DRL = TxD;
    }
    
  }
  
  char ReceiveChar(void){
    unsigned char flags;
    flags = SCI1SR1;
    while((flags & 0b00100000)== 0){
     flags = SCI1SR1;
    }
    return SCI1DRL;
     
  }


  void LCD_init(void){
    // This subroutine sends the commands to Reset and then configure the LCD

	    DDRK = DDRK | 0b00111111; // Make LCD pins output
	    PORTK_BIT1 = 0;
	  
      // Send the two LCD Reset Commands
      WriteInstrToLCD(0x33);      // Reset 1
      delay(5);
      WriteInstrToLCD(0x32);      // Reset 2
      delay(5);
      
      // Send the LCD configuration instruction sequence
      WriteInstrToLCD(0x2C); //4 bit, 2 line, ASCII characters
      WriteInstrToLCD(0x06); //cursor increment, disable display shift
      WriteInstrToLCD(0x0C); //display on, cursor off, no blinking
      WriteInstrToLCD(0x01); // clear display memory, set cursor to home pos
    	delay(2);           //Wait 2ms after configuration sequence
  }



  void WriteInstrToLCD(char Instr){
   // This function will write the 8-bit instruction passed as Instr to the LCD
   // -----------------------------------
   // ? |? | D4 | D3 | D2 | D1 | E | RS |
   // -----------------------------------
   volatile unsigned char temp;
   
   
   PORTK = PORTK & 0b11000000;  // Set 4bit Data pins, E and RS = 0 RS = 0 since this is an Instruction
   PORTK_BIT1 = 1;             // Set E = 1
   
   
   // Write the upper nibble of Instr to the LCD
   temp = Instr >> 2;       // move upper nibble to LCD data pins PORTK 5-2
   temp = temp &0b00111100;
   PORTK = PORTK | temp;
   delay(1);  // wait 1ms (only really need 50us)
   PORTK_BIT1 = 0;  // Set E = 0  this writes the nibble
   delay(5);
   PORTK_BIT1 = 1;  // Set E = 1
   
   // Write the lower nibble of Instr to the LCD   
   temp = Instr << 2;       // move lower nibble to LCD data pins PORTK 5-2
   temp = temp &0b00111100;
   PORTK = PORTK & 0b11000011; // clear data pins
   PORTK = PORTK | temp;     // set to lower nibble of Instr
   delay(1);  // wait 1ms (only really need 50us)
   PORTK_BIT1 = 0;  // Set E = 0  this writes the nibble
   delay(5);
     
  }
   

  
   void WriteDataToLCD(char LCDdata){
   // This function will write the 8-bit instruction passed as Instr to the LCD
    // -----------------------------------
   // ? |? | D4 | D3 | D2 | D1 | E | RS |
   // -----------------------------------
   volatile unsigned char temp;
   
   PORTK = PORTK & 0b11000001;  // Set 4bit Data pins and E = 0 
   PORTK_BIT0 = 1;             // RS = 1 since this is Data
   PORTK_BIT1 = 1;             // Set E = 1 
    
    // Write the upper nibble of LCDdata to the LCD
   temp = LCDdata >> 2;       // move upper nibble to LCD data pins PORTK 5-2
   temp = temp &0b00111100;
   PORTK = PORTK | temp;
   delay(1);                  // wait 1ms (only really need 50us)
   PORTK_BIT1 = 0;            // Set E = 0  this writes the nibble
   delay(5);
   PORTK_BIT1 = 1;            // Set E = 1
   
   // Write the lower nibble of LCDdata to the LCD   
   temp = LCDdata << 2;       // move lower nibble to LCD data pins PORTK 5-2
   temp = temp &0b00111100;
   PORTK = PORTK & 0b11000011;  // clear data pins
   PORTK = PORTK | temp;      // set to lower nibble of Instr
   delay(1);                  // wait 1ms (only really need 50us)
   PORTK_BIT1 = 0;            // Set E = 0  this writes the nibble
   delay(5);
      
  }
    
  void LCDmsg(char *sptr){  
   // sends a string to the LCD.  It expects a pointer to a string of ASCII characters (sptr)
   // it assumes that the string ends with 0x00 which is what you get if you enclose in "".
        while(*sptr){
          WriteDataToLCD(*sptr);
          ++sptr;
       }
  }
    


//delay function delays the code the number of milliseconds passed into it. 
void delay(int ms){
  volatile unsigned int x;
  while(ms>0){ 
    x = 1950;
    while(x>0){
      x--;
    }
    ms--;
  }                     
}    
//delay function delays the code the number of 0.1 milliseconds passed into it. 
void delay2(int ms){
  volatile unsigned int x;
  while(ms>0){ 
    x = 195;
    while(x>0){
      x--;
    }
    ms--;
  }                     
}    

////////////////////////////////////////////
//////// INTERRUPT HANDLERS ////////////////
////////////////////////////////////////////

/////////////////  Interrupt Handlers ///////////////////////////

  void interrupt 8 TimerChannel_0(void){
  // We should come here when PT0 has falling edge which is the start of a bit
  
    falling_edge = TC0;
    TFLG1 = 0b00000001;  //clear the interrupt flag
  }
  
  void interrupt 9 TimerChannel_1(void){
  // We should come here when PT1 has rising edge which is the end of a bit
    rising_edge = TC1;
    TFLG1 = 0b00000010;  //clear the interrupt flag
    
    total_time = rising_edge - falling_edge;  
  }
