/*
 * disco_base.h
 * Common fonctions:
 * - Green LED (connected to Arduino header CN7, pin D13 => can be used for oscilloscope timing measurement as well)
 * - Push Button
 * - printf (through __io_putchar)
 * - I2C communication with the board peripherals (audio, touchscreen, camera, external I2C on Arduino connector)
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */

#include "bsp/disco_base.h"
#include "main.h"
#include "stdio.h"


// -------------------------------- onboard green led (CN7, pin D13) --------------------------------

void LED_On(){

	HAL_GPIO_WritePin(GPIOI, LD_GREEN_Pin, GPIO_PIN_SET);
}

void LED_Off(){

	HAL_GPIO_WritePin(GPIOI, LD_GREEN_Pin, GPIO_PIN_RESET);
}

void LED_Toggle(){

	HAL_GPIO_TogglePin(GPIOI, LD_GREEN_Pin);

}

// --------------------------------- onboard blue button ------------------------------

/**
 * @return GPIO_PIN_SET or GPIO_PIN_RESET depending on the state of the onboard blue (push) button
 */
uint32_t PB_GetState(){

  return HAL_GPIO_ReadPin(BLUE_BTN_GPIO_Port, BLUE_BTN_Pin);

}

// ---------------------------------- VCP on USART1 ------------------------------------

extern UART_HandleTypeDef huart1; // needed below, defined in main.c

/**
 * __io_putchar() is called by usual printf() implementations.
 * This implementation of __io_putchar() overrides the default behavior so that it
 * writes the char given as argument to the USART1 line. As this line is connected to the Virtual COM Port (VCP)
 * of the STLink (USB) programming port, it is enough to open a serial terminal on the host PC
 * (e.g., Hyperterminal or Putty on Windows, screen on Linux or Macos) to obtain a debugging console.
 * On linux/MacOS, this VCP shows up as /dev/ttyUSBSomething (ls /dev etc will provide you with its real name),
 * while on Windows, it's COMSOmething (from COM1 to ...).
 */
int __io_putchar(int ch){

	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF); // beware blocking call! TODO => use DMA
	return ch;
}



