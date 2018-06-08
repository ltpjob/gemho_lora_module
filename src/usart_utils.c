#include "time_utils.h"
#include "usart_utils.h"
#include <rtdevice.h>


void *usart_init(const char *name, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity)
{
	rt_device_t hDev = rt_device_find(name);
	if (hDev != RT_NULL)
	{
		if (RT_EOK == rt_device_open(hDev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX |
                          RT_DEVICE_FLAG_INT_TX))
		{
			if(usart_configure(hDev, USART_BaudRate, stopbig, parity) != 0)
			{
				rt_device_close(hDev);
				hDev = NULL;
			}
		}
		else
		{
			hDev = NULL;
		}

	}
  
  return hDev;
}

int usart_configure(void *USARTx, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity)
{
	if(USARTx == NULL)
		return -1;
	
	rt_device_t hDev = (rt_device_t)USARTx;
	
	uint16_t USART_StopBits;    
	uint16_t USART_Parity;      
	uint16_t USART_WordLength; 
  
  USART_WordLength = DATA_BITS_8;
  
  switch(stopbig)
  {
  case 1:
    USART_StopBits = STOP_BITS_1;
    break;
    
  case 2:
    USART_StopBits = STOP_BITS_2;
    break;
    
  default:
    USART_StopBits = STOP_BITS_1;
  }
  
  switch(parity)
  {
  case 0:
    USART_Parity = PARITY_NONE;
    break;
    
  case 1:
    USART_Parity = PARITY_EVEN;
    USART_WordLength = DATA_BITS_9;
    break;
    
  case 2:
    USART_Parity = PARITY_ODD;
    USART_WordLength = DATA_BITS_9;
    break;
    
  default:
    USART_Parity = PARITY_NONE;
  }
	
	struct serial_configure cfg;
	int ret = 0;
	
	cfg.baud_rate = USART_BaudRate;
	cfg.stop_bits = USART_StopBits;
	cfg.parity = USART_Parity;
	cfg.data_bits = USART_WordLength;
	cfg.bufsz = RT_SERIAL_RB_BUFSZ;
	
	ret = rt_device_control(hDev, RT_DEVICE_CTRL_CONFIG, &cfg);
	
	return ret;
}

int usart_write(void* USARTx, const void *d, size_t len)
{
	if(USARTx == NULL)
		return -1;
	
	rt_device_t hDev = (rt_device_t)USARTx;
	int ret = 0;
	
	ret = rt_device_write(hDev, 0, d, len);
  
  return ret;
}

int usart_read(void* USARTx, void *d, size_t len, int timeout)
{
	if(USARTx == NULL)
		return -1;
	
	rt_device_t hDev = (rt_device_t)USARTx;
  size_t cnt = 0;
  uint8_t *data = d;
	int ret = 0;
  
  uint64_t uLastRcvTime = get_timestamp();
  
  while(1)
  {
		ret = rt_device_read(hDev, 0, data+cnt, len-cnt);
		if(ret > 0)
		{
			uLastRcvTime = get_timestamp();
			cnt+=ret;
		}
		else if(timeout != 0)
			rt_thread_delay(1);
    
    if((timeout != -1 && get_timestamp()-uLastRcvTime >= timeout) || cnt >= len)
      break;
  }
  
  return cnt;
}



