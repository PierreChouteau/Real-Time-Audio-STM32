/*
 * disco_sai.c
 *
 * SAI2 Block A => audio OUT (master)
 *      Block B => audio IN  (slave)
 *
 *  Created on: May 10, 2021
 *      Author: sydxrey
 */

/*  CODEC_AudioFrame_SLOT_TDMMode

 In W8994 codec the Audio frame contains 4 slots : TDM Mode
 TDM format : 4 slots numbered 0, 1, 2 and 3
 +------------------|------------------|--------------------|-------------------+
 | CODEC_SLOT0 Left | CODEC_SLOT1 Left | CODEC_SLOT0 Right  | CODEC_SLOT1 Right |
 +------------------------------------------------------------------------------+


 Output slots:

 To have 2 separate audio stream in Both headphone and speaker the 4 slot must be activated
 CODEC_AUDIOFRAME_SLOT_0123           SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1 | SAI_SLOTACTIVE_2 | SAI_SLOTACTIVE_3

 To have an audio stream in headphone only SAI Slot 0 and Slot 2 must be activated
 CODEC_AUDIOFRAME_SLOT_02             SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_2

 To have an audio stream in speaker only SAI Slot 1 and Slot 3 must be activated
 CODEC_AUDIOFRAME_SLOT_13             SAI_SLOTACTIVE_1 | SAI_SLOTACTIVE_3


 Input slots :

 Line In 1 : CODEC_AUDIOFRAME_SLOT_02 (=1+4=5)
 Dig Mic 2 : CODEC_AUDIOFRAME_SLOT_13 (=2+8=A)



 */

#include "bsp/disco_sai.h"
#include "bsp/wm8994.h"
#include "stdio.h"
#include "main.h"


extern SAI_HandleTypeDef hsai_BlockA2; // see main.c
extern SAI_HandleTypeDef hsai_BlockB2;
extern DMA_HandleTypeDef hdma_sai2_a;
extern DMA_HandleTypeDef hdma_sai2_b;

// -------------------------------- functions --------------------------------

void Error_Handler();

/**
 * Initializes wave recording and playback in parallel (aka full-duplex) with:
 * - LineIn or MicIn as input
 * - and HeadPhone as output.
 *
 * By default: 16 bit per sample, stereo.
 *
 * @param InputDevice is either INPUT_DEVICE_DIGITAL_MICROPHONE_2 or INPUT_DEVICE_INPUT_LINE_1 (output device is always HEADPHONE)
 * @param AudioFreq I2S_AUDIOFREQ_16K, I2S_AUDIOFREQ_48K, etc (48kHz frequency group)
 *
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
void start_Audio_Processing(int16_t *buf_output, int16_t *buf_input,
		uint32_t audio_dma_buf_size, uint16_t InputDevice, uint32_t AudioFreq) {

	if ((InputDevice != INPUT_DEVICE_INPUT_LINE_1)
			&& (InputDevice != INPUT_DEVICE_DIGITAL_MICROPHONE_2))
		Error_Handler();

	__HAL_RCC_SAI2_CLK_ENABLE();// bug fix syd: was not called in stm32f7xx_hal_msp.c (pb with static variable SAI2_client)

	//  Initialize WM8994 CODEC

	if (wm8994_ReadID(AUDIO_I2C_ADDRESS) != WM8994_ID) Error_Handler();
	/* Reset the Codec Registers */
	wm8994_Reset(AUDIO_I2C_ADDRESS);
	/* Initialize the codec internal registers */
	wm8994_Init(AUDIO_I2C_ADDRESS, InputDevice | OUTPUT_DEVICE_HEADPHONE, 100, AudioFreq);
	/* set lower initial volume for Line In */
	if (InputDevice == INPUT_DEVICE_INPUT_LINE_1)
		wm8994_SetVolume(AUDIO_I2C_ADDRESS, 75);
	else if (InputDevice == INPUT_DEVICE_DIGITAL_MICROPHONE_2)
		wm8994_SetVolume(AUDIO_I2C_ADDRESS, 200);
	// unmute CODEC output
	wm8994_SetMute(AUDIO_I2C_ADDRESS, AUDIO_MUTE_OFF);

	//  Start DMA transfers

	/* Start Recording */
	HAL_SAI_Receive_DMA(&hsai_BlockB2, (uint8_t*) buf_input, audio_dma_buf_size);
	/* Start Playback */
	HAL_SAI_Transmit_DMA(&hsai_BlockA2, (uint8_t*) buf_output, audio_dma_buf_size);

}

// work underway (so far the audio input choice b/w Mic and Line is carried out in main.c: MX_SAI2_Init())
void setInput(uint16_t InputDevice) {

	if ((InputDevice != INPUT_DEVICE_INPUT_LINE_1) && (InputDevice != INPUT_DEVICE_DIGITAL_MICROPHONE_2)) Error_Handler();

	HAL_SAI_Abort(&hsai_BlockB2);
	HAL_SAI_DeInit(&hsai_BlockB2);

	if (InputDevice == INPUT_DEVICE_DIGITAL_MICROPHONE_2) hsai_BlockB2.SlotInit.SlotActive = CODEC_AUDIOFRAME_MICIN; // dig mic 2
	else if (InputDevice == INPUT_DEVICE_INPUT_LINE_1) hsai_BlockB2.SlotInit.SlotActive = CODEC_AUDIOFRAME_LINEIN; // line in

	HAL_SAI_Init(&hsai_BlockB2);
	__HAL_SAI_ENABLE(&hsai_BlockB2);

	// TODO : HAL_SAI_Receive_DMA(&hsai_BlockB2, (uint8_t*)buf_input, audio_dma_buf_size);
}


/*------------------------------------------------------------------------------
 SAI DMA Callbacks
 ----------------------------------------------------------------------------*/

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai) {

	if (hsai == &hsai_BlockA2)
		printf("DMA Out error\n");
	else if (hsai == &hsai_BlockB2)
		printf("DMA In error\n");
}

// other callbacks are in audio_processing.c TODO : move here


