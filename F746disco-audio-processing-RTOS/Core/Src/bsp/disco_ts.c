/*
 * disco_ts.c FT5336 touchscreen driver
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */


#include "bsp/disco_ts.h"
#include "bsp/disco_lcd.h"


static void TS_GetXY(uint16_t *X, uint16_t *Y);
static void TS_GetTouchInfo(uint32_t touchIdx, uint32_t* pWeight, uint32_t* pArea, uint32_t* pEvent);

//static uint16_t tsXBoundary, tsYBoundary;
#define TS_ORIENTATION TS_SWAP_XY

/* field holding the current number of simultaneous active touches */
static uint8_t currActiveTouchNb;

/* field holding the touch index currently managed */
static uint8_t currActiveTouchIdx;

/**
 * @brief  Initializes and configures the touch screen functionalities and
 *         configures all necessary hardware resources (GPIOs, I2C, clocks..).
 * @retval TS_OK if all initializations are OK. Other value if error.
 */
void TS_Init()
{
	/* Wait at least 200ms after power up before accessing registers
	 * Trsi timing (Time of starting to report point after resetting) from FT5336GQQ datasheet */
	HAL_Delay(200);

	TS_DisableIT();

}


/**
 * @brief  Returns status and positions of the touch screen.
 * @param  TS_State: Pointer to touch screen current state structure
 * @retval TS_OK if all initializations are OK. Other value if error.
 */
uint8_t TS_GetState(TS_StateTypeDef *TS_State)
{
	static uint32_t _x[TS_MAX_NB_TOUCH] = {0, 0};
	static uint32_t _y[TS_MAX_NB_TOUCH] = {0, 0};
	uint8_t ts_status = TS_OK;
	uint16_t x[TS_MAX_NB_TOUCH];
	uint16_t y[TS_MAX_NB_TOUCH];
	uint16_t brute_x[TS_MAX_NB_TOUCH];
	uint16_t brute_y[TS_MAX_NB_TOUCH];
	uint16_t x_diff;
	uint16_t y_diff;
	uint32_t index;
	uint32_t weight = 0;
	uint32_t area = 0;
	uint32_t event = 0;

	/* Read register FT5336_TD_STAT_REG to check number of touches detection */
	volatile uint8_t nbTouch = TS_I2C_Read(FT5336_TD_STAT_REG) & FT5336_TD_STAT_MASK;

	if(nbTouch > FT5336_MAX_DETECTABLE_TOUCH){
		/* If invalid number of touch detected, set it to zero */
		nbTouch = 0;
	}

	/* Update current number of active touches */
	currActiveTouchNb = nbTouch;

	/* Reset current active touch index on which to work on */
	currActiveTouchIdx = 0;

	/* Check and update the number of touches active detected */
	TS_State->touchDetected = nbTouch; // ft5336_TS_DetectTouch();

	if(TS_State->touchDetected)
	{
		for(index=0; index < TS_State->touchDetected; index++)
		{
			/* Get each touch coordinates */
			TS_GetXY(&(brute_x[index]), &(brute_y[index]));

			if(TS_ORIENTATION == TS_SWAP_NONE)
			{
				x[index] = brute_x[index];
				y[index] = brute_y[index];
			}

			if(TS_ORIENTATION & TS_SWAP_X)
			{
				x[index] = 4096 - brute_x[index];
			}

			if(TS_ORIENTATION & TS_SWAP_Y)
			{
				y[index] = 4096 - brute_y[index];
			}

			if(TS_ORIENTATION & TS_SWAP_XY)
			{
				y[index] = brute_x[index];
				x[index] = brute_y[index];
			}

			x_diff = x[index] > _x[index]? (x[index] - _x[index]): (_x[index] - x[index]);
			y_diff = y[index] > _y[index]? (y[index] - _y[index]): (_y[index] - y[index]);

			if ((x_diff + y_diff) > 5)
			{
				_x[index] = x[index];
				_y[index] = y[index];
			}

			TS_State->touchX[index] = x[index];
			TS_State->touchY[index] = y[index];

			/* Get touch info related to the current touch */
			TS_GetTouchInfo(index, &weight, &area, &event);

			/* Update TS_State structure */
			TS_State->touchWeight[index] = weight;
			TS_State->touchArea[index]   = area;

			/* Remap touch event */
			switch(event)
			{
			case FT5336_TOUCH_EVT_FLAG_PRESS_DOWN	:
				TS_State->touchEventId[index] = TOUCH_EVENT_PRESS_DOWN;
				break;
			case FT5336_TOUCH_EVT_FLAG_LIFT_UP :
				TS_State->touchEventId[index] = TOUCH_EVENT_LIFT_UP;
				break;
			case FT5336_TOUCH_EVT_FLAG_CONTACT :
				TS_State->touchEventId[index] = TOUCH_EVENT_CONTACT;
				break;
			case FT5336_TOUCH_EVT_FLAG_NO_EVENT :
				TS_State->touchEventId[index] = TOUCH_EVENT_NO_EVT;
				break;
			default :
				ts_status = TS_ERROR;
				break;
			} /* of switch(event) */


		} /* of for(index=0; index < TS_State->touchDetected; index++) */

		/* Get gesture Id */
		ts_status = TS_Get_GestureId(TS_State);

	} /* end of if(TS_State->touchDetected != 0) */

	return (ts_status);
}

/**
 * @brief  Update gesture Id following a touch detected.
 * @param  TS_State: Pointer to touch screen current state structure
 * @retval TS_OK if all initializations are OK. Other value if error.
 */
uint8_t TS_Get_GestureId(TS_StateTypeDef *TS_State)
{
	uint32_t gestureId = TS_I2C_Read(FT5336_GEST_ID_REG);

	//ft5336_TS_GetGestureID(&gestureId);

	/* Remap gesture Id to a TS_GestureIdTypeDef value */
	switch(gestureId)
	{
	case FT5336_GEST_ID_NO_GESTURE :
		TS_State->gestureId = GEST_ID_NO_GESTURE;
		break;
	case FT5336_GEST_ID_MOVE_UP :
		TS_State->gestureId = GEST_ID_MOVE_UP;
		break;
	case FT5336_GEST_ID_MOVE_RIGHT :
		TS_State->gestureId = GEST_ID_MOVE_RIGHT;
		break;
	case FT5336_GEST_ID_MOVE_DOWN :
		TS_State->gestureId = GEST_ID_MOVE_DOWN;
		break;
	case FT5336_GEST_ID_MOVE_LEFT :
		TS_State->gestureId = GEST_ID_MOVE_LEFT;
		break;
	case FT5336_GEST_ID_ZOOM_IN :
		TS_State->gestureId = GEST_ID_ZOOM_IN;
		break;
	case FT5336_GEST_ID_ZOOM_OUT :
		TS_State->gestureId = GEST_ID_ZOOM_OUT;
		break;
	default :
		return TS_ERROR;
	} /* of switch(gestureId) */

	return TS_OK;
}

/**
 * @brief  Function used to reset all touch data before a new acquisition
 *         of touch information.
 * @param  TS_State: Pointer to touch screen current state structure
 * @retval TS_OK if OK, TE_ERROR if problem found.
 */
uint8_t TS_ResetTouchData(TS_StateTypeDef *TS_State)
{
	uint8_t ts_status = TS_ERROR;
	uint32_t index;

	if (TS_State != (TS_StateTypeDef *)NULL)
	{
		TS_State->gestureId = GEST_ID_NO_GESTURE;
		TS_State->touchDetected = 0;

		for(index = 0; index < TS_MAX_NB_TOUCH; index++)
		{
			TS_State->touchX[index]       = 0;
			TS_State->touchY[index]       = 0;
			TS_State->touchArea[index]    = 0;
			TS_State->touchEventId[index] = TOUCH_EVENT_NO_EVT;
			TS_State->touchWeight[index]  = 0;
		}

		ts_status = TS_OK;

	} /* of if (TS_State != (TS_StateTypeDef *)NULL) */

	return (ts_status);
}

/**
  * @brief  Get the touch screen X and Y positions values
  *         Manage multi touch thanks to touch Index global
  *         variable 'currActiveTouchIdx'.
  * @param  X: Pointer to X position value
  * @param  Y: Pointer to Y position value
  * @retval None.
  */
static void TS_GetXY(uint16_t *X, uint16_t *Y)
{
  volatile uint8_t ucReadData = 0;
  static uint16_t coord;
  uint8_t regAddressXLow = 0;
  uint8_t regAddressXHigh = 0;
  uint8_t regAddressYLow = 0;
  uint8_t regAddressYHigh = 0;

  if(currActiveTouchIdx < currActiveTouchNb)
  {
    switch(currActiveTouchIdx)
    {
    case 0 :
      regAddressXLow  = FT5336_P1_XL_REG;
      regAddressXHigh = FT5336_P1_XH_REG;
      regAddressYLow  = FT5336_P1_YL_REG;
      regAddressYHigh = FT5336_P1_YH_REG;
      break;

    case 1 :
      regAddressXLow  = FT5336_P2_XL_REG;
      regAddressXHigh = FT5336_P2_XH_REG;
      regAddressYLow  = FT5336_P2_YL_REG;
      regAddressYHigh = FT5336_P2_YH_REG;
      break;

    case 2 :
      regAddressXLow  = FT5336_P3_XL_REG;
      regAddressXHigh = FT5336_P3_XH_REG;
      regAddressYLow  = FT5336_P3_YL_REG;
      regAddressYHigh = FT5336_P3_YH_REG;
      break;

    case 3 :
      regAddressXLow  = FT5336_P4_XL_REG;
      regAddressXHigh = FT5336_P4_XH_REG;
      regAddressYLow  = FT5336_P4_YL_REG;
      regAddressYHigh = FT5336_P4_YH_REG;
      break;

    case 4 :
      regAddressXLow  = FT5336_P5_XL_REG;
      regAddressXHigh = FT5336_P5_XH_REG;
      regAddressYLow  = FT5336_P5_YL_REG;
      regAddressYHigh = FT5336_P5_YH_REG;
      break;

    case 5 :
      regAddressXLow  = FT5336_P6_XL_REG;
      regAddressXHigh = FT5336_P6_XH_REG;
      regAddressYLow  = FT5336_P6_YL_REG;
      regAddressYHigh = FT5336_P6_YH_REG;
      break;

    case 6 :
      regAddressXLow  = FT5336_P7_XL_REG;
      regAddressXHigh = FT5336_P7_XH_REG;
      regAddressYLow  = FT5336_P7_YL_REG;
      regAddressYHigh = FT5336_P7_YH_REG;
      break;

    case 7 :
      regAddressXLow  = FT5336_P8_XL_REG;
      regAddressXHigh = FT5336_P8_XH_REG;
      regAddressYLow  = FT5336_P8_YL_REG;
      regAddressYHigh = FT5336_P8_YH_REG;
      break;

    case 8 :
      regAddressXLow  = FT5336_P9_XL_REG;
      regAddressXHigh = FT5336_P9_XH_REG;
      regAddressYLow  = FT5336_P9_YL_REG;
      regAddressYHigh = FT5336_P9_YH_REG;
      break;

    case 9 :
      regAddressXLow  = FT5336_P10_XL_REG;
      regAddressXHigh = FT5336_P10_XH_REG;
      regAddressYLow  = FT5336_P10_YL_REG;
      regAddressYHigh = FT5336_P10_YH_REG;
      break;

    default :
      break;

    } /* end switch(currActiveTouchIdx) */

    /* Read low part of X position */
    ucReadData = TS_I2C_Read(regAddressXLow);
    coord = (ucReadData & FT5336_TOUCH_POS_LSB_MASK) >> FT5336_TOUCH_POS_LSB_SHIFT;

    /* Read high part of X position */
    ucReadData = TS_I2C_Read(regAddressXHigh);
    coord |= ((ucReadData & FT5336_TOUCH_POS_MSB_MASK) >> FT5336_TOUCH_POS_MSB_SHIFT) << 8;

    /* Send back ready X position to caller */
    *X = coord;

    /* Read low part of Y position */
    ucReadData = TS_I2C_Read(regAddressYLow);
    coord = (ucReadData & FT5336_TOUCH_POS_LSB_MASK) >> FT5336_TOUCH_POS_LSB_SHIFT;

    /* Read high part of Y position */
    ucReadData = TS_I2C_Read(regAddressYHigh);
    coord |= ((ucReadData & FT5336_TOUCH_POS_MSB_MASK) >> FT5336_TOUCH_POS_MSB_SHIFT) << 8;

    /* Send back ready Y position to caller */
    *Y = coord;

    currActiveTouchIdx++; /* next call will work on next touch */

  } /* of if(currActiveTouchIdx < currActiveTouchNb) */
}

/**
  * @brief  Configure the FT5336 device to generate IT on given INT pin
  *         connected to MCU as EXTI.
  * @retval None
  */
void TS_EnableIT()
{
   uint8_t regValue = 0;
   regValue = (FT5336_G_MODE_INTERRUPT_TRIGGER & (FT5336_G_MODE_INTERRUPT_MASK >> FT5336_G_MODE_INTERRUPT_SHIFT)) << FT5336_G_MODE_INTERRUPT_SHIFT;

   /* Set interrupt trigger mode in FT5336_GMODE_REG */
   TS_I2C_Write(FT5336_GMODE_REG, regValue);
}

/**
  * @brief  Configure the FT5336 device to stop generating IT on the given INT pin
  *         connected to MCU as EXTI.
  * @retval None
  */
void TS_DisableIT()
{
  uint8_t regValue = 0;
  regValue = (FT5336_G_MODE_INTERRUPT_POLLING & (FT5336_G_MODE_INTERRUPT_MASK >> FT5336_G_MODE_INTERRUPT_SHIFT)) << FT5336_G_MODE_INTERRUPT_SHIFT;

  /* Set interrupt polling mode in FT5336_GMODE_REG */
  TS_I2C_Write(FT5336_GMODE_REG, regValue);
}

/**
  * @brief  Get the touch detailed informations on touch number 'touchIdx' (0..1)
  *         This touch detailed information contains :
  *         - weight that was applied to this touch
  *         - sub-area of the touch in the touch panel
  *         - event of linked to the touch (press down, lift up, ...)
  * @param  touchIdx : Passed index of the touch (0..1) on which we want to get the
  *                    detailed information.
  * @param  pWeight : Pointer to to get the weight information of 'touchIdx'.
  * @param  pArea   : Pointer to to get the sub-area information of 'touchIdx'.
  * @param  pEvent  : Pointer to to get the event information of 'touchIdx'.

  * @retval None.
  */
static void TS_GetTouchInfo(uint32_t   touchIdx, uint32_t * pWeight, uint32_t * pArea, uint32_t * pEvent)
{
  volatile uint8_t ucReadData = 0;
  uint8_t regAddressXHigh = 0;
  uint8_t regAddressPWeight = 0;
  uint8_t regAddressPMisc = 0;

  if(touchIdx < currActiveTouchNb)
  {
    switch(touchIdx)
    {
    case 0 :
      regAddressXHigh   = FT5336_P1_XH_REG;
      regAddressPWeight = FT5336_P1_WEIGHT_REG;
      regAddressPMisc   = FT5336_P1_MISC_REG;
      break;

    case 1 :
      regAddressXHigh   = FT5336_P2_XH_REG;
      regAddressPWeight = FT5336_P2_WEIGHT_REG;
      regAddressPMisc   = FT5336_P2_MISC_REG;
      break;

    case 2 :
      regAddressXHigh   = FT5336_P3_XH_REG;
      regAddressPWeight = FT5336_P3_WEIGHT_REG;
      regAddressPMisc   = FT5336_P3_MISC_REG;
      break;

    case 3 :
      regAddressXHigh   = FT5336_P4_XH_REG;
      regAddressPWeight = FT5336_P4_WEIGHT_REG;
      regAddressPMisc   = FT5336_P4_MISC_REG;
      break;

    case 4 :
      regAddressXHigh   = FT5336_P5_XH_REG;
      regAddressPWeight = FT5336_P5_WEIGHT_REG;
      regAddressPMisc   = FT5336_P5_MISC_REG;
      break;

    case 5 :
      regAddressXHigh   = FT5336_P6_XH_REG;
      regAddressPWeight = FT5336_P6_WEIGHT_REG;
      regAddressPMisc   = FT5336_P6_MISC_REG;
      break;

    case 6 :
      regAddressXHigh   = FT5336_P7_XH_REG;
      regAddressPWeight = FT5336_P7_WEIGHT_REG;
      regAddressPMisc   = FT5336_P7_MISC_REG;
      break;

    case 7 :
      regAddressXHigh   = FT5336_P8_XH_REG;
      regAddressPWeight = FT5336_P8_WEIGHT_REG;
      regAddressPMisc   = FT5336_P8_MISC_REG;
      break;

    case 8 :
      regAddressXHigh   = FT5336_P9_XH_REG;
      regAddressPWeight = FT5336_P9_WEIGHT_REG;
      regAddressPMisc   = FT5336_P9_MISC_REG;
      break;

    case 9 :
      regAddressXHigh   = FT5336_P10_XH_REG;
      regAddressPWeight = FT5336_P10_WEIGHT_REG;
      regAddressPMisc   = FT5336_P10_MISC_REG;
      break;

    default :
      break;

    } /* end switch(touchIdx) */

    /* Read Event Id of touch index */
    ucReadData = TS_I2C_Read(regAddressXHigh);
    * pEvent = (ucReadData & FT5336_TOUCH_EVT_FLAG_MASK) >> FT5336_TOUCH_EVT_FLAG_SHIFT;

    /* Read weight of touch index */
    ucReadData = TS_I2C_Read(regAddressPWeight);
    * pWeight = (ucReadData & FT5336_TOUCH_WEIGHT_MASK) >> FT5336_TOUCH_WEIGHT_SHIFT;

    /* Read area of touch index */
    ucReadData = TS_I2C_Read(regAddressPMisc);
    * pArea = (ucReadData & FT5336_TOUCH_AREA_MASK) >> FT5336_TOUCH_AREA_SHIFT;

  } /* of if(touchIdx < currActiveTouchNb) */
}



