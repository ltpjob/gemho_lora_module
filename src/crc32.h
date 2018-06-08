#ifndef _CRC32_H_
#define _CRC32_H_

#include "stm32f10x.h"

uint32_t get_crc32(uint32_t crcinit, uint8_t * bs, uint32_t bssize);



#endif


