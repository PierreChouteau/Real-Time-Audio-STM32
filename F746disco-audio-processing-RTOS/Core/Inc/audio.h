/*
 * audio_processing.h
 *
 *  Created on: May 17, 2021
 *      Author: sydxrey
 */

#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

#include "stdint.h"


void audioLoop();
void calculateFFT(int16_t *buff_in);

#endif /* INC_AUDIO_H_ */
