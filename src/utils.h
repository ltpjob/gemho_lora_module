#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>
#include <stdint.h>

 
void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);
   
int checkConfigIPORT(char *pIPort, int *pOutIp, int *pOutPort);

int checkConfigAPPID(char *pAPPID, uint32_t *pOutAPPID);

int checkConfigSPD(char *pSpeed, uint32_t *pOutSpeed);

int checkConfigCHN(char *pChn, uint32_t *pOutChn);

int checkConfigNID(char *pNID, uint32_t *pOutNID);

int checkConfigRS232(char *pIPort, uint32_t *pOutBaudrate, uint8_t *pOutStopBit, uint8_t *pOutParity);
   
#ifdef __cplusplus
 }
#endif

#endif /* __UTILS_H */