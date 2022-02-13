/*
 * disco_base.h
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 *
 * This file defines exported peripheral constants, e.g., addresses, buffer sizes, etc.
 *
 * CAMERA (aka DCMI interface) is not used here, but I left #defines commented in case one wants to re-enable it.
 */

#ifndef INC_DISCO_BASE_H_
#define INC_DISCO_BASE_H_

#include "disco_sdram.h"
#include "stdint.h"



/**
  * SD-detect signal
  */
#define SD_DETECT_PIN                        GPIO_PIN_13
#define SD_DETECT_GPIO_PORT                  GPIOC


/* Exported constant IO ------------------------------------------------------*/

#define RGB565_BYTE_PER_PIXEL     2
#define ARBG8888_BYTE_PER_PIXEL   4

/* Camera have a max resolution of VGA : 640x480 */
#define CAMERA_RES_MAX_X          640
#define CAMERA_RES_MAX_Y          480

/* LCD resolution */
#define  LCD_SCREEN_WIDTH    480
#define  LCD_SCREEN_HEIGHT   272

/**
  * LCD FB_StartAddress
  * LCD Frame buffer start address : starts at beginning of SDRAM = ((uint32_t)0xC0000000)
  *
  */
#define LCD_FRAME_BUFFER          SDRAM_DEVICE_ADDR

/**
  * Camera frame buffer start address
  * Assuming LCD frame buffer is of size 480x800 and format ARGB8888 (32 bits per pixel).
  *
  *
  * taille du LCD frame buffer = 510ko (= 480 * 272 * 4 = 522240)
  *
  */
#define CAMERA_FRAME_BUFFER       ((uint32_t)(LCD_FRAME_BUFFER + (LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARBG8888_BYTE_PER_PIXEL)))

/**
  * SDRAM audio write/read buffer start address after CAM Frame buffer
  * Assuming Camera frame buffer is of size 640x480 and has a pixel format of type RGB565 (16 bits per pixel).
  *
  * The external SDRAM on the 746Discovery has a size of 16Mbytes, but only 8Mbytes are usable (for address bus size reasons).
  * Notes:
  * - 8Mbytes = 2Meg x 32bits
  * - address (=pointers) are referring to bytes, even though the address bus is 32 bits wide.
  *
  * The CAMERA FRAME BUFFER is 600kbytes large (= 640 * 480 * 2 = 614400)
  *
  * Both frame buffers (camera + lcd) make up 1136640 bytes as a whole.
  *
  *
  */
//#define SDRAM_WRITE_READ_ADDR        ((uint32_t)(CAMERA_FRAME_BUFFER + (CAMERA_RES_MAX_X * CAMERA_RES_MAX_Y * RGB565_BYTE_PER_PIXEL)))
//#define SDRAM_WRITE_READ_ADDR       ((uint32_t)(LCD_FRAME_BUFFER + (LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT * ARBG8888_BYTE_PER_PIXEL)))
//#define SDRAM_WRITE_READ_ADDR        	((uint32_t)(SDRAM_DEVICE_ADDR + 1136640))
#define SDRAM_WRITE_READ_ADDR        	((uint32_t)(SDRAM_DEVICE_ADDR + 600 * 1024))

// pour test.c uniquement :
//#define SDRAM_WRITE_READ_ADDR_OFFSET ((uint32_t)0x0800)
#define SDRAM_WRITE_READ_ADDR_OFFSET ((uint32_t)0x0000)

// This is the audio scratch buffer used by processAudio() to store
// audio samples for e.g. long impulse response FIR filters (that is, long enough to not hold inside a single DMA frame!)
// it is located in external SDRAM and its address range begins right after the LCD frame buffer.
#define AUDIO_SCRATCH_ADDR  		SDRAM_WRITE_READ_ADDR
#define AUDIO_SCRATCH_MAXSZ_BYTES	(SDRAM_DEVICE_SIZE - (AUDIO_SCRATCH_ADDR-SDRAM_DEVICE_ADDR))
#define AUDIO_SCRATCH_MAXSZ_WORDS	AUDIO_SCRATCH_MAXSZ_BYTES / 2  // 16 bit
#define AUDIO_SCRATCH_MAXSZ_SAMPLES	AUDIO_SCRATCH_MAXSZ_WORDS / 2  // 16 bit stereo

// -------------------------------- functions --------------------

/* basic I/O functions */
void      	LED_On();
void      	LED_Off();
void      	LED_Toggle();

uint32_t  	PB_GetState();

#endif /* INC_DISCO_BASE_H_ */
