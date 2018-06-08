#ifndef __MSG_FIFO_H
#define __MSG_FIFO_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdio.h>	 

void *msg_init(uint16_t msgSize, uint16_t fifoSize);
	 
int32_t msg_destory(void *h);

int32_t msg_push(void *h, void *msgData, uint16_t msgSize);
	 
int32_t msg_pop(void *h, void *msgData, uint16_t *msgSize, rt_int32_t timeout);


#ifdef __cplusplus
 }
#endif

#endif /* __MSG_FIFO_H */



