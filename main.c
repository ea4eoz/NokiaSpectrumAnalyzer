// EA4EOZ's Handheld 2.4 GHz Spectrum Analyzer
//
// This code is based on ideas and functions taken from these places:
//
// DIY 2.4GHz Spectrum Analyser
// |
// |---> http://www.wireless.org.au/%7Ejhecker/specan/
//
//
// LCD Nokia 3310 (PCD8544) Driver in WinAVR(avr-gcc)
// |
// |---> http://fandigunawan.wordpress.com/2008/06/18/lcd-nokia-3310-pcd8544-driver-in-winavravr-gcc/
//
//
// scrying:module_spectrum
// |
// |---> http://scrying.org/doku.php?id=scrying:module_spectrum
// |
// |---> http://1010.co.uk/spectrumtest.c
//
//
// Atmel AVR code for using Cypress Wireless USB modules
// |
// |---> http://code.google.com/p/cywusb/
//
//
// Simple C code snippets for CYWM6935 RF module
// |
// |---> http://www.topix.com/forum/com/cy/TOGETOVMOQ6THO15B
//
// 
// I wish to thank you all for share your experiences with the rest of us.
//
//


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////
// Hardware
/////////////////////////////////////////////////////////////////

// SPI interface
#define SPI_DDR				DDRB	// Don't change
#define SPI_MOSI_PIN		PB3		// Don't change
#define SPI_CLK_PIN			PB5		// Don't change
#define SPI_SS_PIN			PB2		// Don't change

// Nokia 3410 LCD (PCD8544 compatible)
#define LCD_PORT			PORTB
#define LCD_DDR				DDRB
#define LCD_RST_PIN			PB0
#define LCD_DC_PIN			PB1
#define LCD_CE_PIN			PB2		// Already SS SPI pin
#define LCD_VISIBLE_X_RES	96
#define LCD_VISIBLE_Y_RES	64
#define LCD_REAL_X_RES		102
#define LCD_REAL_Y_RES		72
#define LCD_CACHE_SIZE		((LCD_VISIBLE_X_RES*LCD_VISIBLE_Y_RES)/8)
#define OK					0
#define OUT_OF_BORDER		1
typedef enum {LCD_CMD=0,LCD_DATA=1} LcdCmdData;
typedef enum {PIXEL_OFF=0,PIXEL_ON=1,PIXEL_XOR=2} LcdPixelMode;
unsigned char LcdCache[LCD_CACHE_SIZE];

// Radio module (CYWM6935)
#define RADIO_DDR		DDRC
#define RADIO_PORT		PORTC
#define RADIO_RST_PIN	PC1
#define RADIO_CE_PIN	PC0

// Keyboard
#define KBRD_DDR		DDRD
#define KBRD_PORT		PORTD
#define KBRD_PIN		PIND
#define LEFT_KEY		PD7
#define RIGHT_KEY		PD6



/////////////////////////////////////////////////////////////////
// Function prototypes
/////////////////////////////////////////////////////////////////

// SPI
void SPI_Init(void);
unsigned char SPI(unsigned char data);

// LCD
void LCD_Send(LcdCmdData cd, unsigned char data);
void LCD_Init(void);
void LCD_GotoXY(unsigned char x, unsigned char y);
void LCD_Chr (unsigned char ch);
void LCD_Str(unsigned char dataArray[]);
void LCD_FStr(const unsigned char *dataPtr);
unsigned char LCD_Pixel(unsigned char x, unsigned char y, LcdPixelMode mode );
void LCD_Line ( unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, LcdPixelMode mode);
void LCD_Update(void);


// Radio Module (CYWM6935)
void RADIO_Init(void);
unsigned char RADIO_Read(unsigned char address);
void RADIO_Write(unsigned char address, unsigned char value);
unsigned char RADIO_RSSI(unsigned char channel);

// Analog Digital Converter
void ADC_Init(void);
void ADC_Conversion(void);
unsigned int ADC_Read(void);

// Keyboard
void KBRD_Init(void);
unsigned char KBRD_L(void);
unsigned char KBRD_R(void);



/////////////////////////////////////////////////////////////////
// Misc.
/////////////////////////////////////////////////////////////////

// Radio Module Registers (CYWM6935)
#define REG_ID                0x00
#define REG_CONTROL           0x03
#define REG_DATA_RATE         0x04
#define REG_CONFIG            0x05
#define REG_SERDES_CTL        0x06
#define REG_RX_INT_EN         0x07
#define REG_RX_INT_STAT       0x08
#define REG_RX_DATA_A         0x09
#define REG_RX_VALID_A        0x0A
#define REG_RX_DATA_B         0x0B
#define REG_RX_VALID_B        0x0C
#define REG_TX_INT_EN         0x0D
#define REG_TX_INT_STAT       0x0E
#define REG_TX_DATA           0x0F
#define REG_TX_VALID          0x10
#define REG_THRESHOLD_L       0x19
#define REG_THRESHOLD_H       0x1A
#define REG_WAKE_EN           0x1C
#define REG_WAKE_STAT         0x1D
#define REG_ANALOG_CTL        0x20
#define REG_CHANNEL           0x21
#define REG_RSSI              0x22
#define REG_PA                0x23
#define REG_CRYSTAL_ADJ       0x24
#define REG_VCO_CAL           0x26
#define REG_PWR_CTL           0x2E
#define REG_CARRIER_DETECT    0x2F
#define REG_CLOCK_MANUAL      0x32
#define REG_CLOCK_ENABLE      0x33
#define REG_SYN_LOCK_CNT      0x38
#define REG_MID1              0x3C
#define REG_MID2              0x3D
#define REG_MID3              0x3E
#define REG_MID4              0x3F

// Scale bitmap
const unsigned char scale[] PROGMEM = {
	0xEE,0x22,0xE2,0x02,0x02,0x06,0x02,0x02,0x42,0xE2,0x0E,0xE2,0x22,0xE2,0x02,0x06,
	0x02,0xA2,0xA2,0xE2,0x0E,0xE2,0x22,0xE2,0x02,0x06,0x02,0xA2,0xA2,0xE2,0x0E,0xE2,
	0x22,0xE2,0x02,0x06,0x02,0xE2,0x82,0xE2,0x0E,0xE2,0x22,0xE2,0x02,0x06,0x02,0xE2,
	0xA2,0xA2,0x0E,0xE2,0x22,0xE2,0x02,0x06,0x02,0xE2,0xA2,0xA2,0x0E,0xE2,0x22,0xE2,
	0x02,0x06,0x02,0x62,0x22,0xE2,0x0E,0xE2,0x22,0xE2,0x02,0x06,0x02,0xE2,0xA2,0xE2,
	0x0E,0xE2,0x22,0xE2,0x02,0x06,0x02,0xE2,0xA2,0xE2,0x0E,0xE2,0x22,0xE2,0x02,0x06,
	0x03,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x02,0x03,0x00,0x00,
	0x00,0x03,0x02,0x02,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x02,0x02,0x03,0x00,0x03,
	0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x02,
	0x02,0x03,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x03,0x02,0x03,0x00,0x03,0x02,0x03,
	0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x03,0x02,0x03,
	0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x02,0x02,0x03,0x00,0x03,0x02,0x03,0x00,0x00};

	// Character lookup table code was taken from the work of Sylvain Bissonette
	// This table defines the standard ASCII characters in a 5x7 dot format.
	static const unsigned char FontLookup [][5] PROGMEM={
		{ 0x00, 0x00, 0x00, 0x00, 0x00 },   // space
		{ 0x00, 0x00, 0x2f, 0x00, 0x00 },   // !
		{ 0x00, 0x07, 0x00, 0x07, 0x00 },   // "
		{ 0x14, 0x7f, 0x14, 0x7f, 0x14 },   // #
		{ 0x24, 0x2a, 0x7f, 0x2a, 0x12 },   // $
		{ 0xc4, 0xc8, 0x10, 0x26, 0x46 },   // %
		{ 0x36, 0x49, 0x55, 0x22, 0x50 },   // &
		{ 0x00, 0x05, 0x03, 0x00, 0x00 },   // '
		{ 0x00, 0x1c, 0x22, 0x41, 0x00 },   // (
		{ 0x00, 0x41, 0x22, 0x1c, 0x00 },   // )
		{ 0x14, 0x08, 0x3E, 0x08, 0x14 },   // *
		{ 0x08, 0x08, 0x3E, 0x08, 0x08 },   // +
		{ 0x00, 0x00, 0x50, 0x30, 0x00 },   // ,
		{ 0x10, 0x10, 0x10, 0x10, 0x10 },   // -
		{ 0x00, 0x60, 0x60, 0x00, 0x00 },   // .
		{ 0x20, 0x10, 0x08, 0x04, 0x02 },   // /
		{ 0x3E, 0x51, 0x49, 0x45, 0x3E },   // 0
		{ 0x00, 0x42, 0x7F, 0x40, 0x00 },   // 1
		{ 0x42, 0x61, 0x51, 0x49, 0x46 },   // 2
		{ 0x21, 0x41, 0x45, 0x4B, 0x31 },   // 3
		{ 0x18, 0x14, 0x12, 0x7F, 0x10 },   // 4
		{ 0x27, 0x45, 0x45, 0x45, 0x39 },   // 5
		{ 0x3C, 0x4A, 0x49, 0x49, 0x30 },   // 6
		{ 0x01, 0x71, 0x09, 0x05, 0x03 },   // 7
		{ 0x36, 0x49, 0x49, 0x49, 0x36 },   // 8
		{ 0x06, 0x49, 0x49, 0x29, 0x1E },   // 9
		{ 0x00, 0x36, 0x36, 0x00, 0x00 },   // :
		{ 0x00, 0x56, 0x36, 0x00, 0x00 },   // ;
		{ 0x08, 0x14, 0x22, 0x41, 0x00 },   // <
		{ 0x14, 0x14, 0x14, 0x14, 0x14 },   // =
		{ 0x00, 0x41, 0x22, 0x14, 0x08 },   // >
		{ 0x02, 0x01, 0x51, 0x09, 0x06 },   // ?
		{ 0x32, 0x49, 0x59, 0x51, 0x3E },   // @
		{ 0x7E, 0x11, 0x11, 0x11, 0x7E },   // A
		{ 0x7F, 0x49, 0x49, 0x49, 0x36 },   // B
		{ 0x3E, 0x41, 0x41, 0x41, 0x22 },   // C
		{ 0x7F, 0x41, 0x41, 0x22, 0x1C },   // D
		{ 0x7F, 0x49, 0x49, 0x49, 0x41 },   // E
		{ 0x7F, 0x09, 0x09, 0x09, 0x01 },   // F
		{ 0x3E, 0x41, 0x49, 0x49, 0x7A },   // G
		{ 0x7F, 0x08, 0x08, 0x08, 0x7F },   // H
		{ 0x00, 0x41, 0x7F, 0x41, 0x00 },   // I
		{ 0x20, 0x40, 0x41, 0x3F, 0x01 },   // J
		{ 0x7F, 0x08, 0x14, 0x22, 0x41 },   // K
		{ 0x7F, 0x40, 0x40, 0x40, 0x40 },   // L
		{ 0x7F, 0x02, 0x0C, 0x02, 0x7F },   // M
		{ 0x7F, 0x04, 0x08, 0x10, 0x7F },   // N
		{ 0x3E, 0x41, 0x41, 0x41, 0x3E },   // O
		{ 0x7F, 0x09, 0x09, 0x09, 0x06 },   // P
		{ 0x3E, 0x41, 0x51, 0x21, 0x5E },   // Q
		{ 0x7F, 0x09, 0x19, 0x29, 0x46 },   // R
		{ 0x46, 0x49, 0x49, 0x49, 0x31 },   // S
		{ 0x01, 0x01, 0x7F, 0x01, 0x01 },   // T
		{ 0x3F, 0x40, 0x40, 0x40, 0x3F },   // U
		{ 0x1F, 0x20, 0x40, 0x20, 0x1F },   // V
		{ 0x3F, 0x40, 0x38, 0x40, 0x3F },   // W
		{ 0x63, 0x14, 0x08, 0x14, 0x63 },   // X
		{ 0x07, 0x08, 0x70, 0x08, 0x07 },   // Y
		{ 0x61, 0x51, 0x49, 0x45, 0x43 },   // Z
		{ 0x00, 0x7F, 0x41, 0x41, 0x00 },   // [
		{ 0x55, 0x2A, 0x55, 0x2A, 0x55 },   // "Yen"
		{ 0x00, 0x41, 0x41, 0x7F, 0x00 },   // ]
		{ 0x04, 0x02, 0x01, 0x02, 0x04 },   // ^
		{ 0x40, 0x40, 0x40, 0x40, 0x40 },   // _
		{ 0x00, 0x01, 0x02, 0x04, 0x00 },   // '
		{ 0x20, 0x54, 0x54, 0x54, 0x78 },   // a
		{ 0x7F, 0x48, 0x44, 0x44, 0x38 },   // b
		{ 0x38, 0x44, 0x44, 0x44, 0x20 },   // c
		{ 0x38, 0x44, 0x44, 0x48, 0x7F },   // d
		{ 0x38, 0x54, 0x54, 0x54, 0x18 },   // e
		{ 0x08, 0x7E, 0x09, 0x01, 0x02 },   // f
		{ 0x0C, 0x52, 0x52, 0x52, 0x3E },   // g
		{ 0x7F, 0x08, 0x04, 0x04, 0x78 },   // h
		{ 0x00, 0x44, 0x7D, 0x40, 0x00 },   // i
		{ 0x20, 0x40, 0x44, 0x3D, 0x00 },   // j
		{ 0x7F, 0x10, 0x28, 0x44, 0x00 },   // k
		{ 0x00, 0x41, 0x7F, 0x40, 0x00 },   // l
		{ 0x7C, 0x04, 0x18, 0x04, 0x78 },   // m
		{ 0x7C, 0x08, 0x04, 0x04, 0x78 },   // n
		{ 0x38, 0x44, 0x44, 0x44, 0x38 },   // o
		{ 0x7C, 0x14, 0x14, 0x14, 0x08 },   // p
		{ 0x08, 0x14, 0x14, 0x18, 0x7C },   // q
		{ 0x7C, 0x08, 0x04, 0x04, 0x08 },   // r
		{ 0x48, 0x54, 0x54, 0x54, 0x20 },   // s
		{ 0x04, 0x3F, 0x44, 0x40, 0x20 },   // t
		{ 0x3C, 0x40, 0x40, 0x20, 0x7C },   // u
		{ 0x1C, 0x20, 0x40, 0x20, 0x1C },   // v
		{ 0x3C, 0x40, 0x30, 0x40, 0x3C },   // w
		{ 0x44, 0x28, 0x10, 0x28, 0x44 },   // x
		{ 0x0C, 0x50, 0x50, 0x50, 0x3C },   // y
		{ 0x44, 0x64, 0x54, 0x4C, 0x44 }    // z
	};


// Main
int main(void){
	static unsigned char signal[LCD_VISIBLE_X_RES];
	unsigned char fast=0;
	unsigned char adding=0;
	unsigned char voltage[4];
	unsigned int volt;
	unsigned char s;	
	unsigned char z;
	
	// Hardware initialization
	SPI_Init();
	LCD_Init();
	RADIO_Init();
	ADC_Init();
	KBRD_Init();
	// Draw the scale at line 5
	memcpy_P(LcdCache+(LCD_VISIBLE_X_RES*5),scale,192);
	// Main loop
	while(1){
		// Start an ACD conversion
		ADC_Conversion();
		// if left key is pressed, toggle "adding"
		if (KBRD_L()) adding = ~adding;
		// Read if right key is pressed
		z=KBRD_R();
		if (adding){
			// If mode is adding, clear signal array
			if (z) memset(signal,0x00,LCD_VISIBLE_X_RES);
		}else{
			// If mode is normal, toggle "fast"
			if (z) fast = ~fast;
		}
		// Read signals and process according "adding" and "fast"
		for (unsigned char n=0; n<LCD_VISIBLE_X_RES; n++){
			// Read the signal on current channel
			s=RADIO_RSSI(n);
			if (adding) {
				// If mode is adding, update only if greater
				if (s>signal[n]) signal[n]=s;
			}else{
				// Mode normal
				if (fast){
					// if fast, nothing to do
					signal[n]=s;
				}else{
					// normal-slow
					if (s>signal[n]){
						// If signal if greater, update
						signal[n]=s;
					} else {
						// if not, decrease it
						if (signal[n]!=0) signal[n]--;
					}
				}
			}
		}
		// Draw the signal array
		for (unsigned char n=0; n<LCD_VISIBLE_X_RES; n++){
			LCD_Line(n,8,n,39,PIXEL_OFF);
			LCD_Line(n,39,n,39-signal[n],PIXEL_ON);
		}
		// Battery voltage
		LCD_GotoXY(0,0);
		volt=ADC_Read();
		itoa(volt,voltage,10);
		// Under 3.60, low battery
		if (volt<360)
			LCD_FStr((unsigned char*)PSTR("Low batt   "));
		else 
			LCD_FStr((unsigned char*)PSTR("           "));
		// write buffer to LCD
		LCD_Chr(voltage[0]);
		LCD_Chr('.');
		LCD_Chr(voltage[1]);
		LCD_Chr(voltage[2]);
		LCD_Chr('V');
		// Set lower line acording to actual mode
		LCD_GotoXY(0,7);
		if(adding){LCD_FStr((unsigned char*)PSTR("Mode       Reset"));}
		else{
			if(fast){
			LCD_FStr((unsigned char*)PSTR("Mode        Fast"));
			}
			else
			{
			LCD_FStr((unsigned char*)PSTR("Mode        Slow"));
			}
		}
		// Update LCD
		LCD_Update();
	}
}



/////////////////////////////////////////////////////////////////
// SPI interface functions
/////////////////////////////////////////////////////////////////

// Initialices SPI hardware
void SPI_Init(void){
	// Set output bits on SPI Port
    SPI_DDR |= _BV(SPI_MOSI_PIN) | _BV(SPI_CLK_PIN) | _BV(SPI_SS_PIN);
	// Enable SPI port: No interrupt, MSBit first, Master mode, 
	// CPOL->0, CPHA->0, Clk/4
    SPCR = 0x50;
}



// Send-receive a byte through SPI interface
unsigned char SPI(unsigned char data){
	// Send data
	SPDR=data;
	// Wait until data is sent
	while( ( SPSR & 0x80 ) != 0x80 );
	// Return received byte
	return SPDR;
}



/////////////////////////////////////////////////////////////////
// LCD functions
/////////////////////////////////////////////////////////////////

unsigned int LcdCacheIdx;

// Send data to LCD controller
void LCD_Send(LcdCmdData cd, unsigned char data){
	// Set D/C line
    if (cd == LCD_DATA)
		LCD_PORT |= _BV(LCD_DC_PIN);
    else
		LCD_PORT &= ~(_BV(LCD_DC_PIN));
    // Enable display controller (active low)
    LCD_PORT &= ~(_BV(LCD_CE_PIN));	
    // Send data
	SPI(data);
    // Disable display controller
    LCD_PORT |= _BV( LCD_CE_PIN );
}



// Initializes LCD
void LCD_Init(void){
	// Set LCD output pins
    LCD_DDR |= _BV(LCD_RST_PIN) | _BV(LCD_DC_PIN);
    // Resets the LCD controler
    LCD_PORT &= ~(_BV(LCD_RST_PIN));
    _delay_ms(1);
    LCD_PORT |= _BV(LCD_RST_PIN);
    // Set LCD CE pin to 1 (Disabled)
    LCD_PORT |= _BV(LCD_CE_PIN);
	// LCD Controller initialization
    LCD_Send(LCD_CMD,0x21); // LCD Extended Commands
    LCD_Send(LCD_CMD,0xC8); // Set LCD Vop (Contrast)
    LCD_Send(LCD_CMD,0x06); // Set Temp coefficent
    LCD_Send(LCD_CMD,0x13); // LCD bias mode 1:48
    LCD_Send(LCD_CMD,0x20); // LCD Standard Commands,Horizontal addressing mode
    LCD_Send(LCD_CMD,0x0C); // LCD in normal mode
    // Clear LCD Cache
    memset(LcdCache,0x00,LCD_CACHE_SIZE);
	//Clear LCD Device
	LCD_Send(LCD_CMD, 0x80);
    LCD_Send(LCD_CMD, 0x40);
	for(int n=0; n<(LCD_REAL_X_RES*LCD_REAL_Y_RES); n++)
		LCD_Send(LCD_DATA,0x00);
}



// Move text cursor to X,Y coordinates
void LCD_GotoXY(unsigned char x, unsigned char y){
    // Boundary check!
    if(x>((LCD_VISIBLE_X_RES/6)-1)) return;
    if(y>((LCD_VISIBLE_Y_RES/8)-1)) return;
    // Calculate index. It is defined as address within LcdCache
    LcdCacheIdx = ( x * 6) + ( y * LCD_VISIBLE_X_RES);
}



// Displays a character at current cursor location and increment cursor location.
void LCD_Chr (unsigned char ch){
    unsigned char i;
	// Convert to a printable character.
    if ((ch<0x20)||(ch>0x7b)) ch = 92;
	// Copy lookup table from Flash ROM to LcdCache
	for (i=0; i<5; i++) LcdCache[LcdCacheIdx++]=pgm_read_byte(&(FontLookup[ch-32][i]))<<1;
	// Horizontal gap between characters
    LcdCache[LcdCacheIdx]=0x00;
    // At index number LCD_CACHE_SIZE - 1, wrap to 0
    if(LcdCacheIdx==(LCD_CACHE_SIZE-1)){
        LcdCacheIdx = 0;
	} else {
		// Otherwise just increment the index
		LcdCacheIdx++;
	}
}



// Displays a string at current cursor location and increment
// cursor location. This function is dedicated to print string
// laid in SRAM
void LCD_Str(unsigned char dataArray[]){
    unsigned char tmpIdx=0;
    while(dataArray[tmpIdx]!='\0') LCD_Chr(dataArray[tmpIdx++]);
}



// Displays a string at current cursor location and increment
// cursor location. This function is dedicated to print string
// laid in Flash ROM
void LCD_FStr(const unsigned char *dataPtr){
    unsigned char c;
    for (c=pgm_read_byte(dataPtr); c; ++dataPtr, c=pgm_read_byte(dataPtr)) LCD_Chr(c);
}



// Displays a pixel at given absolute (x, y) location.
unsigned char LCD_Pixel(unsigned char x, unsigned char y, LcdPixelMode mode ){
    unsigned int index;
    unsigned char offset;
    unsigned char data;

    // Prevent from getting out of border
    if (x > (LCD_VISIBLE_X_RES-1)) return OUT_OF_BORDER;
    if (y > (LCD_VISIBLE_Y_RES-1)) return OUT_OF_BORDER;
    // Recalculating index and offset
    index=((y/8)*LCD_VISIBLE_X_RES)+x;
    offset=y-((y/8)*8);
    // Bit processing
    data=LcdCache[index];
    if (mode==PIXEL_ON) data |= (0x01<<offset);          // On mode
	else if (mode==PIXEL_OFF) data &= (~(0x01<<offset)); // Clear mode
    else if (mode==PIXEL_XOR) data ^= (0x01<<offset);    // Xor mode
    LcdCache[index]=data;
	return OK;
}



// Draws a line between two points on the display.
void LCD_Line ( unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, LcdPixelMode mode){
    int dx, dy, stepx, stepy, fraction;
    unsigned char response;

    // Calculate differential form
    // dy   y2 - y1
    // -- = -------
    // dx   x2 - x1
    // Take differences
    dy = y2 - y1;
    dx = x2 - x1;
    // dy is negative
    if ( dy < 0 ) {
        dy    = -dy;
        stepy = -1;
    } else {
        stepy = 1;
    }
    // dx is negative
    if ( dx < 0 ){
        dx    = -dx;
        stepx = -1;
    } else {
        stepx = 1;
    }
    dx <<= 1;
    dy <<= 1;
    // Draw initial position
    response = LCD_Pixel( x1, y1, mode );
    if(response) return;
    // Draw next positions until end
    if ( dx > dy )
    {
        // Take fraction
        fraction = dy - ( dx >> 1);
        while ( x1 != x2 )
        {
            if ( fraction >= 0 )
            {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;

            // Draw calculated point
            response = LCD_Pixel( x1, y1, mode );
            if(response) return;
        }
    }
    else
    {
        // Take fraction
        fraction = dx - ( dy >> 1);
        while ( y1 != y2 )
        {
            if ( fraction >= 0 )
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            // Draw calculated point
            response = LCD_Pixel( x1, y1, mode );
            if(response) return;
        }
    }
}



// Dump chache to LCD
void LCD_Update(void){
	unsigned int i=0;
	for (unsigned char row=0; row<(LCD_VISIBLE_Y_RES / 8); row++){
		LCD_Send(LCD_CMD, 0x80);
		LCD_Send(LCD_CMD, 0x40 | row);
		for (unsigned char col=0; col<LCD_VISIBLE_X_RES; col++)
			LCD_Send(LCD_DATA,LcdCache[i++]);
	}
}



/////////////////////////////////////////////////////////////////
// Radio Module Functions
/////////////////////////////////////////////////////////////////

// Reset Radio Module
void RADIO_Init(void){
	// Set Radio output pins
    RADIO_DDR |= _BV(RADIO_RST_PIN) | _BV(RADIO_CE_PIN);
	// Reset the radio
	RADIO_PORT |= _BV(RADIO_RST_PIN) | _BV(RADIO_CE_PIN);
    _delay_ms(100);
    RADIO_PORT |= _BV(RADIO_RST_PIN);
	// Radio module initialization
	RADIO_Write(REG_CLOCK_MANUAL,0x41);
    RADIO_Write(REG_CLOCK_ENABLE,0x41);
	RADIO_Write(REG_ANALOG_CTL,0x44);	
	RADIO_Write(REG_CRYSTAL_ADJ,0x40);
    RADIO_Write(REG_VCO_CAL,0xC0);
}



// Read data from radio module
unsigned char RADIO_Read(unsigned char address){
    unsigned char value;
	// Enable module
    RADIO_PORT &= ~(_BV(RADIO_CE_PIN));
	// Send address
    SPI(address);
	// Receive data
	value=SPI(0x00);
	// Disable module
    RADIO_PORT |= _BV(RADIO_CE_PIN);
	return value;
}



// Write data to radio module
void RADIO_Write(unsigned char address, unsigned char value){
    // Enable module
	RADIO_PORT &= ~(_BV(RADIO_CE_PIN));
    // Send data
	SPI(0x80|address);
	SPI(value);
	// Disable module
    RADIO_PORT |= _BV(RADIO_CE_PIN);	
}



// Returns RSSI (0..31) for given channel
unsigned char RADIO_RSSI(unsigned char channel){
	unsigned char value;
	// Set channel
	RADIO_Write(REG_CHANNEL,channel);
	// Turn receiver on
	RADIO_Write(REG_CONTROL,0x80);
	// Wait to receiver start-up
	// SYNTH_SETTLE (200) + RECEIVER_READY (35) + RSSI_ADC_CONVERSION (50)
	_delay_us(285);
    while(1){
		// Force conversion
		RADIO_Write(REG_CARRIER_DETECT,0x00);
		RADIO_Write(REG_CARRIER_DETECT,0x80);
		// RSSI_ADC_CONVERSION (50)
		_delay_us(50);
		// Read RSSI
		value=RADIO_Read(REG_RSSI);
		// Exit if valid
		if (value&0x20) break;
	}
	// Turn receiver off
	RADIO_Write(REG_CONTROL,0x00);
	// Return lower 5 bits
	return (value&0x1F);
}



/////////////////////////////////////////////////////////////////
// ADC Functions
/////////////////////////////////////////////////////////////////

// Initialices ADC hardware
void ADC_Init(void){
	// Internal Vref, ADC5
	ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX2) | _BV(MUX0);
	// Enable ADC, CLK/32
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0) ;
}



// Start an ADC conversion
void ADC_Conversion(void){
	ADCSRA = ADCSRA | _BV(ADSC);
}



// Read a conversion
unsigned int ADC_Read(void){
	// Wait if conversion is still running
	while (ADCSRA & 0x40);
	// Return voltage in centivolts (100K/10K divisor)
	return ((ADC*100)/35);
}



/////////////////////////////////////////////////////////////////
// Keyboard Functions
/////////////////////////////////////////////////////////////////

unsigned char LAST_LEFT_KEY;
unsigned char LAST_RIGHT_KEY;

// Initialices keyboard
void KBRD_Init(void){
	// Configure key pins as inputs
	KBRD_DDR &=~ _BV(RIGHT_KEY) | _BV(LEFT_KEY);
	// Pull up resistors in key pins
	PORTD |= _BV(RIGHT_KEY) | _BV(LEFT_KEY);
}



// Checks if Left key is pressed
unsigned char KBRD_L(void){
	unsigned char now;
	unsigned char value;
	// Read pin status
	now = KBRD_PIN & _BV(LEFT_KEY);
	// check if key have been pressed
	if ( (now==0) & (LAST_LEFT_KEY!=0)) value=1; else value=0;
	// Update variable for use next time
	LAST_LEFT_KEY=now;
	// Return in changed
	return value;
}



// Checks if Right key is pressed
unsigned char KBRD_R(void){
	unsigned char now;
	unsigned char value;
	// Read pin status
	now = KBRD_PIN & _BV(RIGHT_KEY);
	// check if key have been pressed
	if ( (now==0) & (LAST_RIGHT_KEY!=0)) value=1; else value=0;
	// Update variable for use next time
	LAST_RIGHT_KEY=now;
	// Return in changed
	return value;
}
