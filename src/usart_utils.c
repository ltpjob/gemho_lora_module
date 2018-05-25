#include "usart_utils.h"
#include "time_utils.h"


int usart_init(USART_TypeDef* USARTx, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity)
{
  USART_InitTypeDef USART_InitStructure;
  uint16_t USART_StopBits;    
  uint16_t USART_Parity;      
  uint16_t USART_WordLength; 
  
  USART_WordLength = USART_WordLength_8b;
  
  switch(stopbig)
  {
  case 1:
    USART_StopBits = USART_StopBits_1;
    break;
    
  case 2:
    USART_StopBits = USART_StopBits_2;
    break;
    
  default:
    USART_StopBits = USART_StopBits_1;
  }
  
  switch(parity)
  {
  case 0:
    USART_Parity = USART_Parity_No;
    break;
    
  case 1:
    USART_Parity = USART_Parity_Even;
    USART_WordLength = USART_WordLength_9b;
    break;
    
  case 2:
    USART_Parity = USART_Parity_Odd;
    USART_WordLength = USART_WordLength_9b;
    break;
    
  default:
    USART_Parity = USART_Parity_No;
  }
  
  USART_InitStructure.USART_BaudRate = USART_BaudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength;
  USART_InitStructure.USART_StopBits = USART_StopBits;
  USART_InitStructure.USART_Parity = USART_Parity;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
  
  USART_Cmd(USARTx, DISABLE);
  USART_Init(USARTx, &USART_InitStructure);
  USART_Cmd(USARTx, ENABLE);
  
  return 0;
}

int usart_write(USART_TypeDef* USARTx, const void *d, size_t len)
{
  const uint8_t *data = d;
  
  for(int i=0; i<len; i++)
  {
    USART_SendData(USARTx, data[i]);
    while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET){}
  }
  
  return len;
}

int usart_read(USART_TypeDef* USARTx, void *d, size_t len, int timeout)
{
  size_t cnt = 0;
  uint8_t *data = d;
  
  uint64_t uLastRcvTime = get_timestamp();
  
  while(1)
  {
    if(USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) != RESET)
    {
      uLastRcvTime = get_timestamp();
      data[cnt++] = USART_ReceiveData(USARTx);
    }
    
    if((timeout != -1 && get_timestamp()-uLastRcvTime >= timeout) || cnt >= len)
      break;
  }
  
  return cnt;
}




