#include "utils.h"
#include <stdio.h>
#include "stm32f10x.h"
#include <string.h>

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len)  
{  
    char *cur, *last;  
    const char *cl = (const char *)l;  
    const char *cs = (const char *)s;  
   
    /* we need something to compare */  
    if (l_len == 0 || s_len == 0)  
        return NULL;  
   
    /* "s" must be smaller or equal to "l" */  
    if (l_len < s_len)  
        return NULL;  
   
    /* special case where s_len == 1 */  
    if (s_len == 1)  
        return memchr(l, (int)*cs, l_len);  
   
    /* the last position where its possible to find "s" in "l" */  
    last = (char *)cl + l_len - s_len;  
   
    for (cur = (char *)cl; cur <= last; cur++)  
        if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)  
            return cur;  
   
    return NULL;  
}  

int checkConfigIPORT(char *pIPort, int *pOutIp, int *pOutPort)
{
  int checkFlag = 0;
  
  if (sscanf(pIPort, "%d.%d.%d.%d,%d",
             &pOutIp[0], &pOutIp[1], &pOutIp[2], &pOutIp[3], pOutPort) == 5)
  {
    for(int i=0; i<4; i++)
    {
      if(pOutIp[i] > 255 || pOutIp[i] < 0)
      {
        checkFlag = -1;
        break;
      }   
    }
    
    if(*pOutPort<0 || *pOutPort>65535)
      checkFlag = -1;
  }
  else
    checkFlag = -1;
  
  return checkFlag;
}

int checkConfigAPPID(char *pAPPID, uint32_t *pOutAPPID)
{
  int checkFlag = 0;
  
  if (sscanf(pAPPID, "%08X", pOutAPPID) != 1)
  {
    checkFlag = -1;
  }

  return checkFlag;
}

int checkConfigSPD(char *pSpeed, uint32_t *pOutSpeed)
{
  int checkFlag = 0;
  
  if (sscanf(pSpeed, "%d", pOutSpeed) != 1)
  {
    checkFlag = -1;
  }
  else
  {
    if(*pOutSpeed>10 || *pOutSpeed<1)
    {
      checkFlag = -1;
    }
  }

  return checkFlag;
}

int checkConfigCHN(char *pChn, uint32_t *pOutChn)
{
  int checkFlag = 0;
  
  if (sscanf(pChn, "%d", pOutChn) != 1)
  {
    checkFlag = -1;
  }
  else
  {
    if(*pOutChn>127)
    {
      checkFlag = -1;
    }
  }

  return checkFlag;
}

int checkConfigNID(char *pNID, uint32_t *pOutNID)
{
  int checkFlag = 0;
  
  if (sscanf(pNID, "%08X", pOutNID) != 1)
  {
    checkFlag = -1;
  }

  return checkFlag;
}

int checkConfigRS232(char *pIPort, uint32_t *pOutBaudrate, uint8_t *pOutStopBit, uint8_t *pOutParity)
{
  int checkFlag = 0;
  uint32_t baudrate;
  uint32_t stopbit;
  uint32_t parity;
  
  if (sscanf(pIPort, "%d,%d,%d", &baudrate, &stopbit, &parity) == 3)
  { 
    *pOutBaudrate = baudrate;
    *pOutStopBit = stopbit;
    *pOutParity = parity;

    if(*pOutBaudrate != 4800 &&
       *pOutBaudrate != 9600 &&
         *pOutBaudrate != 57600 &&
           *pOutBaudrate != 115200)
    {
      checkFlag = -1;
    }
    
    if(*pOutStopBit != 1 &&
       *pOutStopBit != 2)
    {
      checkFlag = -1;
    }
    
    if(*pOutParity != 0 &&
       *pOutParity != 1 &&
         *pOutParity != 2)
    {
      checkFlag = -1;
    }

  }
  else
    checkFlag = -1;
  
  return checkFlag;
}

