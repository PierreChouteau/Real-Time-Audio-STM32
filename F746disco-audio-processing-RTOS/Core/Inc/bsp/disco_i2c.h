/*
 * disco_i2c.h
 *
 *  Created on: Nov 21, 2021
 *      Author: sydxrey
 *
 *  I2C board-specific functions and constants.
 */

#ifndef INC_BSP_DISCO_I2C_H_
#define INC_BSP_DISCO_I2C_H_

#include "stm32f7xx_hal.h"



/* peripheral addresses */

#define LCD_I2C_ADDRESS                  ((uint16_t)0x70)
// not used #define CAMERA_I2C_ADDRESS               ((uint16_t)0x60)
#define AUDIO_I2C_ADDRESS                ((uint16_t)0x34)
#define TS_I2C_ADDRESS                   ((uint16_t)0x70)




/* I2C clock speed configuration (in Hz) */

 //#define I2C_SPEED                       ((uint32_t)100000)

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated from APB1 source clock = 50 MHz */
/* Due to the big MOFSET capacity for adapting the camera level the rising time is very large (>1us) */
/* 0x40912732 takes in account the big rising and aims a clock of 100khz */
//#define DISCOVERY_I2Cx_TIMING                      ((uint32_t)0x40912732)




/* TOUCHSCREEN controller  */

void      	TS_I2C_Write(uint8_t Reg, uint8_t Value);
uint8_t   	TS_I2C_Read(uint8_t Reg);
void      	TS_IO_Delay(uint32_t Delay);



/* AUDIO codec control */

void 		AUDIO_I2C_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
uint16_t 	AUDIO_I2C_Read(uint8_t Addr, uint16_t Reg);


#endif /* INC_BSP_DISCO_I2C_H_ */
