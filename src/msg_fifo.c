#include <stdio.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "msg_fifo.h"


typedef struct tag_MsgUint
{
  uint16_t msgSize;
  uint8_t *msgData;
}MsgUint;


typedef struct tag_MsgHandle
{
	struct rt_data_queue pop_queue;
	struct rt_data_queue push_queue;
	uint16_t msgSize;
	uint16_t fifoSize;
}MsgHandle;


void *msg_init(uint16_t msgSize, uint16_t fifoSize)
{
	MsgHandle *handle = NULL;
	
	handle = rt_malloc(sizeof(MsgHandle));
	
	if(handle == NULL)
		goto __err1;
	
	handle->msgSize = msgSize;
	handle->fifoSize = fifoSize;
	rt_data_queue_init(&handle->pop_queue, fifoSize, 0, RT_NULL);
	rt_data_queue_init(&handle->push_queue, fifoSize, 0, RT_NULL);
	
	for(int i=0; i<fifoSize; i++)
	{
		MsgUint *mu = rt_malloc(sizeof(MsgUint));
		if(mu == NULL)
			goto __err2;

		rt_data_queue_push(&handle->push_queue, mu, sizeof(*mu), RT_WAITING_NO);
		mu->msgSize = 0;
		mu->msgData = rt_malloc(msgSize);
		if(mu->msgData == NULL)
			goto __err2;
	}
	
	return handle;

__err2:
	while(1)
	{
		MsgUint *mu = NULL;
		rt_size_t size = 0;
		if(rt_data_queue_pop(&handle->push_queue, (const void **)&mu, &size, RT_WAITING_NO) == RT_EOK)
		{
			if(mu != NULL)
			{
				if(mu->msgData != NULL)
				{
					rt_free(mu->msgData);
				}
				rt_free(mu);
			}
		}
		else
			break;
	}
	
	rt_free(handle);
	
__err1:
		
	return NULL;
}

int32_t msg_destory(void *h)
{
	if(h == NULL)
		return -1;
	
	MsgHandle *handle = h;
	
	while(1)
	{
		MsgUint *mu = NULL;
		rt_size_t size = 0;
		if(rt_data_queue_pop(&handle->push_queue, (const void **)&mu, &size, RT_WAITING_NO) == RT_EOK)
		{
			if(mu != NULL)
			{
				if(mu->msgData != NULL)
				{
					rt_free(mu->msgData);
				}
				rt_free(mu);
			}
		}
		else
			break;
	}
	
	rt_free(handle);
	
	return 0;
}

int32_t msg_push(void *h, void *msgData, uint16_t msgSize)
{
	if(h == NULL)
		return -1;
		
	MsgHandle *handle = h;
	MsgUint *mu = NULL;
	rt_size_t size = 0;
	
	if(msgSize > handle->msgSize)
	{
		msgSize = handle->msgSize;
	}
	
	if(rt_data_queue_pop(&handle->push_queue, (const void **)&mu, &size, RT_WAITING_NO) == RT_EOK)
	{
		if(mu == NULL)
			return -2;
	}
	else if(rt_data_queue_pop(&handle->pop_queue, (const void **)&mu, &size, RT_WAITING_NO) == RT_EOK)
	{
		if(mu == NULL)
			return -3;
	}
	else
	{
		return -4;
	}
	
	memcpy(mu->msgData, msgData, msgSize);
	mu->msgSize = msgSize;
	
	rt_data_queue_push(&handle->pop_queue, mu, sizeof(*mu), RT_WAITING_NO);
	
	return 0;
}


int32_t msg_pop(void *h, void *msgData, uint16_t *msgSize, rt_int32_t timeout)
{
	if(h == NULL)
		return -1;
		
	MsgHandle *handle = h;
	MsgUint *mu = NULL;
	rt_size_t size = 0;
	rt_err_t ret = 0;
	
	ret = rt_data_queue_pop(&handle->pop_queue, (const void **)&mu, &size, timeout);
	
	if(ret == -RT_ETIMEOUT)
	{
		return -RT_ETIMEOUT;
	}
	else if(ret == 0)
	{
		if(mu == NULL)
			return -2;
	}
	else
	{
		return -3;
	}
	
	memcpy(msgData, mu->msgData, mu->msgSize);
	*msgSize = mu->msgSize;

	mu->msgSize = 0;
	rt_data_queue_push(&handle->push_queue, mu, sizeof(*mu), RT_WAITING_NO);
	
	return 0;
}


