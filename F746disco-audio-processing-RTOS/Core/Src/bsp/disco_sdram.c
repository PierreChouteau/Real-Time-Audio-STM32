/*
 * disco_sdram.c
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */


#include "bsp/disco_sdram.h"
#include "main.h"

extern SDRAM_HandleTypeDef hsdram1;
static FMC_SDRAM_CommandTypeDef Command;

__IO uint32_t uwDMA_Transfer_Complete = 0;



/**
  * @brief  Programs the SDRAM device.
  * @param  RefreshCount: SDRAM refresh counter value
  * @retval None
  */
void DISCO_SDRAM_Initialization_sequence(uint32_t RefreshCount)
{
  __IO uint32_t tmpmrd = 0;

  /* Step 1: Configure a clock configuration enable command */
  Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 2: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 3: Configure a PALL (precharge all) command */
  Command.CommandMode            = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 4: Configure an Auto Refresh command */
  Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 8;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 5: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |\
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |\
                     SDRAM_MODEREG_CAS_LATENCY_2           |\
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |\
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 6: Set the refresh rate counter */
  /* Set the device refresh rate */
  HAL_SDRAM_ProgramRefreshRate(&hsdram1, RefreshCount);
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in polling mode.
  * @param  uwStartAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  uwDataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t DISCO_SDRAM_ReadData(uint32_t uwStartAddress, uint32_t *pData, uint32_t uwDataSize)
{
  if(HAL_SDRAM_Read_32b(&hsdram1, (uint32_t *)uwStartAddress, pData, uwDataSize) != HAL_OK)
  {
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in DMA mode.
  * @param  uwStartAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  uwDataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t DISCO_SDRAM_ReadData_DMA(uint32_t uwStartAddress, uint32_t *pData, uint32_t uwDataSize)
{
  if(HAL_SDRAM_Read_DMA(&hsdram1, (uint32_t *)uwStartAddress, pData, uwDataSize) != HAL_OK)
  {
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in polling mode.
  * @param  uwStartAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  uwDataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t DISCO_SDRAM_WriteData(uint32_t uwStartAddress, uint32_t *pData, uint32_t uwDataSize)
{
  return HAL_SDRAM_Write_32b(&hsdram1, (uint32_t *)uwStartAddress, pData, uwDataSize);
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in DMA mode.
  * @param  uwStartAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  uwDataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t DISCO_SDRAM_WriteData_DMA(uint32_t uwStartAddress, uint32_t *pData, uint32_t uwDataSize)
{
  return HAL_SDRAM_Write_DMA(&hsdram1, (uint32_t *)uwStartAddress, pData, uwDataSize);
}

/**
  * @brief  Sends command to the SDRAM bank.
  * @param  SdramCmd: Pointer to SDRAM command structure
  * @retval SDRAM status
  */
uint8_t DISCO_SDRAM_Sendcmd(FMC_SDRAM_CommandTypeDef *SdramCmd)
{
  return HAL_SDRAM_SendCommand(&hsdram1, SdramCmd, SDRAM_TIMEOUT);
}


/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void HAL_SDRAM_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
  /* Set transfer complete flag */
  uwDMA_Transfer_Complete = 1;
}

/**
  * @brief  DMA transfer complete error callback.
  * @param  hdma: DMA handle
  * @retval None
  */

void HAL_SDRAM_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
  Error_Handler();
}

// =============================== arxiv =====================================

/**
  * @brief  Initializes the SDRAM device.
  * @retval SDRAM status
  */
//uint8_t DISCO_SDRAM_Init(void) // aka MX_FMC_Init()
//{
//  static uint8_t sdramstatus = HAL_ERROR;
//  /* SDRAM device configuration */
//  hsdram1.Instance = FMC_SDRAM_DEVICE;
//
//  hsdram1.Init.SDBank             = FMC_SDRAM_BANK1;
//  hsdram1.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
//  hsdram1.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
//  hsdram1.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_16;
//  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
//  hsdram1.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_2; // TODO FMC_SDRAM_CAS_LATENCY_3 in MX_FMC_Init()
//  hsdram1.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
//  hsdram1.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;
//  hsdram1.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
//  hsdram1.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;
//
//  /* Timing configuration for 100Mhz as SD clock frequency (System clock is up to 200Mhz) */
//  SdramTiming.LoadToActiveDelay    = 2;
//  SdramTiming.ExitSelfRefreshDelay = 7;
//  SdramTiming.SelfRefreshTime      = 4;
//  SdramTiming.RowCycleDelay        = 7;
//  SdramTiming.WriteRecoveryTime    = 2; // TODO 3 in MX_FMC_Init
//  SdramTiming.RPDelay              = 2;
//  SdramTiming.RCDDelay             = 2;
//
//  /* SDRAM controller initialization
//   * 1) enable clocks (FMC, DMA2, GPIOx)
//   * 2) configure GPIO pins
//   * 3) fill dma handle
//   * 4) link dma handle with sdram
//   * 5) init dma from dma handle
//   * 6) config DMA IRQ priority
//   */
//  //DISCO_SDRAM_MspInit(&hsdram1, NULL);
//
//  if(HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK) sdramstatus = HAL_ERROR;
//  else sdramstatus = HAL_OK;
//
//  /* SDRAM initialization sequence */
//  DISCO_SDRAM_Initialization_sequence(REFRESH_COUNT);
//
//  return sdramstatus;
//}


//void DISCO_SDRAM_MspInit(SDRAM_HandleTypeDef  *hsdram, void *Params)
//{
//  static DMA_HandleTypeDef dma_handle;
//  GPIO_InitTypeDef gpio_init_structure;
//
//  /* Enable FMC clock */
//  __HAL_RCC_FMC_CLK_ENABLE();
//
//  /* Enable chosen DMAx clock */
//  __HAL_RCC_DMA2_CLK_ENABLE(); // done in MX_DMA_Init()
//
//  /* Enable GPIOs clock */
//  __HAL_RCC_GPIOC_CLK_ENABLE(); // all GPIO clocks done in MX_GPIO_Init()
//  __HAL_RCC_GPIOD_CLK_ENABLE();
//  __HAL_RCC_GPIOE_CLK_ENABLE();
//  __HAL_RCC_GPIOF_CLK_ENABLE();
//  __HAL_RCC_GPIOG_CLK_ENABLE();
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//
//  /* Common GPIO configuration */
//  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
//  gpio_init_structure.Pull      = GPIO_PULLUP;
//  gpio_init_structure.Speed     = GPIO_SPEED_FAST;
//  gpio_init_structure.Alternate = GPIO_AF12_FMC;
//
//  /* GPIOC configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_3;
//  HAL_GPIO_Init(GPIOC, &gpio_init_structure);
//
//  /* GPIOD configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
//  HAL_GPIO_Init(GPIOD, &gpio_init_structure);
//
//  /* GPIOE configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7| GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//  HAL_GPIO_Init(GPIOE, &gpio_init_structure);
//
//  /* GPIOF configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//  HAL_GPIO_Init(GPIOF, &gpio_init_structure);
//
//  /* GPIOG configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4| GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
//  HAL_GPIO_Init(GPIOG, &gpio_init_structure);
//
//  /* GPIOH configuration */
//  gpio_init_structure.Pin   = GPIO_PIN_3 | GPIO_PIN_5;
//  HAL_GPIO_Init(GPIOH, &gpio_init_structure);
//
//  /* Configure common DMA parameters */
//  dma_handle.Init.Channel             = DMA_CHANNEL_0;
//  dma_handle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
//  dma_handle.Init.PeriphInc           = DMA_PINC_ENABLE;
//  dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
//  dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
//  dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
//  dma_handle.Init.Mode                = DMA_NORMAL;
//  dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
//  dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE; // vs enable
//  dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
//  dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
//  dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;
//
//  dma_handle.Instance = DMA2_Stream0;
//
//   /* Associate the DMA handle */
//  __HAL_LINKDMA(hsdram, hdma, dma_handle);
//  //ie:
//  // hsdram->hdma = &dma_handle;
//  // dma_handle.Parent = hsdram;
//
//  /* Deinitialize the stream for new transfer */
//  HAL_DMA_DeInit(&dma_handle);
//
//  /* Configure the DMA stream */
//  HAL_DMA_Init(&dma_handle);
//
//  /* NVIC configuration for DMA transfer complete interrupt */
//  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0x0F, 0);
//  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
//}
