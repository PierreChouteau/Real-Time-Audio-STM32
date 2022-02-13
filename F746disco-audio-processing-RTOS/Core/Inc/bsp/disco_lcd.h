/*
 * disco_lcd.h
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */

#ifndef INC_DISCO_LCD_H_
#define INC_DISCO_LCD_H_

#ifdef __cplusplus
 extern "C" {
#endif


#include "disco_sdram.h"
#include "fonts.h"
#include "types.h"

 // ======================================= IMPORTANT CONFIG DEFINES ========================

/* uncomment to put FB in SDRAM, else it will be in RAM (in which case 565 pixel format is mandatory else FB doesn't fit in RAM) */
//#define FB_IN_SDRAM

 /* uncomment if FB's pixel format is 565, leave commented for ARGB888 format */
#define PF_565

// ==========================================================================================

#ifdef PF_565
	#define __GetAddress(X,Y) ((hltdc.LayerCfg[0].FBStartAdress)+2*(LCD_SCREEN_WIDTH*Y+X))
	#define __DrawPixel(X, Y, C) *(__IO uint16_t*)(__GetAddress(X, Y))=C
	#define STROKE_COLOR StrokeColor565
	#define FILL_COLOR FillColor565
	#define BACK_COLOR BackColor565
#else
	#define __GetAddress(X,Y) ((hltdc.LayerCfg[0].FBStartAdress)+4*(LCD_SCREEN_WIDTH*Y+X))
	#define __DrawPixel(X, Y, C) *(__IO uint32_t*)(__GetAddress(X, Y))=C
	#define STROKE_COLOR StrokeColor
	#define FILL_COLOR FillColor
	#define BACK_COLOR BackColor
#endif

typedef struct
{
  int16_t X;
  int16_t Y;
} Point, * pPoint;

typedef enum
{
  CENTER_MODE             = 0x01,    /* Center mode */
  RIGHT_MODE              = 0x02,    /* Right mode  */
  LEFT_MODE               = 0x03     /* Left mode   */
}Text_AlignModeTypdef;

#define LCD_OK                 ((uint8_t)0x00)
#define LCD_ERROR              ((uint8_t)0x01)
#define LCD_TIMEOUT            ((uint8_t)0x02)

/**
  * LCD colors in ARGB format
  */
#define LCD_COLOR_BLUE          ((uint32_t)0xFF0000FF)
#define LCD_COLOR_GREEN         ((uint32_t)0xFF00FF00)
#define LCD_COLOR_RED           ((uint32_t)0xFFFF0000)
#define LCD_COLOR_CYAN          ((uint32_t)0xFF00FFFF)
#define LCD_COLOR_MAGENTA       ((uint32_t)0xFFFF00FF)
#define LCD_COLOR_YELLOW        ((uint32_t)0xFFFFFF00)
#define LCD_COLOR_LIGHTBLUE     ((uint32_t)0xFF8080FF)
#define LCD_COLOR_LIGHTGREEN    ((uint32_t)0xFF80FF80)
#define LCD_COLOR_LIGHTRED      ((uint32_t)0xFFFF8080)
#define LCD_COLOR_LIGHTCYAN     ((uint32_t)0xFF80FFFF)
#define LCD_COLOR_LIGHTMAGENTA  ((uint32_t)0xFFFF80FF)
#define LCD_COLOR_LIGHTYELLOW   ((uint32_t)0xFFFFFF80)
#define LCD_COLOR_DARKBLUE      ((uint32_t)0xFF000080)
#define LCD_COLOR_DARKGREEN     ((uint32_t)0xFF008000)
#define LCD_COLOR_DARKRED       ((uint32_t)0xFF800000)
#define LCD_COLOR_DARKCYAN      ((uint32_t)0xFF008080)
#define LCD_COLOR_DARKMAGENTA   ((uint32_t)0xFF800080)
#define LCD_COLOR_DARKYELLOW    ((uint32_t)0xFF808000)
#define LCD_COLOR_WHITE         ((uint32_t)0xFFFFFFFF)
#define LCD_COLOR_LIGHTGRAY     ((uint32_t)0xFFD3D3D3)
#define LCD_COLOR_GRAY          ((uint32_t)0xFF808080)
#define LCD_COLOR_DARKGRAY      ((uint32_t)0xFF404040)
#define LCD_COLOR_BLACK         ((uint32_t)0xFF000000)
#define LCD_COLOR_BROWN         ((uint32_t)0xFFA52A2A)
#define LCD_COLOR_ORANGE        ((uint32_t)0xFFFFA500)
#define LCD_COLOR_TRANSPARENT   ((uint32_t)0xFF000000)


#define LCD_DEFAULT_FONT        Font24


void	 LCD_Init(void);

/* Functions using the LTDC controller */
void     LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency);
void     LCD_SetTransparency_NoReload(uint32_t LayerIndex, uint8_t Transparency);
void     LCD_SetColorKeying(uint32_t LayerIndex, uint32_t RGBValue);
void     LCD_SetColorKeying_NoReload(uint32_t LayerIndex, uint32_t RGBValue);
void     LCD_ResetColorKeying(uint32_t LayerIndex);
void     LCD_ResetColorKeying_NoReload(uint32_t LayerIndex);
void     LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     LCD_SetLayerWindow_NoReload(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     LCD_Reload(uint32_t ReloadType);

void     LCD_SetStrokeColor(uint32_t Color);
void     LCD_SetBackColor(uint32_t Color);
void     LCD_SetFillColor(uint32_t Color);
void     LCD_SetFont(sFONT *fonts);

uint32_t LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos);
void     LCD_DrawPixel_Color(uint16_t Xpos, uint16_t Ypos, uint16_t STOKE_COLOR);
void     LCD_FillPixel(uint16_t Xpos, uint16_t Ypos);
void     LCD_ErasePixel(uint16_t Xpos, uint16_t Ypos);
void     LCD_Clear();
void     LCD_ClearStringAtLine(uint32_t Line);
void     LCD_DrawStringAtLine(uint16_t Line, uint8_t *ptr, const boolean_t isOpaqueBackground);
void     LCD_DrawString(uint16_t Xpos, uint16_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode, const boolean_t isOpaqueBackground);
void     LCD_DrawChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii, const boolean_t isOpaqueBackground);

void     LCD_DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     LCD_EraseHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     LCD_DrawVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     LCD_EraseVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void     LCD_EraseLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void     LCD_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     LCD_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);
void     LCD_DrawPolygon(pPoint Points, uint16_t PointCount);
void     LCD_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius);
void     LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp);

void     LCD_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     LCD_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);
void     LCD_FillPolygon(pPoint Points, uint16_t PointCount);
void     LCD_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius);

void     LCD_DisplayOff(void);
void     LCD_DisplayOn(void);

#ifdef __cplusplus
}
#endif


#endif /* INC_DISCO_LCD_H_ */
