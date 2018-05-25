#ifndef __USART_UTILS_H
#define __USART_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>
#include "stm32f10x.h"


int usart_init(USART_TypeDef* USARTx, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity);
 
int usart_write(USART_TypeDef* USARTx, const void *d, size_t len);

int usart_read(USART_TypeDef* USARTx, void *d, size_t len, int timeout);
   
   
   
   
   
   
#ifdef __cplusplus
 }
#endif

#endif /* __USART_UTILS_H */