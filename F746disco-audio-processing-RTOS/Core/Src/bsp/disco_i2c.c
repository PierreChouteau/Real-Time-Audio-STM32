/*
 * disco_i2c.c
 *
 *  Created on: Nov 21, 2021
 *      Author: sydxrey
 *
 *
 * I2C board-specific functions.
 */

#include "bsp/disco_i2c.h"
#include "main.h"


extern I2C_HandleTypeDef hi2c1; // I2C1 => Camera + connecteur extérieur
extern I2C_HandleTypeDef hi2c3; // I2C3 => Touchscreen & Audio




static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static void              I2Cx_Error(I2C_HandleTypeDef *i2c_handler, uint8_t Addr);



/**
  * Reads multiple data over the given I2C bus (blocking mode)
  * @param  i2c_handler : I2C handler
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  MemAddress: Memory address
  * @param  Buffer: Pointer to data buffer
  * @param  Length: Length of the data
  * @retval Number of read data
  */
static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *i2c_handler,
                                           uint8_t Addr,
                                           uint16_t Reg,
                                           uint16_t MemAddress,
                                           uint8_t *Buffer,
                                           uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Read(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* I2C error occurred */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}

/**
  * Writes a value in a register of the device over the given I2C bus (blocking mode)
  * @param  i2c_handler : I2C handler
  * @param  Addr: Device address on BUS Bus.
  * @param  Reg: The target register address to write
  * @param  MemAddress: Memory address
  * @param  Buffer: The target register value to be written
  * @param  Length: buffer size to be written
  * @retval HAL status
  */
static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *i2c_handler,
                                            uint8_t Addr,
                                            uint16_t Reg,
                                            uint16_t MemAddress,
                                            uint8_t *Buffer,
                                            uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Write(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Re-Initiaize the I2C Bus */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}

/**
  * Manages error callback by re-initializing I2C.
  * @param  i2c_handler : I2C handler
  * @param  Addr: I2C Address
  * @retval None
  */
static void I2Cx_Error(I2C_HandleTypeDef *i2c_handler, uint8_t Addr)
{
  /* De-initialize the I2C communication bus */
  HAL_I2C_DeInit(i2c_handler);

  /* Re-Initialize the I2C communication bus */
  HAL_I2C_Init(i2c_handler);
}





/*  ---------------------------------- AUDIO WM8994 codec I2C communication ---------------------------------- */



/**
  * Writes a single CONTROL data to the audio codec over the I2C bus.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
void AUDIO_I2C_Write(uint8_t Addr, uint16_t Reg, uint16_t Value){

  uint16_t tmp = Value;

  Value = ((uint16_t)(tmp >> 8) & 0x00FF);

  Value |= ((uint16_t)(tmp << 8)& 0xFF00);

  I2Cx_WriteMultiple(&hi2c3, Addr, Reg, I2C_MEMADD_SIZE_16BIT,(uint8_t*)&Value, 2);
}

/**
  * Reads a single CONTROL data from the audio codec over the I2C bus.
  * @param  Addr: I2C address
  * @param  Reg: Reg address
  * @retval Data to be read
  */
uint16_t AUDIO_I2C_Read(uint8_t Addr, uint16_t Reg){

  uint16_t read_value = 0, tmp = 0;

  I2Cx_ReadMultiple(&hi2c3, Addr, Reg, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&read_value, 2);

  tmp = ((uint16_t)(read_value >> 8) & 0x00FF);

  tmp |= ((uint16_t)(read_value << 8)& 0xFF00);

  read_value = tmp;

  return read_value;
}




/* ---------------------------------- TOUCHSCREEN I2C communication ---------------------------------- */

/**
  * Writes a single control data to the touchscreen controller over the I2C bus.
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
void TS_I2C_Write(uint8_t Reg, uint8_t Value){

  I2Cx_WriteMultiple(&hi2c3, TS_I2C_ADDRESS, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT,(uint8_t*)&Value, 1);

}

/**
  * Reads a single control data from the touchscreen controller over the I2C bus.
  * @param  Reg: Reg address
  * @retval Data to be read
  */
uint8_t TS_I2C_Read(uint8_t Reg){

  uint8_t read_value = 0;

  I2Cx_ReadMultiple(&hi2c3, TS_I2C_ADDRESS, Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&read_value, 1);

  return read_value;
}

