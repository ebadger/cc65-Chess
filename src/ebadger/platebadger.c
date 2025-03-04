/*
 *  platA2.c
 *  cc65 Chess
 *
 *  Created by Oliver Schmidt, January 2020.
 *
 */

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include "../types.h"
#include "../globals.h"
#include "../undo.h"
#include "../frontend.h"
#include "../plat.h"

/*-----------------------------------------------------------------------*/
#define BOARD_PIECE_WIDTH       3
#define BOARD_PIECE_HEIGHT      22
#define CHAR_HEIGHT             8
#define LOG_WINDOW_HEIGHT       23
#define SCREEN_WIDTH            40
#define SCREEN_HEIGHT           192

#define ROP_CONST(val)          0xA980|(val)
#define ROP_BLACK               0xA980
#define ROP_WHITE               0xA9FF
#define ROP_XOR(val)            0x4900|(val)
#define ROP_CPY                 0x4980
#define ROP_INV                 0x49FF
#define ROP_AND(val)            0x2900|(val)

char rop_Line[2][7] = {{0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F},
                       {0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E}};

char rop_Color[2][2] = {{0x55, 0x2A}, {0xD5, 0xAA}};

extern char hires_CharSet[96][CHAR_HEIGHT];
extern char hires_Pieces[6][2][BOARD_PIECE_WIDTH*BOARD_PIECE_HEIGHT];

void hires_Init(void);
void hires_Done(void);
void hires_Text(char flag);
void hires_Draw(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop, char *src);
void hires_Mask(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop);

/*-----------------------------------------------------------------------*/
void plat_Drawchar(char x, char y, unsigned rop, char c)
{
    hires_Draw(x,y,1,CHAR_HEIGHT,rop,hires_CharSet[c-' ']);
}

/*-----------------------------------------------------------------------*/
void plat_Drawstring(char x, char y, unsigned rop, char *s)
{
    while(*s)
        plat_Drawchar(x++,y,rop,*s++);
}

/*-----------------------------------------------------------------------*/
void plat_Init()
{
    char i;
    
    // Clear the hires screen
    hires_Mask(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,ROP_BLACK);

    // Show the hires screen
    hires_Init();


    // Show credits and wait for key press
    plat_Drawstring(2,SCREEN_HEIGHT/2-20-CHAR_HEIGHT/2,
                    ROP_CPY,gszAbout);
    plat_Drawstring(2,SCREEN_HEIGHT/2-CHAR_HEIGHT/2,
                    ROP_CPY,"Apple][ version by O. Schmidt, 2020.");
    plat_Drawstring(2,SCREEN_HEIGHT/2+20-CHAR_HEIGHT/2,
                    ROP_CPY,"eb6502 version by Eric Badger, 2023.");
    hires_Draw(SCREEN_WIDTH/2-2,SCREEN_HEIGHT/2-50-BOARD_PIECE_HEIGHT/2,
               BOARD_PIECE_WIDTH,BOARD_PIECE_HEIGHT,ROP_CPY,
               hires_Pieces[KING-1][SIDE_BLACK]);
    hires_Draw(SCREEN_WIDTH/2-2,SCREEN_HEIGHT/2+50-BOARD_PIECE_HEIGHT/2,
               BOARD_PIECE_WIDTH,BOARD_PIECE_HEIGHT,ROP_CPY,
               hires_Pieces[KING-1][SIDE_WHITE]);
    plat_ReadKeys(1);

    // Draw the board border
    hires_Mask( 1, 12,1,8*BOARD_PIECE_HEIGHT+2*2,ROP_CONST(rop_Line[1][2]));
    hires_Mask(26, 12,1,8*BOARD_PIECE_HEIGHT+2*2,ROP_CONST(rop_Line[0][2]));
    hires_Mask( 2, 12,8*BOARD_PIECE_WIDTH,2,     ROP_WHITE);
    hires_Mask( 2,190,8*BOARD_PIECE_WIDTH,2,     ROP_WHITE);

    // Add the A..H and 1..8 tile-keys
    for(i=0; i<8; ++i)
    {
        plat_Drawchar(3+i*BOARD_PIECE_WIDTH,0,                ROP_CPY,i+'A');
        plat_Drawchar(0,SCREEN_HEIGHT-17-i*BOARD_PIECE_HEIGHT,ROP_CPY,i+'1');
    }

    // Setting this to 0 will not show the "Quit" option in the main menu
    gReturnToOS = 1;
}

/*-----------------------------------------------------------------------*/
void plat_UpdateScreen()
{
}

/*-----------------------------------------------------------------------*/
char plat_Menu(char **menuItems, char height, char *scroller)
{
    int keyMask;
    char i, j;

    clrscr();
    hires_Text(0x00);

    // Show the title or the scroller if that is more relevant.
    cputs(scroller == gszAbout ? menuItems[0] : scroller);

    // Select the first item
    i = 1;
    do
    {
        // Show all the menu items
        for(j=1; j<height; ++j)
        {
            if(!menuItems[j])
                break;

            gotoxy(((j-1)%2)*SCREEN_WIDTH/2,((j-1)/2)+1);

            // Highlight the selected item

            if(j == i)
            {
                cprintf("> %s <",menuItems[j]);
            }
            else
            {
                cprintf("  %s  ",menuItems[j]);
            }
        }

        // Look for user input
        keyMask = plat_ReadKeys(1);

        if(keyMask & INPUT_MOTION)
        {
            switch(keyMask & INPUT_MOTION)
            {
                case INPUT_LEFT:
                    if(!--i)
                        i = j-1;
                break;

                case INPUT_RIGHT:
                    if(j == ++i)
                        i = 1;
                break;

                case INPUT_UP:
                    if(i > 2)
                        i -= 2;
                    else
                        i = j-1-(i-1)%2;
                break;

                case INPUT_DOWN:
                    if(i < j-2)
                        i += 2;
                    else
                        i = 1+(i-1)%2;
                break;
            }
        }
        keyMask &= (INPUT_SELECT | INPUT_BACKUP);

    } while(keyMask != INPUT_SELECT && keyMask != INPUT_BACKUP);

    hires_Text(0x80);

    // If backing out of the menu, return 0
    if(keyMask & INPUT_BACKUP)
        return 0;

    // Return the selection
    return i;
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_DrawBoard(char clearLog)
{
    char i;

    // Add the A..H and 1..8 tile-keys
    for(i=0; i<8; ++i)
    {
        plat_Drawchar(3+i*BOARD_PIECE_WIDTH,0,                ROP_CPY,i+'A');
        plat_Drawchar(0,SCREEN_HEIGHT-17-i*BOARD_PIECE_HEIGHT,ROP_CPY,i+'1');
    }

    if(clearLog)
        hires_Mask(27,0,SCREEN_WIDTH-27,SCREEN_HEIGHT,ROP_BLACK);

    // Redraw all tiles
    for(i=0; i<64; ++i)
        plat_DrawSquare(i);
}

/*-----------------------------------------------------------------------*/
// Draw a tile with background and piece on it for positions 0..63
void plat_DrawSquare(char position)
{
    unsigned rop;
    char inv;
    char y = position / 8, x = position & 7;
    char piece = gChessBoard[y][x];
    char blackWhite = !((x & 1) ^ (y & 1));

    if(piece)
    {
        rop = blackWhite ? ROP_INV : ROP_CPY;
        inv = blackWhite ^ (piece & PIECE_WHITE) != 0;
    }
    else
        rop = blackWhite ? ROP_WHITE : ROP_BLACK;

    hires_Draw(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT,
               BOARD_PIECE_WIDTH,BOARD_PIECE_HEIGHT,rop,
               hires_Pieces[(piece&PIECE_DATA)-1][inv]);

    // Show the attack numbers
    if(gShowAttackBoard)
    {
        char val[4];

        rop = blackWhite ? ROP_INV : ROP_CPY;
        sprintf(val,"%02X%d",gChessBoard[y][x],(gChessBoard[y][x]&PIECE_WHITE)>>7);

        plat_Drawchar(2+x*BOARD_PIECE_WIDTH,14+(y+1)*BOARD_PIECE_HEIGHT-CHAR_HEIGHT,
                      rop,(gpAttackBoard[giAttackBoardOffset[position][0]])+'0');
        plat_Drawchar(2+(x+1)*BOARD_PIECE_WIDTH-1,14+(y+1)*BOARD_PIECE_HEIGHT-CHAR_HEIGHT,
                      rop,(gpAttackBoard[giAttackBoardOffset[position][1]])+'0');
        plat_Drawstring(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT,rop,val);
    }
}

/*-----------------------------------------------------------------------*/
void plat_ShowSideToGoLabel(char side)
{
    plat_Drawstring(28,SCREEN_HEIGHT-CHAR_HEIGHT,
                    side?ROP_CPY:ROP_INV,gszSideLabel[side]);
}

/*-----------------------------------------------------------------------*/
void plat_Highlight(char position, char color, char cursor)
{
    char y = position / 8, x = position & 7;

    if (cursor && color != HCOLOR_SELECTED)
    {
        char size = color == HCOLOR_EMPTY ? 6 : color == HCOLOR_INVALID ? 4 : 2;

        hires_Mask(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT+size,
                   1,BOARD_PIECE_HEIGHT-2*size,ROP_XOR(rop_Line[0][size]));
        hires_Mask(2+x*BOARD_PIECE_WIDTH+BOARD_PIECE_WIDTH-1,14+y*BOARD_PIECE_HEIGHT+size,
                   1,BOARD_PIECE_HEIGHT-2*size,ROP_XOR(rop_Line[1][size]));
        hires_Mask(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT,
                   BOARD_PIECE_WIDTH,size,ROP_XOR(0x7F));
        hires_Mask(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT+BOARD_PIECE_HEIGHT-size,
                   BOARD_PIECE_WIDTH,size,ROP_XOR(0x7F));
    }
    else
    {
        char set = color == HCOLOR_ATTACK;
        char val = cursor ^ x * BOARD_PIECE_WIDTH & 1;

        hires_Mask(2+x*BOARD_PIECE_WIDTH,14+y*BOARD_PIECE_HEIGHT,
                   1,BOARD_PIECE_HEIGHT,ROP_AND(rop_Color[set][!val]));
        hires_Mask(2+x*BOARD_PIECE_WIDTH+1,14+y*BOARD_PIECE_HEIGHT,
                   1,BOARD_PIECE_HEIGHT,ROP_AND(rop_Color[set][val]));
        hires_Mask(2+x*BOARD_PIECE_WIDTH+2,14+y*BOARD_PIECE_HEIGHT,
                   1,BOARD_PIECE_HEIGHT,ROP_AND(rop_Color[set][!val]));
    }
}

/*-----------------------------------------------------------------------*/
void plat_ShowMessage(char *str, char)
{
    plat_Drawstring(26,0,ROP_CPY,str);
}

/*-----------------------------------------------------------------------*/
void plat_ClearMessage()
{
    hires_Mask(26,0,7,CHAR_HEIGHT,ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_AddToLogWin()
{
    char y;

    for(y=0; y<=LOG_WINDOW_HEIGHT; ++y)
    {
        hires_Mask(SCREEN_WIDTH-6,y*CHAR_HEIGHT,
                   6,CHAR_HEIGHT,ROP_BLACK);

        if(undo_FindUndoLine(LOG_WINDOW_HEIGHT-y))
        {
            frontend_FormatLogString();
            plat_Drawstring(SCREEN_WIDTH-6,y*CHAR_HEIGHT,
                            gColor[0]?ROP_CPY:ROP_INV,gLogStrBuffer);
        }
    }
}

/*-----------------------------------------------------------------------*/
void plat_AddToLogWinTop()
{
    plat_AddToLogWin();
}

/*-----------------------------------------------------------------------*/
int plat_ReadKeys(char blocking)
{
    char key = 0;
    int keyMask = 0;
    
    __asm__ ("lda #$00"); 
    __asm__ ("sta $D2"); 

    if(blocking || kbhit())
        key = cgetc();
    else
        return 0;

    switch(key)
    {
        case 'A':
            keyMask |= INPUT_LEFT;
        break;

        case 'D':
            keyMask |= INPUT_RIGHT;
        break;

        case 'W':
            keyMask |= INPUT_UP;
        break;

        case 'S':
            keyMask |= INPUT_DOWN;
        break;

        case '\x1b':
            keyMask |= INPUT_BACKUP;
        break;

        case '\r':
            keyMask |= INPUT_SELECT;
        break;

        case 'Q':
            keyMask |= INPUT_TOGGLE_A;
        break;

        case 'E':
            keyMask |= INPUT_TOGGLE_B;
        break;

        case 'Z':
            keyMask |= INPUT_TOGGLE_D;
        break;

        case 'M':
            keyMask |= INPUT_MENU;
        break;

        case 'R':
            keyMask |= INPUT_REDO;
        break;

        case 'U':
            keyMask |= INPUT_UNDO;
        break;
    }

    return keyMask;
}

/*-----------------------------------------------------------------------*/
// Only ever gets called if gReturnToOS is true
void plat_Shutdown()
{
    hires_Done();
}
