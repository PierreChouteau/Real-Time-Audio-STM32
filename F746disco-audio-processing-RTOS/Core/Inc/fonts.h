/*
 * fonts.h
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */

#ifndef INC_FONTS_H_
#define INC_FONTS_H_


#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

typedef struct _tFont
{
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;

} sFONT;

extern sFONT Font24;
extern sFONT Font20;
extern sFONT Font16;
extern sFONT Font12;
extern sFONT Font8;

#define LINE(x) ((x) * (((sFONT *)pFont)->Height))


#ifdef __cplusplus
}
#endif



#endif /* INC_FONTS_H_ */
