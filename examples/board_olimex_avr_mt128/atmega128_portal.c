// NOTE: lines below added to allow compilation in simavr's build system
#undef F_CPU
#define F_CPU 16000000

#include <limits.h>
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

/*	Sample program for Olimex AVR-MT-128 with ATMega-128 processor
 *	Button 1 (Up)		- Turn ON the display
 *	Button 2 (Left)		- Set Text1 on the upline
 *	Button 3 (Middle)	- Hold to slide Text
 *	Button 4 (Right)	- Set Text2 on the downline
 *	Button 5 (Down)		- Turn OFF the display
 *	Compile with AVRStudio+WinAVR (gcc version 3.4.6)
 */

 #define Text1	"Conflux"
 #define Text2	"Rampard"



#include "avr/io.h"
#include "avr_lcd.h" // NOTE: changed header name to better fit in simavr's file naming conventions

#define	__AVR_ATMEGA128__	1

unsigned char data, Line = 0;
char Text[16], Ch;
unsigned int Bl = 1, LCD_State = 0, i, j;

void Delay(unsigned int b)
{
  volatile unsigned int a = b;  // NOTE: volatile added to prevent the compiler to optimization the loop away
  while (a)
	{
		a--;
	}
}

/*****************************L C D**************************/

void E_Pulse()
{
	PORTC = PORTC | 0b00000100;	//set E to high
	Delay(1400); 				//delay ~110ms
	PORTC = PORTC & 0b11111011;	//set E to low
}

void LCD_Init()
{
	//LCD initialization
	//step by step (from Gosho) - from DATASHEET

	PORTC = PORTC & 0b11111110;

	Delay(10000);


	PORTC = 0b00110000;						//set D4, D5 port to 1
	E_Pulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00110000;						//set D4, D5 port to 1
	E_Pulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00110000;						//set D4, D5 port to 1
	E_Pulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00100000;						//set D4 to 0, D5 port to 1
	E_Pulse();								//high->low to E port (pulse)
}

void LCDSendCommand(unsigned char a)
{
	data = 0b00001111 | a;					//get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	PORTC = PORTC & 0b11111110;				//set RS port to 0
	E_Pulse();                              //pulse to set D4-D7 bits

	data = a<<4;							//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//set D4-D7
	PORTC = PORTC & 0b11111110;				//set RS port to 0 -> display set to command mode
	E_Pulse();                              //pulse to set d4-d7 bits

}

void LCDSendChar(unsigned char a)
{
	data = 0b00001111 | a;					//get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	PORTC = PORTC | 0b00000001;				//set RS port to 1
	E_Pulse();                              //pulse to set D4-D7 bits

	data = a<<4;							//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//clear D4-D7
	PORTC = PORTC | 0b00000001;				//set RS port to 1 -> display set to command mode
	E_Pulse();                              //pulse to set d4-d7 bits
}

static unsigned char cgram[64] = {
        0b00000, 0b01110, 0b01010, 0b01010, 0b01010, 0b01110, 0b00000, 0b11111,
        0b00000, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b11111,
        0b00000, 0b01110, 0b00010, 0b01110, 0b01000, 0b01110, 0b00000, 0b11111,
        0b00000, 0b01110, 0b00010, 0b01110, 0b00010, 0b01110, 0b00000, 0b11111,
        0b00000, 0b01000, 0b01000, 0b01110, 0b00100, 0b00100, 0b00000, 0b11111,
        0b00000, 0b01110, 0b01000, 0b01110, 0b00010, 0b01110, 0b00000, 0b11111,
        0b00000, 0b01110, 0b01000, 0b01110, 0b01010, 0b01110, 0b00000, 0b11111,
        0b00000, 0b01110, 0b00010, 0b00010, 0b00010, 0b00010, 0b00000, 0b11111,
};

static void LCDInitCGRAM()
{
    LCDSendCommand(0x40);
    for (unsigned int i = 0; i < 64; ++i) {
        LCDSendChar(cgram[i]);
    }
}

void LCDSendTxt(char* a)
{
	int Temp;
	for(Temp=0; Temp<strlen(a); Temp++)
  {
    LCDSendChar(a[Temp]);
  }
}

void LCDSendInt(long a)
{
	int C[20];
	unsigned char Temp=0, NumLen = 0;
	if (a < 0)
	{
		LCDSendChar('-');
		a = -a;
	}
	do
	{
		Temp++;
		C[Temp] = a % 10;
		a = a/10;
	}
	while (a);
	NumLen = Temp;
	for (Temp = NumLen; Temp>0; Temp--) LCDSendChar(C[Temp] + 48);
}

void LCDSendInt_Old(int a)
{
  int h = 0;
  int l = 0;

  l = a%10;
  h = a/10;

  LCDSendChar(h+48);
  LCDSendChar(l+48);
}

void SmartUp(void)
{
	int Temp;
	for(Temp=0; Temp<1; Temp++) LCDSendCommand(CUR_UP);
}

void SmartDown(void)
{
	int Temp;
	for(Temp=0; Temp<40; Temp++) LCDSendCommand(CUR_DOWN);
}

void Light(short a)
{
  if(a == 1)
  {
	PORTC = PORTC | 0b00100000;
	DDRC = PORTC | 0b00100000;

    //IO0SET_bit.P0_25 = 1;
    //IO0DIR_bit.P0_25 = 1;
  }
  if(a == 0)
  {
    PORTC = PORTC & 0b11011111;
    DDRC = DDRC & 0b11011111;

    //IO0SET_bit.P0_25 = 0;
    //IO0DIR_bit.P0_25 = 0;
  }

}

/*****************************L C D**************************/

/*************************Game Logic*********************/

const static short numberOfLines = 2;
const static short lineLength = 16;
const static short mapSize = numberOfLines * lineLength;

// Empty map tile
const char F = INT_MAX - 1;
// Empty/pit tile
const char E = INT_MAX;
// Player
const char P = 'S';
// Portal gun crosshair
const char C = 'C';
// Entrance portal
const char EP = 'o';
// Exit portal
const char XP = 'O';

int reRender = 1;

int isPortalGunEnabled = 0;

// 0 = the entrance portal will be place
// 1 = the exit portal will be placed
int portalState = 0;

unsigned playerPosX = 0;
unsigned playerPosY = 0;

unsigned crosshairPosX = 0;
unsigned crosshairPosy = 0;

unsigned entrancePortalPosX = 0;
unsigned entrancePortalPosY = 0;

unsigned exitPortalPosX = 0;
unsigned exitPortalPosY = 0;

char map[32] = {
        P, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
        F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
};

unsigned Map2DCoordsTo1D(unsigned x, unsigned y)
{
    return (y * lineLength) + x;
}

int ValidateNewPosition(unsigned newPlayerPosX, unsigned newPlayerPosY)
{
    // Check if the player is out of bounds
    if (newPlayerPosX > lineLength - 1 || newPlayerPosY > numberOfLines - 1) {
        return 0;
    }

    return 1;
}

void UpdatePlayerPos(int xOffset, int yOffset)
{
    // Update player pos
    unsigned newPlayerPosX = playerPosX + xOffset;
    unsigned newPlayerPosY = playerPosY + yOffset;

    if (!ValidateNewPosition(newPlayerPosX, newPlayerPosY)) {
        return;
    }

    unsigned playerPos = Map2DCoordsTo1D(playerPosX, playerPosY);
    unsigned newPlayerPos = Map2DCoordsTo1D(newPlayerPosX, newPlayerPosY);

    char tileToMoveTo = map[newPlayerPos];

    switch (tileToMoveTo) {
        case F:
        {
            // Player moved from tile, make it an empty tile
            map[playerPos] = F;

            // Player moved to tile, make it a player tile
            map[newPlayerPos] = P;

            playerPosX = newPlayerPosX;
            playerPosY = newPlayerPosY;
            break;
        }
        case EP:
        {
            // Both portals haven't been placed
            if (portalState != 2) {
                break;
            }

            unsigned portalPos = Map2DCoordsTo1D(exitPortalPosX, exitPortalPosY);

            playerPosX = exitPortalPosX;
            playerPosY = exitPortalPosY;

            map[playerPos] = F;
            map[portalPos] = P;
            break;
        }
        case XP:
        {
            // Both portals haven't been placed
            if (portalState != 2) {
                break;
            }

            unsigned portalPos = Map2DCoordsTo1D(entrancePortalPosX, entrancePortalPosY);

            playerPosX = entrancePortalPosX;
            playerPosY = entrancePortalPosY;

            map[playerPos] = F;
            map[portalPos] = P;
            break;
        }
    }
}

void UpdateCrosshairPos(int xOffset, int yOffset)
{
    unsigned newCrosshairPosX = crosshairPosX + xOffset;
    unsigned newCrosshairPosY = crosshairPosy + yOffset;

    if (!ValidateNewPosition(newCrosshairPosX, newCrosshairPosY)) {
        return;
    }

    unsigned crosshairPos = Map2DCoordsTo1D(crosshairPosX, crosshairPosy);

    // Crosshair moved make the tile empty
    map[crosshairPos] = F;

    unsigned newCrosshairPos = Map2DCoordsTo1D(newCrosshairPosX, newCrosshairPosY);

    // The crosshair moved, make it a crosshair tile
    map[newCrosshairPos] = C;

    crosshairPosX = newCrosshairPosX;
    crosshairPosy = newCrosshairPosY;
}

void CleanupPortals()
{
    unsigned entrancePortalPos = Map2DCoordsTo1D(entrancePortalPosX, entrancePortalPosY);
    unsigned exitPortalPos = Map2DCoordsTo1D(exitPortalPosX, exitPortalPosY);

    portalState = 0;

    map[entrancePortalPos] = F;
    map[exitPortalPos] = F;
}

void TogglePortalGun()
{
    // If the 2 portals has already been placed clean them up
    if (portalState == 2) {
        CleanupPortals();

        return;
    }

    isPortalGunEnabled = (isPortalGunEnabled == 0) ? 1 : 0;
    unsigned playerPos = Map2DCoordsTo1D(playerPosX, playerPosY);

    if (isPortalGunEnabled == 1) {
        // If the portal gun is enabled place the crosshair on the player
        crosshairPosX = playerPosX;
        crosshairPosy = playerPosY;

        map[playerPos] = C;
    } else {
        // If the portal gun is disabled remove the crosshair
        unsigned crossHairPos = Map2DCoordsTo1D(crosshairPosX, crosshairPosy);

        map[playerPos] = P;

        if (portalState == 0) {
            portalState = 1;

            entrancePortalPosX = crosshairPosX;
            entrancePortalPosY = crosshairPosy;

            map[crossHairPos] = EP;
        } else {
            portalState = 2;

            exitPortalPosX = crosshairPosX;
            exitPortalPosY = crosshairPosy;

            map[crossHairPos] = XP;
        }
    }
}

/*************************Game Logic*********************/

void Port_Init()
{
	PORTA = 0b00011111;		DDRA = 0b01000000; // NOTE: set A4-0 to initialize buttons to unpressed state
	PORTB = 0b00000000;		DDRB = 0b00000000;
	PORTC = 0b00000000;		DDRC = 0b11110111;
	PORTD = 0b11000000;		DDRD = 0b00001000;
	PORTE = 0b00000000;		DDRE = 0b00110000;
	PORTF = 0b00000000;		DDRF = 0b00000000;
	PORTG = 0b00000000;		DDRG = 0b00000000;
}

int main()
{
	Port_Init();
	LCD_Init();
    //LCDInitCGRAM();
	// NOTE: added missing initialization steps
	LCDSendCommand(0x28); // function set: 4 bits interface, 2 display lines, 5x8 font
	LCDSendCommand(DISP_OFF); // display off, cursor off, blinking off
	LCDSendCommand(CLR_DISP); // clear display
	LCDSendCommand(0x06); // entry mode set: cursor increments, display does not shift
	LCDSendCommand(DISP_OFF);
    LCDSendCommand(DISP_ON);
    LCD_State = 1;

	while (1)
	{
		//Value of Bl prevents holding the buttons
		//Bl = 0: One of the Buttons is pressed, release to press again
		//LCD_State value is the state of LCD: 1 - LCD is ON; 0 - LCD is OFF

		//Up Button (Button 1) : Turn ON Display
 		if (!(PINA & 0b00000001) & Bl & LCD_State)	//check state of button 1 and value of Bl and LCD_State
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(0, -1) : UpdateCrosshairPos(0, -1);

            reRender = 1;
            Bl = 0;
		}

        //Down Button (Button 5) Turn OFF Display
        if (!(PINA & 0b00010000) & Bl & LCD_State)	//check state of button 5 and value of Bl and LCD_State
        {
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(0, 1) : UpdateCrosshairPos(0, 1);

            reRender = 1;
            Bl = 0;
        }

		//Left Button (Button 2) : Set Text1
		if (!(PINA & 0b00000010) & Bl & LCD_State)		//check state of button 2 and value of Bl and LCD_State
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(-1, 0) : UpdateCrosshairPos(-1, 0);

            reRender = 1;
            Bl = 0;
		}

		//Right Button (Button 4) Set Text2
		if (!(PINA & 0b00001000) & Bl & LCD_State)	//check state of button 4 and value of Bl and LCD_State
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(1, 0) : UpdateCrosshairPos(1, 0);

            reRender = 1;
            Bl = 0;
		}

        //Middle Button (Button 3) : Slide the text
        if (!(PINA & 0b00000100) & Bl & LCD_State)
        {
            TogglePortalGun();
            reRender = 1;
            Bl = 0;
        }

        if (reRender == 1)
        {
            LCDSendCommand(CLR_DISP);
            LCDSendCommand(DD_RAM_ADDR);
            Line = 1;

            for (int idx = 0; idx < mapSize; idx++) {
                if (idx == lineLength) {
                    LCDSendCommand(DD_RAM_ADDR2);
                    Line = 2;
                }

                LCDSendChar(map[idx]);
            }

            reRender = 0;
        }

		//check state of all buttons
		if (( (PINA & 0b00000001)
			| (PINA & 0b00000010)
			| (PINA & 0b00000100)
			| (PINA & 0b00001000)
			| (PINA & 0b00010000)) == 31)
        {
            Bl = 1;		//if all buttons are released Bl gets value 1
        }
	}

	return 0;
}
