#include <stdint.h>

/* 
    VSS -> gnd
    VDD -> 5V
    V0  -> Pot
    Rs  -> PC5
    R/W -> gnd
    E   -> PC6
    DB0 -> not connected
    DB1 -> not connected
    DB2 -> not connected
    DB3 -> not connected
    DB4 -> PB4
    DB5 -> PB5
    DB6 -> PB6
    DB7 -> PB7
    A   -> 5V
    K   -> gnd
*/

#define EN                  0x40    // PA6
#define RS                  0x20    // PA5
#define clearDisplay        0x01    // delay 2ms


// Writes blank to all addresses and returns to address 0
#define returnHome          0x02   // delay 2ms

// Returns to address 0 and to original state
// Cursor goes to left edge of first line
#define displayOn           0x0A   // delay 37us

// Should be on to display the characters
#define cursorOn            0x0E   // delay 37us

// Displays cursor position
#define cursorBlink         0x0F   // delay 37us

// Blinks cursor or character where the cursor is
#define cursorOff           0xC     // delay 37us
#define IncrementCursor     0x10|0x04    // delay 37us
#define DecrementCursor     0x10|0x00    // delay 37us

// In 2-line display, it moves to second line when it passes 40
#define setTo4Bits          0x28   // delay 37us

// 4-bit length, 2 display lines, 5x8 font
#define entryMode           0x06   // delay 37us

// Increments DDRAM address by 1 when a character is written into it
#define firstLine           0x80   // delay 37us
#define SecondLine			0xC0   // delay 37us
#define LCD_SETDDRAMADDR    0x80   
#define ShiftDisplyRight    0x10|0x08|0x04  // delay 37us
#define ShiftDisplayLeft    0x10|0x08|0x00  // delay 37us

/*
// S/C = 1 -> display shift
// S/C = 0 -> cursor move
// R/L = 1 -> shift to right
// R/L = 0 -> shift to left
// DL  = 0 -> 4 bits
// N   = 1 -> 2 lines
// F   = 0 -> 5x8 dots
// I/D = 1 -> Increment
// S   = 0 -> no display shift
// D   = 1 -> Display is on
// C   = 1 -> Cursor displayed
// B   = 1 -> Cursor blinks
*/

// We set RS to 1 to write data


// Called before any LCD command to first initialize LCD
void LCD_init(void);
/*!
 * @fn      LCD_Write4bits
 * @param   data    data to be sent to LCD, can either be a command or a character.
 * @param   conrtol can be RS or 0 to make sure we're sending a command or data.
 * @brief   sent data is written to DRAM of LCD to either make a command or write data
 *          to screen.
*/
void LCD_Write4bits(char data, char control);
/*!
 * @fn      LCD_cmd
 * @param   command command to be given to LCD
 * @brief   takes a command and sends it to LCD
*/
void LCD_cmd(char command);
/*!
 * @fn      LCD_data
 * @param   data    input character
 * @brief   takes input character and displays it on LCD 
 *          and shifts cursor right
*/
void LCD_data(char data);
/*!
 * @fn      LCD_WriteString
 * @param   str string to be displayed on LCD
 * @brief   takes input string and displays it on LCD
*/
void LCD_WriteString(char* str);
/*!
 * @fn      gotoxy
 * @param   x   x position on the LCD (0 to 15)
 * @param   y   y position on the LCD (0 or 1)
 * @brief   gotoxy(x, y) moves cursor to specified location on LCD
*/
// Goto point (x,y) ==> first point at the first line(0,0)
// first point at the second line(0,1)
void gotoxy(uint32_t x, uint32_t y);
/*!
 * @fn      LCD_CountDown
 * @param   time takes a char array that is to be displayed on LCD
 * @brief   takes input in the form of "mm:ss" and counts it down on
 *          the LCD
*/
void LCD_CountDown(void);
void DELAY(const int ms);