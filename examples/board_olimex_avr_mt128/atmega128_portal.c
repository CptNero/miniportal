// NOTE: lines below added to allow compilation in simavr's build system
#undef F_CPU
#define F_CPU 16000000

#include <limits.h>
#include <string.h>

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

unsigned char data;
unsigned int Bl = 1, LCD_State = 0, i, j;

void Delay(unsigned long b)
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
        // Pushed down pressure plate
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b01110,
        0b01110,
        0b01110,
        0b11111,

        // Player symbol
        0b00110,
        0b10110,
        0b11000,
        0b11001,
        0b10101,
        0b10010,
        0b01000,
        0b10000,

        // Crosshair symbol
        0b01110,
        0b10101,
        0b10101,
        0b11111,
        0b11111,
        0b10101,
        0b10101,
        0b01110,

        // Goal symbol
        0b00011,
        0b00100,
        0b00100,
        0b11111,
        0b11111,
        0b00000,
        0b11111,
        0b11111,

        // Horizontal wall symbol
        0b00000,
        0b00000,
        0b10001,
        0b11111,
        0b11111,
        0b10001,
        0b00000,
        0b00000,

        // Cross wall symbol
        0b00100,
        0b00100,
        0b00100,
        0b11111,
        0b11111,
        0b00100,
        0b00100,
        0b00100,

        // Pressure plate
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111,
        0b11111,

        // Companion cube
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b01110,
        0b01110,
        0b01110,
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

/*****************************L C D**************************/

/*************************Game Logic*********************/

const static unsigned numberOfLines = 8;
const static unsigned lineLength = 16;
const static unsigned mapSize = numberOfLines * lineLength;

// Floor tile
const char F = INT_MAX - 1;
// Pit tile
const char E = INT_MAX;
// Vertical Wall tile
const char VW = 'I';
// Horizontal wall tile
const char HW = 4;
// Cross wall tile
const char CW = 5;
// Pressure plate
const char PP = 6;
// Portal cube
const char PC = 7;
// Checkmark
const char PDP = 0;

// Goal tile
const char G = 3;

// Player
const char P = 1;
// Portal gun crosshair
const char C = 2;
// Entrance portal
const char EP = 'o';
// Exit portal
const char XP = 'O';

int reRender = 1;
int winCondition = 0;
unsigned mapCounter = 0;

int isPortalGunEnabled = 0;
int isHoldingCube = 0;

int portalGunDirectionX = 0;
int portalGunDirectionY = 0;

// 0 = the entrance portal will be place
// 1 = the exit portal will be placed
// 2 = both portals have been placed
int portalGunState = 0;

unsigned playerPosX = 0;
unsigned playerPosY = 0;

unsigned crosshairPosX = 0;
unsigned crosshairPosY = 0;

unsigned entrancePortalPosX = -1;
unsigned entrancePortalPosY = -1;

unsigned exitPortalPosX = -1;
unsigned exitPortalPosY = -1;

unsigned Map2DCoordsTo1D(unsigned x, unsigned y)
{
    return (y * lineLength) + x;
}

char map1[128] = {
    E, E,  E, VW, F,  E,  E,  E,  E,  E,  E,  E,  E,  E,  F,  G,
    E, P,  F, VW, F,  HW, HW, CW, HW, HW, HW, HW, HW, HW, HW, E,
    E, E,  E, VW, F,  E,  F,  VW, F,  E,  F,  HW, HW, CW, HW, E,
    E, F,  F, VW, E,  VW, F,  E,  F,  VW, F,  E,  F,  VW, F,  E,
    E, HW, F, CW, HW, HW, HW, HW, HW, HW, HW, HW, HW, HW, HW, E,
    E, E,  E, VW, F,  F,  E,  PC, E,  PP, F,  F,  F,  F,  F,  E,
    E, E,  F, F,  F,  F,  E,  F,  E,  F,  F,  F,  F,  F,  F,  E,
    E, E,  E, E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,
};

char map2[128] = {
    F,  F,  F,  F, F,  F, F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
    F,  F,  F,  F, F,  F, F,  CW, HW, HW, HW, HW, CW, F,  F,  F,
    F,  PC, F,  F, F,  F, F,  VW, PP, F,  F,  PC, VW, F,  F,  F,
    F,  F,  F,  F, F,  F, F,  VW, F,  P,  F,  F,  VW, F,  F,  F,
    E,  E,  E,  E, E,  E, E,  CW, HW, HW, HW, HW, CW, HW, HW, HW,
    F,  E,  F,  E, F,  E, VW, E,  E,  E,  E,  E,  E,  E,  E,  E,
    F,  E,  F,  E, F,  E, VW, F,  PC, VW, PC, VW, F,  F,  F,  F,
    PP, E,  PP, E, PP, E, VW, E,  E,  E,  E,  E,  E,  E,  E,  G,
};

char map[128];

char mapMemory[128] = {
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
    F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,
};

void RWX12Y4(char tile)
{
    map[Map2DCoordsTo1D(12, 4)] = tile;
}

void RWX12Y2(char tile)
{
    map[Map2DCoordsTo1D(12, 2)] = tile;
}

void RWX6Y6(char tile)
{
    map[Map2DCoordsTo1D(6, 6)] = tile;
}

void RWX9Y9(char tile)
{
    map[Map2DCoordsTo1D(9, 6)] = tile;
}

void RWX11Y6(char tile)
{
    map[Map2DCoordsTo1D(11, 6)] = tile;
}

// Events
void (*eventPtrs[128])(char) = {
        0,      0, 0,      0, 0,       0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, RWX12Y2, 0,       0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, 0,       RWX12Y4, 0, 0, 0, 0, 0, 0,
        0,      0, 0,      0, 0,       0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
        RWX6Y6, 0, RWX9Y9, 0, RWX11Y6, 0, 0, 0, 0,       0,       0, 0, 0, 0, 0, 0,
};

void MoveToTile(unsigned pos, unsigned newPos, char newTile, int delay)
{
    map[pos] = mapMemory[pos];
    mapMemory[newPos] = map[newPos];

    if (delay) {
        for (int i = 0; i < 24; i++) {
            Delay(99999999);
        }
    }

    map[newPos] = newTile;
}

int CheckBounds(unsigned newPlayerPosX, unsigned newPlayerPosY)
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

    if (!CheckBounds(newPlayerPosX, newPlayerPosY)) {
        return;
    }

    unsigned playerPos = Map2DCoordsTo1D(playerPosX, playerPosY);
    unsigned newPlayerPos = Map2DCoordsTo1D(newPlayerPosX, newPlayerPosY);

    char tileToMoveTo = map[newPlayerPos];

    switch (tileToMoveTo) {
        case F:
        {
            MoveToTile(playerPos, newPlayerPos, P, 0);

            playerPosX = newPlayerPosX;
            playerPosY = newPlayerPosY;
            break;
        }
        case EP:
        {
            // Both portals haven't been placed
            if (portalGunState != 2) {
                MoveToTile(playerPos, newPlayerPos, P, 0);

                playerPosX = newPlayerPosX;
                playerPosY = newPlayerPosY;
                break;
            }

            unsigned portalPos = Map2DCoordsTo1D(exitPortalPosX, exitPortalPosY);

            playerPosX = exitPortalPosX;
            playerPosY = exitPortalPosY;

            MoveToTile(playerPos, portalPos, P, 1);
            break;
        }
        case XP:
        {
            // Both portals haven't been placed
            if (portalGunState != 2) {
                MoveToTile(playerPos, newPlayerPos, P, 0);

                playerPosX = newPlayerPosX;
                playerPosY = newPlayerPosY;
                break;
            }

            unsigned portalPos = Map2DCoordsTo1D(entrancePortalPosX, entrancePortalPosY);

            playerPosX = entrancePortalPosX;
            playerPosY = entrancePortalPosY;

            MoveToTile(playerPos, portalPos, P, 1);
            break;
        }
        case PC:
        {
            if (!isHoldingCube) {
                isHoldingCube = 1;

                map[playerPos] = F;
                map[newPlayerPos] = P;

                playerPosX = newPlayerPosX;
                playerPosY = newPlayerPosY;
            }

            break;
        }
        case PP:
        {
            if (isHoldingCube) {
                isHoldingCube = 0;
                map[newPlayerPos] = PDP;

                (*eventPtrs[newPlayerPos])(F);
            }

            break;
        }
        case PDP:
        {
            if (!isHoldingCube) {
                isHoldingCube = 1;

                map[newPlayerPos] = PP;

                (*eventPtrs[newPlayerPos])(VW);
            }
        }
        case E:
        {
            break;
        }
        case G:
        {
            winCondition = 1;
        }
    }
}

void UpdateCrosshairPos(int xOffset, int yOffset)
{
    if (portalGunDirectionX == 0 && portalGunDirectionY == 0) {
        portalGunDirectionX = xOffset;
        portalGunDirectionY = yOffset;
    }

    // Make sure that the portal gun can only be shot in a straight line
    if (portalGunDirectionX != xOffset && portalGunDirectionY != yOffset) {
        return;
    }

    unsigned newCrosshairPosX = crosshairPosX + xOffset;
    unsigned newCrosshairPosY = crosshairPosY + yOffset;

    if (!CheckBounds(newCrosshairPosX, newCrosshairPosY)) {
        return;
    }

    unsigned crosshairPos = Map2DCoordsTo1D(crosshairPosX, crosshairPosY);
    unsigned newCrosshairPos = Map2DCoordsTo1D(newCrosshairPosX, newCrosshairPosY);

    char tileToMoveTo = map[newCrosshairPos];

    switch (tileToMoveTo) {
        case F:
        case E:
        case EP:
        case XP:
        case PC:
        {
            MoveToTile(crosshairPos, newCrosshairPos, C, 0);

            crosshairPosX = newCrosshairPosX;
            crosshairPosY = newCrosshairPosY;
            break;
        }
        case VW:
        case HW:
        case CW:
        {
            break;
        }
    }
}

void CleanupPortals()
{
    unsigned entrancePortalPos = Map2DCoordsTo1D(entrancePortalPosX, entrancePortalPosY);
    unsigned exitPortalPos = Map2DCoordsTo1D(exitPortalPosX, exitPortalPosY);

    portalGunState = 0;

    entrancePortalPosX = -1;
    entrancePortalPosY = -1;

    exitPortalPosX = -1;
    exitPortalPosY = -1;

    map[entrancePortalPos] = (map[entrancePortalPos] == P) ? P : F;
    map[exitPortalPos] = (map[exitPortalPos] == P) ? P : F;

    mapMemory[entrancePortalPos] = F;
    mapMemory[exitPortalPos] = F;
}

void TogglePortalGun()
{
    // Make sure that a portal cannot be placed on invalid objects
    if (isPortalGunEnabled) {
        if (crosshairPosX == playerPosX && crosshairPosY == playerPosY) {
            return;
        }

        const char tileCrossHairAt = mapMemory[Map2DCoordsTo1D(crosshairPosX, crosshairPosY)];
        switch (tileCrossHairAt) {
            case E:
            case P:
            case EP:
            case XP:
            case PC:
            case PP: {
                return;
            }
            default: {
                break;
            }
        }
    }

    // If the 2 portals has already been placed clean them up
    if (portalGunState == 2) {
        CleanupPortals();

        return;
    }

    isPortalGunEnabled = (isPortalGunEnabled == 0) ? 1 : 0;

    // If the portal gun is disabled reset the direction
    if (isPortalGunEnabled == 0) {
        portalGunDirectionX = 0;
        portalGunDirectionY = 0;
    }

    unsigned playerPos = Map2DCoordsTo1D(playerPosX, playerPosY);

    // Enable portal gun
    if (isPortalGunEnabled == 1) {
        // If the portal gun is enabled place the crosshair on the player
        crosshairPosX = playerPosX;
        crosshairPosY = playerPosY;

        map[playerPos] = C;
    // Disable portal gun
    } else {
        // If the portal gun is disabled remove the crosshair
        unsigned crossHairPos = Map2DCoordsTo1D(crosshairPosX, crosshairPosY);

        map[playerPos] = P;

        if (portalGunState == 0) {
            portalGunState = 1;

            entrancePortalPosX = crosshairPosX;
            entrancePortalPosY = crosshairPosY;

            crosshairPosX = 0;
            crosshairPosY = 0;

            map[crossHairPos] = EP;
        } else {
            portalGunState = 2;

            exitPortalPosX = crosshairPosX;
            exitPortalPosY = crosshairPosY;

            crosshairPosX = 0;
            crosshairPosY = 0;

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

void ShowTitleScreen()
{
    int renderTitleScreen = 1;
    int playerPosInLine = 0;

    char titleScreenArt[16] = {
            P, ' ', ' ', ' ', ' ', 'P', 'O', 'R', 'T', 'A', 'L', ' ', ' ', ' ', ' ', ' '
    };

    LCDSendCommand(CLR_DISP);
    LCDSendCommand(DD_RAM_ADDR);
    LCDSendTxt("Press 5 to play!");
    LCDSendCommand(DD_RAM_ADDR2);
    LCDSendTxt(titleScreenArt);
    LCDSendCommand(DD_RAM_ADDR2);

    while (renderTitleScreen)
    {
        if (playerPosInLine == 16) {
            playerPosInLine = -1;
        }
        else {
            LCDSendCommand(DD_RAM_ADDR2 + playerPosInLine);
            LCDSendChar(' ');

            if (playerPosInLine == 4) {
                playerPosInLine += sizeof("PORTAL");
            } else {
                playerPosInLine++;
            }

            LCDSendCommand(DD_RAM_ADDR2 + playerPosInLine);
            LCDSendChar(P);

            for (int i = 0; i < 12; i++) {
                Delay(99999999);
            }
        }

        //Middle Button (Button 3) : Slide the text
        if (!(PINA & 0b00000100) & Bl & LCD_State)
        {
            renderTitleScreen = 0;

            Bl = 0;
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
}

void ResetGameState()
{
    crosshairPosX = 0;
    crosshairPosY = 0;
    entrancePortalPosX = -1;
    entrancePortalPosY = -1;
    exitPortalPosX = -1;
    exitPortalPosY = -1;
    portalGunState = 0;
    portalGunDirectionX = 0;
    portalGunDirectionY = 0;
    winCondition = 0;
    isPortalGunEnabled = 0;
    isHoldingCube = 0;
    reRender = 1;
}

void InitMap1()
{
    memcpy(map, map1, mapSize);
    memset(mapMemory, F, mapSize);
    playerPosX = 1;
    playerPosY = 1;
    ResetGameState();
}

void InitMap2()
{
    memcpy(map, map2, mapSize);
    memset(mapMemory, F, mapSize);
    playerPosX = 9;
    playerPosY = 3;
    ResetGameState();

}

const unsigned numberOfMaps = 2;
void (*mapPtrs[2])() = {
    InitMap2, InitMap1
};

int main()
{
	Port_Init();
	LCD_Init();
    LCDInitCGRAM();
	// NOTE: added missing initialization steps
	LCDSendCommand(0x28); // function set: 4 bits interface, 2 display lines, 5x8 font
	LCDSendCommand(DISP_OFF); // display off, cursor off, blinking off
	LCDSendCommand(CLR_DISP); // clear display
	LCDSendCommand(0x06); // entry mode set: cursor increments, display does not shift
	LCDSendCommand(DISP_OFF);
    LCDSendCommand(DISP_ON);
    LCD_State = 1;

    ShowTitleScreen();

    mapPtrs[mapCounter]();

	while (1)
	{
		//Value of Bl prevents holding the buttons
		//Bl = 0: One of the Buttons is pressed, release to press again

		//Up Button (Button 1) Move up
 		if (!(PINA & 0b00000001) & Bl)
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(0, -1) : UpdateCrosshairPos(0, -1);

            reRender = 1;
            Bl = 0;
		}

        //Down Button (Button 5) Move down
        if (!(PINA & 0b00010000) & Bl)
        {
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(0, 1) : UpdateCrosshairPos(0, 1);

            reRender = 1;
            Bl = 0;
        }

		//Left Button (Button 2) Move left
		if (!(PINA & 0b00000010) & Bl)
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(-1, 0) : UpdateCrosshairPos(-1, 0);

            reRender = 1;
            Bl = 0;
		}

		//Right Button (Button 4) Move right
		if (!(PINA & 0b00001000) & Bl)
		{
            (isPortalGunEnabled == 0) ? UpdatePlayerPos(1, 0) : UpdateCrosshairPos(1, 0);

            reRender = 1;
            Bl = 0;
		}

        //Middle Button (Button 3) Enable/Disable portal gun
        if (!(PINA & 0b00000100) & Bl)
        {
            TogglePortalGun();
            reRender = 1;
            Bl = 0;
        }

        if (reRender == 1)
        {
            LCDSendCommand(CLR_DISP);
            LCDSendCommand(DD_RAM_ADDR);

            unsigned viewPortOffset;
            unsigned relativePosY = (isPortalGunEnabled) ? crosshairPosY : playerPosY;

            switch (relativePosY) {
                case 0: {
                    viewPortOffset = 0;
                    break;
                }
                case numberOfLines - 2: {
                    viewPortOffset = relativePosY;
                    break;
                }
                default: {
                    viewPortOffset = relativePosY - 1;
                    break;
                }
            }

            unsigned renderAreaBegin = viewPortOffset * lineLength;
            unsigned renderAreaEnd = renderAreaBegin + (lineLength * 2);

            for (unsigned idx = renderAreaBegin; idx < renderAreaEnd; idx++) {
                if (idx > renderAreaBegin && idx % lineLength == 0) {
                    LCDSendCommand(DD_RAM_ADDR2);
                }

                LCDSendChar(map[idx]);
            }

            reRender = 0;
        }

        if (winCondition == 1) {
            LCDSendCommand(CLR_DISP);
            LCDSendCommand(DD_RAM_ADDR);
            LCDSendTxt("TheCakeWasALie");
            LCDSendCommand(DD_RAM_ADDR2);
            LCDSendTxt("LoveYou:)-Glados");

            for (int k = 0; k < 24; k++) {
                Delay(99999999);
            }

            mapCounter++;
            mapCounter %= numberOfMaps;
            mapPtrs[mapCounter]();
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
