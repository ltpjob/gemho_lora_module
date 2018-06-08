#ifndef __CONFIG_UTILS_H
#define __CONFIG_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif 
  
#include "gemho_lora_module.h"
  
void config_init(void);

int save_config(const loraModu_config *pConfig);

int read_config(loraModu_config *pConfig);

void config_init(void);
  
#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_UTILS_H */


