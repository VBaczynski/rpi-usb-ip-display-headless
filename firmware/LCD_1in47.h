/*****************************************************************************
* | File      	:   LCD_1IN28.h
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master 
*                and enhance portability
*----------------
* |	This version:   V1.0
* | Date        :   2020-12-16
* | Info        :   Basic version
*
******************************************************************************/
#ifndef __LCD_1IN47_H
#define __LCD_1IN47_H	
	
#include "DEV_Config.h"
#include <stdint.h>

#include <stdlib.h>     //itoa()
#include <stdio.h>

#define LCD_1IN47_HEIGHT 172
#define LCD_1IN47_WIDTH 320

#define HORIZONTAL 0
#define VERTICAL   1

#define LCD_1IN47_SetBacklight(Value) ; 

typedef struct{
    UWORD WIDTH;
    UWORD HEIGHT;
    UBYTE SCAN_DIR;
}LCD_1IN47_ATTRIBUTES;
extern LCD_1IN47_ATTRIBUTES LCD_1IN47;

/********************************************************************************
function:	
			Macro definition variable name
********************************************************************************/
void LCD_1IN47_Init(UBYTE Scan_dir);
void LCD_1IN47_Clear(UWORD Color);
void LCD_1IN47_Display(UWORD *Image);
void LCD_1IN47_DisplayWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD *Image);
void LCD_1IN47_DisplayPoint(UWORD X, UWORD Y, UWORD Color);
void Handler_1IN47_LCD(int signo);
#endif
