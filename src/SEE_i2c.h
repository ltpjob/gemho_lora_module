#ifndef _SEE_I2C_H_
#define _SEE_I2C_H_

#include "stm32f10x_conf.h"

#define I2C_SPEED 200000
#define sEE_I2C I2C1
#define I2C_SLAVE_ADDRESS7 0x10
#define sEE_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define sEE_LONG_TIMEOUT         ((uint32_t)(10 * sEE_FLAG_TIMEOUT))
#define sEE_MAX_TRIALS_NUMBER     150
#define sEE_OK                    0
#define sEE_FAIL                  1   
  

void SEE_i2c_init();

uint32_t sEE_WaitEepromStandbyState(void);

uint32_t SEE_i2c_write(uint8_t wData, uint16_t WriteAddr);

uint32_t SEE_i2c_read(uint8_t* rData, uint16_t ReadAddr);


#endif