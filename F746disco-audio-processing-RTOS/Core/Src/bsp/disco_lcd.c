/*
 * disco_lcd.c RK043FN48H driver
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */




/* Includes ------------------------------------------------------------------*/
#include "bsp/disco_lcd.h"
#include "bsp/disco_base.h"
#include "main.h"
#include "fonts.h"
#include "types.h"
#include "main.h"
#include "stdio.h"

#define POLY_X(Z)              ((int32_t)((Points + Z)->X))
#define POLY_Y(Z)              ((int32_t)((Points + Z)->Y))

#define ABS(X)  ((X) > 0 ? (X) : -(X))

extern LTDC_HandleTypeDef  hltdc;
extern DMA2D_HandleTypeDef hdma2d;

// if the LCD framebuffer is in RAM (as opposed to external SDRAM), frameBuf[] holds it.
// In this case it takes up around 255kb of the available 320ko of RAM, beware!
#ifndef FB_IN_SDRAM
	uint16_t frameBuf[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT];
	//ALIGN_32BYTES (static uint8_t frameBuf[2*LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT]);
	static uint32_t frameBuf0 = (uint32_t)& frameBuf[0];
	//#define __CleanDCache() SCB_CleanDCache_by_Addr((uint32_t*)frameBuf0, LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT*2) // SO SLOW...
	//#define __CleanDCache() SCB_CleanDCache() // we no longer need it if we're using the MPU to disable
	#define __CleanDCache()
#else
	#define __CleanDCache()
#endif




static uint32_t StrokeColor;
static uint32_t FillColor;
static uint32_t BackColor;
#ifdef PF_565
static uint32_t StrokeColor565; // shadow copy of FrontColor in 565 pixel format for better performance
static uint32_t FillColor565; // ibid
static uint32_t BackColor565; // ibid
#endif
static sFONT    *pFont;

static void DrawChar(uint16_t Xpos, uint16_t Ypos, const uint8_t *c, const boolean_t isOpaqueBackground);
#ifdef PF_565
static void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
#else
static void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
#endif
static void FillTriangle(uint16_t x1, uint16_t x2, uint16_t x3, uint16_t y1, uint16_t y2, uint16_t y3);
static void LL_FillBuffer(void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t withColor);
static void LL_ConvertLineToARGB8888(void * pSrc, void *pDst, uint32_t xSize, uint32_t InputColorMode);
static uint16_t ARGB888ToRGB565(uint32_t RGB_Code);
static void DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint32_t Color8888);


// ============================================= on/off =============================================

/**
 * @brief  Initializes the LCD.
 * @retval LCD state
 */
void LCD_Init(void){

	/* check if FB has a pixel format coherent with PF_565 define */
#ifdef PF_565
	if (hltdc.LayerCfg[0].PixelFormat != LTDC_PIXEL_FORMAT_RGB565) Error("Pixel Format Error: should be RGB565, check MX_LTDC_Init() or undefine PF_565");
#else
	if (hltdc.LayerCfg[0].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) Error("Pixel Format Error: should be RGB888, check MX_LTDC_Init() or define PF_565");
#endif

#ifndef FB_IN_SDRAM
	HAL_LTDC_SetAddress(&hltdc, frameBuf0, 0);
#endif

	LCD_DisplayOn();

	LCD_SetFont(&LCD_DEFAULT_FONT);

	LCD_SetBackColor(LCD_COLOR_WHITE);
	LCD_SetStrokeColor(LCD_COLOR_BLACK);
	LCD_SetFillColor(LCD_COLOR_RED);


}

/**
 * @brief  Enables the display.
 * @retval None
 */
void LCD_DisplayOn(void)
{
	__HAL_LTDC_ENABLE(&hltdc);
	HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);        /* Assert LCD_DISP pin */
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);  /* Assert LCD_BL_CTRL pin */
}


/**
 * @brief  Disables the display.
 * @retval None
 */
void LCD_DisplayOff(void)
{
	__HAL_LTDC_DISABLE(&hltdc);
	HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_RESET);      /* De-assert LCD_DISP pin */
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);/* De-assert LCD_BL_CTRL pin */
}

/**
 * @brief  Configures the transparency.
 * @param  LayerIndex: Layer foreground or background.
 * @param  Transparency: Transparency
 *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
void LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency)
{
	HAL_LTDC_SetAlpha(&hltdc, Transparency, LayerIndex);
}

/**
 * @brief  Configures the transparency without reloading.
 * @param  LayerIndex: Layer foreground or background.
 * @param  Transparency: Transparency
 *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
void LCD_SetTransparency_NoReload(uint32_t LayerIndex, uint8_t Transparency)
{
	HAL_LTDC_SetAlpha_NoReload(&hltdc, Transparency, LayerIndex);
}


/**
 * @brief  Sets display window.
 * @param  LayerIndex: Layer index
 * @param  Xpos: LCD X position
 * @param  Ypos: LCD Y position
 * @param  Width: LCD window width
 * @param  Height: LCD window height
 * @retval None
 */
void LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
	/* Reconfigure the layer size */
	HAL_LTDC_SetWindowSize(&hltdc, Width, Height, LayerIndex);

	/* Reconfigure the layer position */
	HAL_LTDC_SetWindowPosition(&hltdc, Xpos, Ypos, LayerIndex);
}

/**
 * @brief  Sets display window without reloading.
 * @param  LayerIndex: Layer index
 * @param  Xpos: LCD X position
 * @param  Ypos: LCD Y position
 * @param  Width: LCD window width
 * @param  Height: LCD window height
 * @retval None
 */
void LCD_SetLayerWindow_NoReload(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
	/* Reconfigure the layer size */
	HAL_LTDC_SetWindowSize_NoReload(&hltdc, Width, Height, LayerIndex);

	/* Reconfigure the layer position */
	HAL_LTDC_SetWindowPosition_NoReload(&hltdc, Xpos, Ypos, LayerIndex);
}

// ================================================ Color keying ================================================

/**
 * @brief  Configures and sets the color keying.
 * @param  LayerIndex: Layer foreground or background
 * @param  RGBValue: Color reference
 * @retval None
 */
void LCD_SetColorKeying(uint32_t LayerIndex, uint32_t RGBValue)
{
	/* Configure and Enable the color Keying for LCD Layer */
	HAL_LTDC_ConfigColorKeying(&hltdc, RGBValue, LayerIndex);
	HAL_LTDC_EnableColorKeying(&hltdc, LayerIndex);
}

/**
 * @brief  Configures and sets the color keying without reloading.
 * @param  LayerIndex: Layer foreground or background
 * @param  RGBValue: Color reference
 * @retval None
 */
void LCD_SetColorKeying_NoReload(uint32_t LayerIndex, uint32_t RGBValue)
{
	/* Configure and Enable the color Keying for LCD Layer */
	HAL_LTDC_ConfigColorKeying_NoReload(&hltdc, RGBValue, LayerIndex);
	HAL_LTDC_EnableColorKeying_NoReload(&hltdc, LayerIndex);
}

/**
 * @brief  Disables the color keying.
 * @param  LayerIndex: Layer foreground or background
 * @retval None
 */
void LCD_ResetColorKeying(uint32_t LayerIndex)
{
	/* Disable the color Keying for LCD Layer */
	HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);
}

/**
 * @brief  Disables the color keying without reloading.
 * @param  LayerIndex: Layer foreground or background
 * @retval None
 */
void LCD_ResetColorKeying_NoReload(uint32_t LayerIndex)
{
	/* Disable the color Keying for LCD Layer */
	HAL_LTDC_DisableColorKeying_NoReload(&hltdc, LayerIndex);
}

/**
 * @brief  Disables the color keying without reloading.
 * @param  ReloadType: can be one of the following values
 *         - LCD_RELOAD_IMMEDIATE
 *         - LCD_RELOAD_VERTICAL_BLANKING
 * @retval None
 */
void LCD_Reload(uint32_t ReloadType)
{
	HAL_LTDC_Reload (&hltdc, ReloadType);
}

// ================================================ Colors ================================================

/**
 * @brief  Sets the LCD stroke (and text) color.
 * @param  Color: Text color code ARGB(8-8-8-8)
 */
void LCD_SetStrokeColor(uint32_t Color8888)
{
	StrokeColor = Color8888;
#ifdef PF_565
	StrokeColor565 = ARGB888ToRGB565(StrokeColor);
#endif
}

/**
 * @brief  Sets the LCD fill color.
 * @param  Color: Text color code ARGB(8-8-8-8)
 */
void LCD_SetFillColor(uint32_t Color8888)
{
	FillColor = Color8888;
#ifdef PF_565
	FillColor565 = ARGB888ToRGB565(FillColor);
#endif
}

/**
 * @brief  Sets the LCD background color.
 * @param  Color: Layer background color code ARGB(8-8-8-8)
 * @retval None
 */
void LCD_SetBackColor(uint32_t Color)
{
	BackColor = Color;
#ifdef PF_565
	BackColor565 = ARGB888ToRGB565(BackColor);
#endif
}

// ================================================ Fonts ================================================

/**
 * @brief  Sets the LCD text font.
 * @param  fonts: Layer font to be used
 * @retval None
 */
void LCD_SetFont(sFONT *fonts)
{
	pFont = fonts;
}


// ================================================ Drawing ================================================

/**
 * @brief  Reads an LCD pixel. (pas utilisé)
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @retval RGB pixel color
 */
uint32_t LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
#ifdef PF_565
	/* Read data value from SDRAM memory */
	return *(__IO uint16_t*) (__GetAddress(Xpos, Ypos));
#else
	/* Read data value from SDRAM memory */
	return *(__IO uint32_t*) (__GetAddress(Xpos, Ypos));
#endif
}

/**
 * @brief  Draws a pixel using current stroke color.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @retval None
 */
void LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos)
{
	__DrawPixel(Xpos, Ypos, STROKE_COLOR);
	__CleanDCache();

}

/**
 * @brief  Draws a pixel using a parametrable color.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  color: Color of the pixel
 * @retval None
 */
void LCD_DrawPixel_Color(uint16_t Xpos, uint16_t Ypos, uint16_t color)
{
	__DrawPixel(Xpos, Ypos, color);
	__CleanDCache();

}

/**
 * @brief  Draws a pixel using current fill color.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @retval None
 */
void LCD_FillPixel(uint16_t Xpos, uint16_t Ypos)
{
	__DrawPixel(Xpos, Ypos, FILL_COLOR);
	__CleanDCache();
}

/**
 * @brief  Erase a pixel, ie, draws a pixel using the current backcolor.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @retval None
 */
void LCD_ErasePixel(uint16_t Xpos, uint16_t Ypos)
{
	__DrawPixel(Xpos, Ypos, BACK_COLOR);
	__CleanDCache();
}

/**
 * @brief  Clears the whole LCD, ie fills with background color
 * @retval None
 */
void LCD_Clear()
{
	/* Clear the LCD */
	LL_FillBuffer((uint32_t *)(hltdc.LayerCfg[0].FBStartAdress), LCD_SCREEN_WIDTH, LCD_SCREEN_HEIGHT, 0, BackColor);
}

/**
 * @brief  Clears the selected line.
 * @param  Line: Line to be cleared
 * @retval None
 */
void LCD_ClearStringAtLine(uint32_t Line)
{
	uint32_t color_backup = StrokeColor;
	FillColor = BackColor;

	/* Draw rectangle with background color */
	LCD_FillRect(0, (Line * pFont->Height), LCD_SCREEN_WIDTH, pFont->Height);

	FillColor = color_backup;
}

/**
 * @brief  Displays one character.
 * @param  Xpos: Start column address
 * @param  Ypos: Line where to display the character shape.
 * @param  Ascii: Character ascii code
 *           This parameter must be a number between Min_Data = 0x20 and Max_Data = 0x7E
 * @retval None
 */
void LCD_DrawChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii, const boolean_t isOpaqueBackground)
{
	DrawChar(Xpos, Ypos, &pFont->table[(Ascii-' ') * pFont->Height * ((pFont->Width + 7) / 8)], isOpaqueBackground);

}

/**
 * @brief  Displays characters on the LCD.
 * @param  Xpos: X position (in pixel)
 * @param  Ypos: Y position (in pixel)
 * @param  Text: Pointer to string to display on LCD
 * @param  Mode: Display mode
 *          This parameter can be one of the following values:
 *            @arg  CENTER_MODE
 *            @arg  RIGHT_MODE
 *            @arg  LEFT_MODE
 * @retval None
 */
void LCD_DrawString(uint16_t Xpos, uint16_t Ypos, uint8_t *Text, Text_AlignModeTypdef Alignment, const boolean_t isOpaqueBackground)
{
	uint16_t ref_column = 1, i = 0;
	uint32_t size = 0, xsize = 0;
	uint8_t  *ptr = Text;

	/* Get the text size */
	while (*ptr++) size ++ ;

	/* Characters number per line */
	xsize = (LCD_SCREEN_WIDTH / pFont->Width);

	switch (Alignment)
	{
	case CENTER_MODE:
	{
		ref_column = Xpos + ((xsize - size)* pFont->Width) / 2;
		break;
	}
	case LEFT_MODE:
	{
		ref_column = Xpos;
		break;
	}
	case RIGHT_MODE:
	{
		ref_column = - Xpos + ((xsize - size)*pFont->Width);
		break;
	}
	default:
	{
		ref_column = Xpos;
		break;
	}
	}

	/* Check that the Start column is located in the screen */
	if ((ref_column < 1) || (ref_column >= 0x8000))
	{
		ref_column = 1;
	}

	/* Send the string character by character on LCD */
	while ((*Text != 0) & (((LCD_SCREEN_WIDTH - (i*pFont->Width)) & 0xFFFF) >= pFont->Width))
	{
		/* Display one character on LCD */
		LCD_DrawChar(ref_column, Ypos, *Text, isOpaqueBackground);
		/* Decrement the column position by 16 */
		ref_column += pFont->Width;
		/* Point on the next character */
		Text++;
		i++;
	}
}

/**
 * @brief  Displays a maximum of 60 characters on the LCD.
 * @param  Line: Line where to display the character shape
 * @param  ptr: Pointer to string to display on LCD
 * @retval None
 */
void LCD_DrawStringAtLine(uint16_t Line, uint8_t *ptr, const boolean_t isOpaqueBackground)
{
	LCD_DrawString(0, LINE(Line), ptr, LEFT_MODE, isOpaqueBackground);
}

/**
 * @brief  Draws an horizontal line using the stroke color
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Length: Line length
 * @retval None
 */
void LCD_DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), Length, 1, 0, StrokeColor);
	__CleanDCache();
}

void LCD_EraseHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), Length, 1, 0, BackColor);
	__CleanDCache();
}

/**
 * @param color stroke color in ARGB8888 format (even if FB may use RGB565)
 */
static void DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint32_t Color8888)
{
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), Length, 1, 0, Color8888);
}

/**
 * @brief  Draws a vertical line using the stroke color
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Length: Line length
 * @retval None
 */
void LCD_DrawVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), 1, Length, (LCD_SCREEN_WIDTH - 1), StrokeColor);
	__CleanDCache();
}

void LCD_EraseVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), 1, Length, (LCD_SCREEN_WIDTH - 1), BackColor);
	__CleanDCache();
}


/**
 * @brief Draw a line using stroke color
 * @param  x1: Point 1 X position
 * @param  y1: Point 1 Y position
 * @param  x2: Point 2 X position
 * @param  y2: Point 2 Y position
 * @retval None
 */
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
	DrawLine(x1, y1, x2, y2, STROKE_COLOR);
	__CleanDCache();
}

void LCD_EraseLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
	DrawLine(x1, y1, x2, y2, BACK_COLOR);
	__CleanDCache();
}


/**
 * Utility that draws a line using the given color
 * @param color must be a color given in the current FB pixel format
 */
#ifdef PF_565
static void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
#else
static void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
#endif
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, num_add = 0, num_pixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);        /* The difference between the x's */
	deltay = ABS(y2 - y1);        /* The difference between the y's */
	x = x1;                       /* Start x off at the first pixel */
	y = y1;                       /* Start y off at the first pixel */

	if (x2 >= x1)                 /* The x-values are increasing */
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else                          /* The x-values are decreasing */
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)                 /* The y-values are increasing */
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else                          /* The y-values are decreasing */
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)         /* There is at least one x-value for every y-value */
	{
		xinc1 = 0;                  /* Don't change the x when numerator >= denominator */
		yinc2 = 0;                  /* Don't change the y for every iteration */
		den = deltax;
		num = deltax / 2;
		num_add = deltay;
		num_pixels = deltax;         /* There are more x-values than y-values */
	}
	else                          /* There is at least one y-value for every x-value */
	{
		xinc2 = 0;                  /* Don't change the x for every iteration */
		yinc1 = 0;                  /* Don't change the y when numerator >= denominator */
		den = deltay;
		num = deltay / 2;
		num_add = deltax;
		num_pixels = deltay;         /* There are more y-values than x-values */
	}

	for (curpixel = 0; curpixel <= num_pixels; curpixel++)
	{
		__DrawPixel(x, y, color);   /* Draw the current pixel */
		num += num_add;                            /* Increase the numerator by the top of the fraction */
		if (num >= den)                           /* Check if numerator >= denominator */
		{
			num -= den;                             /* Calculate the new numerator value */
			x += xinc1;                             /* Change the x as appropriate */
			y += yinc1;                             /* Change the y as appropriate */
		}
		x += xinc2;                               /* Change the x as appropriate */
		y += yinc2;                               /* Change the y as appropriate */
	}
}

/**
 * @brief  Draws a rectangle.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Width: Rectangle width
 * @param  Height: Rectangle height
 * @retval None
 */
void LCD_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{

	/* Draw horizontal lines */
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), Width, 1, 0, StrokeColor);
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, (Ypos+Height)), Width, 1, 0, StrokeColor);

	/* Draw vertical lines */
	LL_FillBuffer((uint32_t *)__GetAddress(Xpos, Ypos), 1, Height, (LCD_SCREEN_WIDTH - 1), StrokeColor);
	LL_FillBuffer((uint32_t *)__GetAddress((Xpos+Width), Ypos), 1, Height, (LCD_SCREEN_WIDTH - 1), StrokeColor);

	__CleanDCache();
}

/**
 * @brief  Draws a circle.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Radius: Circle radius
 * @retval None
 */
void LCD_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius)
{
	int32_t   decision;    /* Decision Variable */
	uint32_t  current_x;   /* Current X Value */
	uint32_t  current_y;   /* Current Y Value */

	decision = 3 - (Radius << 1);
	current_x = 0;
	current_y = Radius;

	while (current_x <= current_y)
	{
		__DrawPixel((Xpos + current_x), (Ypos - current_y), STROKE_COLOR);

		__DrawPixel((Xpos - current_x), (Ypos - current_y), STROKE_COLOR);

		__DrawPixel((Xpos + current_y), (Ypos - current_x), STROKE_COLOR);

		__DrawPixel((Xpos - current_y), (Ypos - current_x), STROKE_COLOR);

		__DrawPixel((Xpos + current_x), (Ypos + current_y), STROKE_COLOR);

		__DrawPixel((Xpos - current_x), (Ypos + current_y), STROKE_COLOR);

		__DrawPixel((Xpos + current_y), (Ypos + current_x), STROKE_COLOR);

		__DrawPixel((Xpos - current_y), (Ypos + current_x), STROKE_COLOR);

		if (decision < 0)
		{
			decision += (current_x << 2) + 6;
		}
		else
		{
			decision += ((current_x - current_y) << 2) + 10;
			current_y--;
		}
		current_x++;
	}
	__CleanDCache();
}

/**
 * @brief  Draws a polygon using stroke color
 * @param  Points: Pointer to the points array
 * @param  PointCount: Number of points
 * @retval None
 */
void LCD_DrawPolygon(pPoint Points, uint16_t PointCount)
{
	int16_t x = 0, y = 0;

	if(PointCount < 2)
	{
		return;
	}

	DrawLine(Points->X, Points->Y, (Points+PointCount-1)->X, (Points+PointCount-1)->Y, STROKE_COLOR);

	while(--PointCount)
	{
		x = Points->X;
		y = Points->Y;
		Points++;
		DrawLine(x, y, Points->X, Points->Y, STROKE_COLOR);
	}

	__CleanDCache();
}

/**
 * @brief  Draws an ellipse on LCD.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  XRadius: Ellipse X radius
 * @param  YRadius: Ellipse Y radius
 * @retval None
 */
void LCD_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius)
{
	int x = 0, y = -YRadius, err = 2-2*XRadius, e2;
	float k = 0, rad1 = 0, rad2 = 0;

	rad1 = XRadius;
	rad2 = YRadius;

	k = (float)(rad2/rad1);

	do {
		__DrawPixel((Xpos-(uint16_t)(x/k)), (Ypos+y), STROKE_COLOR);
		__DrawPixel((Xpos+(uint16_t)(x/k)), (Ypos+y), STROKE_COLOR);
		__DrawPixel((Xpos+(uint16_t)(x/k)), (Ypos-y), STROKE_COLOR);
		__DrawPixel((Xpos-(uint16_t)(x/k)), (Ypos-y), STROKE_COLOR);

		e2 = err;
		if (e2 <= x) {
			err += ++x*2+1;
			if (-y == x && e2 <= y) e2 = 0;
		}
		if (e2 > y) err += ++y*2+1;
	}
	while (y <= 0);

	__CleanDCache();
}


/**
 * @brief  Draws a bitmap picture loaded in the internal Flash in ARGB888 format (32 bits per pixel).
 * @param  Xpos: Bmp X position in the LCD
 * @param  Ypos: Bmp Y position in the LCD
 * @param  pbmp: Pointer to Bmp picture address in the internal Flash
 * @retval None
 *
 * Example of use : LCD_DrawBitmap(150, 65, (uint8_t *)stlogo);
 */
void LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp) // TODO check + DCache
{
	uint32_t index = 0, width = 0, height = 0, bit_pixel = 0;
	uint32_t address;
	uint32_t input_color_mode = 0;

	/* Get bitmap data address offset */
	index = pbmp[10] + (pbmp[11] << 8) + (pbmp[12] << 16)  + (pbmp[13] << 24);

	/* Read bitmap width */
	width = pbmp[18] + (pbmp[19] << 8) + (pbmp[20] << 16)  + (pbmp[21] << 24);

	/* Read bitmap height */
	height = pbmp[22] + (pbmp[23] << 8) + (pbmp[24] << 16)  + (pbmp[25] << 24);

	/* Read bit/pixel */
	bit_pixel = pbmp[28] + (pbmp[29] << 8);

	/* Set the address */
	//address = hltdc.LayerCfg[0].FBStartAdress + (((LCD_SCREEN_WIDTH*Ypos) + Xpos)*(4)); // TODO : PF_565 ???
	address = __GetAddress(Xpos,Ypos);

	/* Get the layer pixel format */
	if ((bit_pixel/8) == 4)
	{
		input_color_mode = CM_ARGB8888;
	}
	else if ((bit_pixel/8) == 2)
	{
		input_color_mode = CM_RGB565;
	}
	else
	{
		input_color_mode = CM_RGB888;
	}

	/* Bypass the bitmap header */
	pbmp += (index + (width * (height - 1) * (bit_pixel/8)));

	/* Convert picture to ARGB8888 pixel format */
	for(index=0; index < height; index++) // for every line...
	{
		/* Pixel format conversion */
		LL_ConvertLineToARGB8888((uint32_t *)pbmp, (uint32_t *)address, width, input_color_mode);

		/* Increment the source and destination buffers */
#ifdef PF_565
		address+=  (LCD_SCREEN_WIDTH*2);
#else
		address+=  (LCD_SCREEN_WIDTH*4);
#endif
		pbmp -= width*(bit_pixel/8);
	}
}

/**
 * @brief  Draws a full rectangle.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Width: Rectangle width
 * @param  Height: Rectangle height
 * @retval None
 */
void LCD_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
	uint32_t  x_address = __GetAddress(Xpos, Ypos);

	/* Fill the rectangle */
	LL_FillBuffer((uint32_t *)x_address, Width, Height, (LCD_SCREEN_WIDTH - Width), FillColor);
}


/**
 * @brief  Draws a full circle.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  Radius: Circle radius
 * @retval None
 */
void LCD_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius)
{
	int32_t  decision;     /* Decision Variable */
	uint32_t  current_x;   /* Current X Value */
	uint32_t  current_y;   /* Current Y Value */

	decision = 3 - (Radius << 1);

	current_x = 0;
	current_y = Radius;

	while (current_x <= current_y)
	{
		if(current_y > 0)
		{
			DrawHLine(Xpos - current_y, Ypos + current_x, 2*current_y, FillColor);
			DrawHLine(Xpos - current_y, Ypos - current_x, 2*current_y, FillColor);
		}

		if(current_x > 0)
		{
			DrawHLine(Xpos - current_x, Ypos - current_y, 2*current_x, FillColor);
			DrawHLine(Xpos - current_x, Ypos + current_y, 2*current_x, FillColor);
		}
		if (decision < 0)
		{
			decision += (current_x << 2) + 6;
		}
		else
		{
			decision += ((current_x - current_y) << 2) + 10;
			current_y--;
		}
		current_x++;
	}

	__CleanDCache();

}

/**
 * @brief  Draws a full poly-line (between many points).
 * @param  Points: Pointer to the points array
 * @param  PointCount: Number of points
 * @retval None
 */
void LCD_FillPolygon(pPoint Points, uint16_t PointCount)
{
	int16_t X = 0, Y = 0, X2 = 0, Y2 = 0, X_center = 0, Y_center = 0, X_first = 0, Y_first = 0, pixelX = 0, pixelY = 0, counter = 0;
	uint16_t  image_left = 0, image_right = 0, image_top = 0, image_bottom = 0;

	image_left = image_right = Points->X;
	image_top= image_bottom = Points->Y;

	for(counter = 1; counter < PointCount; counter++)
	{
		pixelX = POLY_X(counter);
		if(pixelX < image_left)
		{
			image_left = pixelX;
		}
		if(pixelX > image_right)
		{
			image_right = pixelX;
		}

		pixelY = POLY_Y(counter);
		if(pixelY < image_top)
		{
			image_top = pixelY;
		}
		if(pixelY > image_bottom)
		{
			image_bottom = pixelY;
		}
	}

	if(PointCount < 2)
	{
		return;
	}

	X_center = (image_left + image_right)/2;
	Y_center = (image_bottom + image_top)/2;

	X_first = Points->X;
	Y_first = Points->Y;

	while(--PointCount)
	{
		X = Points->X;
		Y = Points->Y;
		Points++;
		X2 = Points->X;
		Y2 = Points->Y;

		FillTriangle(X, X2, X_center, Y, Y2, Y_center);
		FillTriangle(X, X_center, X2, Y, Y_center, Y2);
		FillTriangle(X_center, X2, X, Y_center, Y2, Y);
	}

	FillTriangle(X_first, X2, X_center, Y_first, Y2, Y_center);
	FillTriangle(X_first, X_center, X2, Y_first, Y_center, Y2);
	FillTriangle(X_center, X2, X_first, Y_center, Y2, Y_first);

	__CleanDCache();
}

/**
 * @brief  Draws a full ellipse.
 * @param  Xpos: X position
 * @param  Ypos: Y position
 * @param  XRadius: Ellipse X radius
 * @param  YRadius: Ellipse Y radius
 * @retval None
 */
void LCD_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius) // TODO check
{
	int x = 0, y = -YRadius, err = 2-2*XRadius, e2;
	float k = 0, rad1 = 0, rad2 = 0;

	rad1 = XRadius;
	rad2 = YRadius;

	k = (float)(rad2/rad1);

	do
	{
		DrawHLine((Xpos-(uint16_t)(x/k)), (Ypos+y), (2*(uint16_t)(x/k) + 1), FillColor);
		DrawHLine((Xpos-(uint16_t)(x/k)), (Ypos-y), (2*(uint16_t)(x/k) + 1), FillColor);

		e2 = err;
		if (e2 <= x)
		{
			err += ++x*2+1;
			if (-y == x && e2 <= y) e2 = 0;
		}
		if (e2 > y) err += ++y*2+1;
	}
	while (y <= 0);

	__CleanDCache();
}



/*******************************************************************************
                            Static Functions
 *******************************************************************************/

/**
 * @brief  Draws a character on LCD.
 * @param  Xpos: Line where to display the character shape
 * @param  Ypos: Start column address
 * @param  c: Pointer to the character data
 * @retval None
 */
static void DrawChar(uint16_t Xpos, uint16_t Ypos, const uint8_t *c, const boolean_t isOpaqueBackground)
{
	uint32_t i = 0, j = 0;
	uint16_t height, width;
	uint8_t  offset;
	uint8_t  *pchar;
	uint32_t line;

	height = pFont->Height;
	width  = pFont->Width;

	offset =  8 *((width + 7)/8) -  width ;

	for(i = 0; i < height; i++)
	{
		pchar = ((uint8_t *)c + (width + 7)/8 * i);

		switch(((width + 7)/8))
		{

		case 1:
			line =  pchar[0];
			break;

		case 2:
			line =  (pchar[0]<< 8) | pchar[1];
			break;

		case 3:
		default:
			line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
			break;
		}

		for (j = 0; j < width; j++)
		{
			if(line & (1 << (width- j + offset- 1)))
			{
				__DrawPixel((Xpos + j), Ypos, STROKE_COLOR);
			}
			else
			{
				if (isOpaqueBackground == true) LCD_ErasePixel((Xpos + j), Ypos);
			}
		}
		Ypos++;
	}

	__CleanDCache();
}

/**
 * @brief  Fills a triangle (between 3 points).
 * @param  x1: Point 1 X position
 * @param  y1: Point 1 Y position
 * @param  x2: Point 2 X position
 * @param  y2: Point 2 Y position
 * @param  x3: Point 3 X position
 * @param  y3: Point 3 Y position
 * @retval None
 */
static void FillTriangle(uint16_t x1, uint16_t x2, uint16_t x3, uint16_t y1, uint16_t y2, uint16_t y3)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, num_add = 0, num_pixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);        /* The difference between the x's */
	deltay = ABS(y2 - y1);        /* The difference between the y's */
	x = x1;                       /* Start x off at the first pixel */
	y = y1;                       /* Start y off at the first pixel */

	if (x2 >= x1)                 /* The x-values are increasing */
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else                          /* The x-values are decreasing */
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)                 /* The y-values are increasing */
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else                          /* The y-values are decreasing */
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)         /* There is at least one x-value for every y-value */
	{
		xinc1 = 0;                  /* Don't change the x when numerator >= denominator */
		yinc2 = 0;                  /* Don't change the y for every iteration */
		den = deltax;
		num = deltax / 2;
		num_add = deltay;
		num_pixels = deltax;         /* There are more x-values than y-values */
	}
	else                          /* There is at least one y-value for every x-value */
	{
		xinc2 = 0;                  /* Don't change the x for every iteration */
		yinc1 = 0;                  /* Don't change the y when numerator >= denominator */
		den = deltay;
		num = deltay / 2;
		num_add = deltax;
		num_pixels = deltay;         /* There are more y-values than x-values */
	}

	for (curpixel = 0; curpixel <= num_pixels; curpixel++)
	{
		DrawLine(x, y, x3, y3, FILL_COLOR);

		num += num_add;              /* Increase the numerator by the top of the fraction */
		if (num >= den)             /* Check if numerator >= denominator */
		{
			num -= den;               /* Calculate the new numerator value */
			x += xinc1;               /* Change the x as appropriate */
			y += yinc1;               /* Change the y as appropriate */
		}
		x += xinc2;                 /* Change the x as appropriate */
		y += yinc2;                 /* Change the y as appropriate */
	}

	__CleanDCache();
}

// ================================================ DMA2D based functions  ================================================

/**
 * @brief  Fills a buffer.
 * @param  pDst: Pointer to destination buffer
 * @param  xSize: Buffer width
 * @param  ySize: Buffer height
 * @param  OffLine: Offset
 * @param  withColor: fill color in ARGB888 format (even if FB may use RGB565)
 * @retval None
 */
static void LL_FillBuffer(void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t withColor)
{
	/* Register to memory mode with ARGB8888 as color Mode */
	hdma2d.Init.Mode         = DMA2D_R2M;
#ifdef PF_565
	/* RGB565 "output" format */
	hdma2d.Init.ColorMode    = DMA2D_RGB565;
#else
	/* ARGB8888 "output" format */
	hdma2d.Init.ColorMode    = DMA2D_ARGB8888;
#endif
	hdma2d.Init.OutputOffset = OffLine;

	hdma2d.Instance = DMA2D;

	/* DMA2D Initialization */
	if(HAL_DMA2D_Init(&hdma2d) == HAL_OK)
	{
		if(HAL_DMA2D_ConfigLayer(&hdma2d, 0) == HAL_OK)
		{
			if (HAL_DMA2D_Start(&hdma2d, withColor, (uint32_t)pDst, xSize, ySize) == HAL_OK)
			{
				/* Polling For DMA transfer */
				HAL_DMA2D_PollForTransfer(&hdma2d, 10); // TODO : adapt to to CMSIS-RTOS
			}
		}
	}
}

/**
 * @brief  Copy a line to the FB in ARGB8888 pixel format. (utilisé uniquement par DrawBitMap)
 * @param  pSrc: Pointer to source buffer
 * @param  pDst: Output color
 * @param  xSize: Buffer width
 * @param  ColorMode: Input color mode
 * @retval None
 */
static void LL_ConvertLineToARGB8888(void *pSrc, void *pDst, uint32_t xSize, uint32_t InputColorMode) // TODO check
{
	/* Configure the DMA2D Mode, Color Mode and output offset */
	hdma2d.Init.Mode         = DMA2D_M2M_PFC;

	/* Framebuffer pixel format : */
#ifdef PF_565
	/* RGB565 "output" format */
	hdma2d.Init.ColorMode    = DMA2D_RGB565;
#else
	/* ARGB8888 "output" format */
	hdma2d.Init.InputColorMode    = DMA2D_ARGB8888;
#endif
	hdma2d.Init.OutputOffset = 0;

	/* Active Layer Configuration */
	hdma2d.LayerCfg[0].AlphaMode = DMA2D_NO_MODIF_ALPHA;
	hdma2d.LayerCfg[0].InputAlpha = 0xFF;
	hdma2d.LayerCfg[0].InputColorMode = InputColorMode;
	hdma2d.LayerCfg[0].InputOffset = 0;

	hdma2d.Instance = DMA2D;

	/* DMA2D Initialization */
	if(HAL_DMA2D_Init(&hdma2d) == HAL_OK)
	{
		if(HAL_DMA2D_ConfigLayer(&hdma2d, 1) == HAL_OK)
		{
			if (HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
			{
				/* Polling For DMA transfer */
				HAL_DMA2D_PollForTransfer(&hdma2d, 10);
			}
		}
	}
}

// =============================== utilities ====================================

/**
 * @param  RGB_Code: Pixel color in ARGB mode (8-8-8-8)
 */
static uint16_t ARGB888ToRGB565(uint32_t RGB_Code)
{
	uint8_t red   = (RGB_Code & 0x00FF0000) >> 16;
	uint8_t green = (RGB_Code & 0x0000FF00) >> 8;
	uint8_t blue  = (RGB_Code & 0x000000FF);

	uint16_t b = (blue >> 3) & 0x1f; // 5 bits
	uint16_t g = ((green >> 2) & 0x3f) << 5; // 6 bits
	uint16_t r = ((red >> 3) & 0x1f) << 11; // 5 bits

	return (uint16_t) (r | g | b);
}

















/* archives */



// ============================================= basic properties =============================================

/**
 * @brief  Gets the LCD X size.
 * @retval Used LCD X size
 */
/*uint32_t LCD_GetXSize(void)
{
  return hltdc.LayerCfg[0].ImageWidth;
}*/

/**
 * @brief  Gets the LCD Y size.
 * @retval Used LCD Y size
 */
/*uint32_t LCD_GetYSize(void)
{
  return hltdc.LayerCfg[0].ImageHeight;
}*/

/**
 * @brief  Set the LCD X size.
 * @param  imageWidthPixels : image width in pixels unit
 * @retval None
 */
/*void LCD_SetXSize(uint32_t imageWidthPixels)
{
  hltdc.LayerCfg[0].ImageWidth = imageWidthPixels;
}*/

/**
 * @brief  Set the LCD Y size.
 * @param  imageHeightPixels : image height in lines unit
 * @retval None
 */
/*void LCD_SetYSize(uint32_t imageHeightPixels)
{
  hltdc.LayerCfg[0].ImageHeight = imageHeightPixels;
}*/

// ================================================ LAYERS ================================================

/**
 * @brief  Initializes the LCD layer in ARGB8888 format (32 bits per pixel). No longer used (moved to main.c).
 * @param  LayerIndex: Layer foreground or background
 * @param  FB_Address: Layer frame buffer
 * @retval None
 */
/*void LCD_LayerArgb8888Init(uint16_t LayerIndex, uint32_t FB_Address)
{
  LTDC_LayerCfgTypeDef  pLayerCfg;

  // Layer Init
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = LCD_SCREEN_WIDTH;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = LCD_SCREEN_HEIGHT;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.FBStartAdress = FB_Address;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.ImageWidth = LCD_SCREEN_WIDTH;
  pLayerCfg.ImageHeight = LCD_SCREEN_HEIGHT;

  HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LayerIndex);

  BackColor = LCD_COLOR_WHITE;
  pFont     = &Font24;
  TextColor = LCD_COLOR_BLACK;
}*/

/**
 * @brief  Initializes the LCD layer in RGB565 format (16 bits per pixel).
 * @param  LayerIndex: Layer foreground or background
 * @param  FB_Address: Layer frame buffer
 * @retval None
 */
/*void LCD_LayerRgb565Init(uint16_t LayerIndex, uint32_t FB_Address)
{
  LTDC_LayerCfgTypeDef  layer_cfg;

  // Layer Init
  layer_cfg.WindowX0 = 0;
  layer_cfg.WindowX1 = LCD_SCREEN_WIDTH;
  layer_cfg.WindowY0 = 0;
  layer_cfg.WindowY1 = LCD_SCREEN_HEIGHT;
  layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  layer_cfg.FBStartAdress = FB_Address;
  layer_cfg.Alpha = 255;
  layer_cfg.Alpha0 = 0;
  layer_cfg.Backcolor.Blue = 0;
  layer_cfg.Backcolor.Green = 0;
  layer_cfg.Backcolor.Red = 0;
  layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  layer_cfg.ImageWidth = LCD_SCREEN_WIDTH;
  layer_cfg.ImageHeight = LCD_SCREEN_HEIGHT;

  HAL_LTDC_ConfigLayer(&hltdc, &layer_cfg, LayerIndex);

  BackColor = LCD_COLOR_WHITE;
  pFont     = &Font24;
  TextColor = LCD_COLOR_BLACK;
}*/


/**
 * @brief  Sets an LCD Layer visible
 * @param  LayerIndex: Visible Layer
 * @param  State: New state of the specified layer
 *          This parameter can be one of the following values:
 *            @arg  ENABLE
 *            @arg  DISABLE
 * @retval None
 */
/*void LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State)
{
  if(State == ENABLE)
  {
    __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
  }
  else
  {
    __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
  }
  __HAL_LTDC_RELOAD_CONFIG(&hltdc);
}*/

/**
 * @brief  Sets an LCD Layer visible without reloading.
 * @param  LayerIndex: Visible Layer
 * @param  State: New state of the specified layer
 *          This parameter can be one of the following values:
 *            @arg  ENABLE
 *            @arg  DISABLE
 * @retval None
 */
/*void LCD_SetLayerVisible_NoReload(uint32_t LayerIndex, FunctionalState State)
{
  if(State == ENABLE)
  {
    __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
  }
  else
  {
    __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
  }
  // Do not Sets the Reload
}*/

/**
 * @brief  Sets an LCD layer frame buffer address.
 * @param  LayerIndex: Layer foreground or background
 * @param  Address: New LCD frame buffer value
 * @retval None
 */
/*void LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address)
{
  HAL_LTDC_SetAddress(&hltdc, Address, LayerIndex);
}*/

/**
 * @brief  Sets an LCD layer frame buffer address without reloading.
 * @param  LayerIndex: Layer foreground or background
 * @param  Address: New LCD frame buffer value
 * @retval None
 */
/*void LCD_SetLayerAddress_NoReload(uint32_t LayerIndex, uint32_t Address)
{
  HAL_LTDC_SetAddress_NoReload(&hltdc, Address, LayerIndex);
}*/
