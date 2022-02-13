/*
 * disco_sai.h
 *
 *  Created on: May 17, 2021
 *      Author: sydxrey
 */

#ifndef INC_DISCO_SAI_H_
#define INC_DISCO_SAI_H_


#include "stm32f7xx_hal.h"
#include "wm8994.h"
#include "stdint.h"

/* Codec audio Standards */
#define CODEC_STANDARD                0x04
#define I2S_STANDARD                  I2S_STANDARD_PHILIPS

#define AUDIO_I2C_ADDRESS                ((uint16_t)0x34)

/* To have 2 separate audio stream in Both headphone and speaker the 4 slot must be activated */
#define CODEC_AUDIOFRAME_SLOT_0123           SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1 | SAI_SLOTACTIVE_2 | SAI_SLOTACTIVE_3
/* To have an audio stream in headphone only SAI Slot 0 and Slot 2 must be activated */
#define CODEC_AUDIOFRAME_SLOT_02             SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_2
/* To have an audio stream in speaker only SAI Slot 1 and Slot 3 must be activated */
#define CODEC_AUDIOFRAME_SLOT_13             SAI_SLOTACTIVE_1 | SAI_SLOTACTIVE_3

// defines for easier remembering:
#define CODEC_AUDIOFRAME_LINEIN CODEC_AUDIOFRAME_SLOT_02
#define CODEC_AUDIOFRAME_MICIN 	CODEC_AUDIOFRAME_SLOT_13
#define CODEC_AUDIOFRAME_HPOUT 	CODEC_AUDIOFRAME_SLOT_02
//#define CODEC_AUDIOFRAME_SPKOUT	CODEC_AUDIOFRAME_SLOT_13 // NOT USED

#define AUDIO_OK                            ((uint8_t)0)
#define AUDIO_ERROR                         ((uint8_t)1)
#define AUDIO_TIMEOUT                       ((uint8_t)2)

void setInput(uint16_t InputDevice);
void start_Audio_Processing(int16_t* buf_output, int16_t* buf_input, uint32_t audio_dma_buf_size, uint16_t InputDevice, uint32_t AudioFreq);

#endif /* INC_DISCO_SAI_H_ */
