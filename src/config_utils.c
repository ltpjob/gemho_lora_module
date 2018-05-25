#include <stdio.h>
#include "SEE_i2c.h"
#include "crc32.h"
#include "config_utils.h"
#include <string.h>

void config_init()
{
  SEE_i2c_init();
}

int save_config(const loraModu_config *pConfig)
{
  int ret = 0;
  confSaveUnit cfgUnit;
  uint8_t *pcfg = (uint8_t *)&cfgUnit;
  
  if(pConfig == NULL)
    return -1;
  
  memcpy(&cfgUnit.config, pConfig, sizeof(cfgUnit.config));
  
  cfgUnit.crc32 = get_crc32(0, (uint8_t *)&cfgUnit.config, sizeof(cfgUnit.config));
  
  for(int i=0; i<sizeof(confSaveUnit); i++)
  {
    ret = SEE_i2c_write(pcfg[i], i);
    if(ret != 0)
      break;
  }
  
  return ret;
}


int read_config(loraModu_config *pConfig)
{
  int ret = 0;
  confSaveUnit cfgUnit;
  uint8_t *pcfg = (uint8_t *)&cfgUnit;
  
  if(pConfig == NULL)
    return -1;
  
  for(int i=0; i<sizeof(cfgUnit); i++)
  {
    ret = SEE_i2c_read(&pcfg[i], i);
    if(ret != 0)
      break;
  }
  
  if(ret == 0)
  {
    if(get_crc32(0, (uint8_t *)&cfgUnit.config, sizeof(cfgUnit.config)) == cfgUnit.crc32)
      memcpy(pConfig, &cfgUnit.config, sizeof(cfgUnit.config));
    else
      ret = -1;
  }
  
  return ret;
}




