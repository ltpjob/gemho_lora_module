#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include <string.h>
#include "gemho_lora_module.h"
#include "utils.h"
#include "time_utils.h"
#include "usart_utils.h"
#include "config_utils.h"
#include <rtthread.h>
#include "msg_fifo.h"

static void *USERCOM = NULL;
static void *L101CCOM = NULL;


static const loraModu_config l_default_loraMCFG = 
{
  .aid = 0x01,
  .spd = 5,
  .ch = 72,
  .baudrate = 115200,
  .stopbit = 1,
  .parity = 0,
	.watchdog = 0,
};


static loraModu_config l_loraModuConfig;

static uint32_t l_nid = 0;

//初始载入配置
int load_config()
{
  int ret = 0;
  
  ret = read_config(&l_loraModuConfig);
  
  if(ret != 0)
  {
    memcpy(&l_loraModuConfig, &l_default_loraMCFG, sizeof(l_loraModuConfig));
  }
  
  return ret;
}

void L101C_reset()
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_10);
  delay_ms(10);
  GPIO_SetBits(GPIOB, GPIO_Pin_10);
}

int wait_OK_noEnter(int timeout)
{
  int ret = 0;
  char buf[128] = "";
  
  usart_read(L101CCOM, buf, sizeof(buf), timeout);
  
  if(memmem(buf, sizeof(buf), OKSTRNOENTER, strlen(OKSTRNOENTER)) != NULL)
  {
    ret = 0;
  }
  else
  {
    ret = -1;
  }

  return ret;
}


int wait_OK(int timeout)
{
  int ret = 0;
  char buf[128] = "";
  
  usart_read(L101CCOM, buf, sizeof(buf), timeout);
  
  if(memmem(buf, sizeof(buf), OKSTR, strlen(OKSTR)) != NULL)
  {
    ret = 0;
  }
  else
  {
    ret = -1;
  }

  return ret;
}

int L101C_inAT(int loopTime, int timeout)
{
  int status = 0;
  
  status = -1;
  for(int i=0; i<loopTime; i++)
  {
    int len = 0;
    int loopcount = 10;
    char buf[128] = "";
    
    L101C_reset();
    while(loopcount-- && usart_read(L101CCOM, buf, sizeof(buf), timeout));
    
    usart_write(L101CCOM, "+++", strlen("+++"));
    memset(buf, 0, sizeof(buf));
    len = usart_read(L101CCOM, buf, sizeof(buf), timeout);
    if(len == 1 && buf[0] == 'a')
    {
      usart_write(L101CCOM, "a", strlen("a"));
      if(wait_OK_noEnter(timeout) == 0)
      {
        status = 0;
        break;
      }
    }

  }
  
  return status;
}

int ATCMD_waitOK(char *cmd, int loopTime, int timeout)
{
  int status = 0;
  
  status = -1;
  for(int i=0; i<loopTime; i++)
  {
    usart_write(L101CCOM, cmd, strlen(cmd));
    if(wait_OK(timeout) == 0)
    {
      status = 0;
      break;
    }
  }
  
  return status;
}

//获取nid
int get_nid(uint32_t *nid)
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  char buf[128] = "";
  
  usart_write(L101CCOM, NIDGET, strlen(NIDGET));
  
  ret = -1;
  if(usart_read(L101CCOM, buf, sizeof(buf), 300) > 0)
  {
    pStart = memmem(buf, sizeof(buf), NIDRTN, strlen(NIDRTN));
    pEnd = memmem(buf, sizeof(buf), ENDOK, strlen(ENDOK));
    
    if(pStart != NULL && pEnd != NULL)
    {
      uint32_t id = 0;
      if(checkConfigNID((char *)pStart+strlen(NIDRTN), &id) == 0)
      {
        *nid = id;
        ret = 0;
      }
    }
  }
  
  return ret;
}


static int ATRS232_cmd(char *cmd, int len)
{
  return 0;
}


static int ATSPEED_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATSPEED) == 0)
  {
    snprintf(buf, sizeof(buf), "+SPEED:%d\r\n", l_loraModuConfig.spd);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATSPEEDEQ, strlen(ATSPEEDEQ)) == 0)
  {
    uint32_t speed;
    
    if(checkConfigSPD(buf+strlen(ATSPEEDEQ), &speed) == 0)
    {
      l_loraModuConfig.spd = speed;
      
      usart_write(USERCOM, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;

}

static int ATCHN_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATCHN) == 0)
  {
    snprintf(buf, sizeof(buf), "+CHN:%d\r\n", l_loraModuConfig.ch);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATCHNEQ, strlen(ATCHNEQ)) == 0)
  {
    uint32_t speed;
    
    if(checkConfigSPD(buf+strlen(ATCHNEQ), &speed) == 0)
    {
      l_loraModuConfig.spd = speed;
      
      usart_write(USERCOM, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
}


static int ATAPPID_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATAPPID) == 0)
  {
    snprintf(buf, sizeof(buf), "+APPID:%08X\r\n", l_loraModuConfig.aid);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATAPPIDEQ, strlen(ATAPPIDEQ)) == 0)
  {
    uint32_t appid;
    
    if(checkConfigAPPID(buf+strlen(ATAPPIDEQ), &appid) == 0)
    {
      l_loraModuConfig.aid = appid;
      
      usart_write(USERCOM, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
  
}


static int ATNID_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATNID) == 0)
  {
    snprintf(buf, sizeof(buf), "+NID:%08X\r\n", l_nid);
    usart_write(USERCOM, buf, strlen(buf));
  }
  
  return 0;
}


static int ATSAVE_cmd(char *cmd, int len)
{
  int ret = 0;
  
  for(int i=0; i<3; i++)
  {
    ret = save_config(&l_loraModuConfig);
    if(ret == 0)
      break;
  }
  
  if(ret == 0)
  {
    usart_write(USERCOM, OKSTR, strlen(OKSTR));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return ret;
}


static int ATDELO_cmd(char *cmd, int len)
{
  int ret = 0;
  
  memcpy(&l_loraModuConfig, &l_default_loraMCFG, sizeof(l_loraModuConfig));
    
  for(int i=0; i<3; i++)
  {
    ret = save_config(&l_loraModuConfig);
    if(ret == 0)
      break;
  }
  
  if(ret == 0)
  {
    usart_write(USERCOM, OKSTR, strlen(OKSTR));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return ret;
}


cmdExcute cmdExe[] = 
{
  {ATRS232, ATRS232_cmd},
  {ATSPEED, ATSPEED_cmd},
  {ATCHN, ATCHN_cmd},
  {ATAPPID, ATAPPID_cmd},
  {ATNID, ATNID_cmd},
  {ATSAVE, ATSAVE_cmd},
  {ATDELO, ATDELO_cmd},
};


//时钟设置
void RCC_Configuration(void)
{
  /* Enable GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_I2C1, ENABLE);
}

//io设置
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  //USERCOM 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  //L101-C
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    //i2c1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  //Lora_REST Lora_REST1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  //led_R led_Y
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  
  //key
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  //HOST_WAKE
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  //WAKE 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
}

void RST_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource15);

  /* Configure EXTI0 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line15;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void HOSTWAKE_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

  /* Configure EXTI0 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

//BC95COM直连USERCOM
void USERCOM_direct_L101C()
{
  while(1)
  {
    char buf[64]="";
		int ret = 0;
    
		ret = usart_read(USERCOM, buf, sizeof(buf), 0);
		if(ret > 0)
		{
			usart_write(L101CCOM, buf, ret);
		}
		
		ret = usart_read(L101CCOM, buf, sizeof(buf), 0);
		if(ret > 0)
		{
			usart_write(USERCOM, buf, ret);
		}
  }
}

//gh配置
void ghConfig()
{
  int len = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  uint8_t buf[128] = "";
  int cnt = 0;
  
  
  while(1)
  {
    len = usart_read(USERCOM, buf+cnt, sizeof(buf)-cnt, 10);
    cnt += len;
    
    if(cnt == 0)
      continue;
      
    pEnd = memmem(buf, cnt, ENDFLAG, strlen(ENDFLAG));
    if(pEnd != NULL)
    {
      for(int i=0; i<sizeof(cmdExe)/sizeof(cmdExe[0]); i++)
      {
        pStart = memmem(buf, cnt, cmdExe[i].cmd, strlen(cmdExe[i].cmd));
        if(pStart == buf)
        {
          cmdExe[i].ce_fun((char *)pStart, pEnd - pStart);
          break;
        }
        else if(sizeof(cmdExe)/sizeof(cmdExe[0]) <= i+1) //未定义命令
        {
          usart_write(USERCOM, UDCMD, strlen(UDCMD));
        }
      }
      
      memset(buf, 0, sizeof(buf));
      cnt = 0;
    }
    
    if(cnt >= sizeof(buf))
    {
      memset(buf, 0, sizeof(buf));
      cnt = 0;
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
    
  }
}

//开机模式选择
ModeToRun start_mode()
{
  ModeToRun mode = lucTrans;
  uint8_t buf[1024] = "";
  int len = 0;
  
  do
  {
    len = usart_read(USERCOM, buf, sizeof(buf), 100);
  }while(len == 0);
  
  if(memmem(buf, len, ATDEBUG, strlen(ATDEBUG)) != NULL)
  {
    mode = atDebug;
    usart_write(USERCOM, ATDEBUG, strlen(ATDEBUG));
  }
  else if(memmem(buf, len, GEMHOCFG, strlen(GEMHOCFG)) != NULL)
  {
    mode = gemhoConfig;
    usart_write(USERCOM, GEMHOCFG, strlen(GEMHOCFG));
  }
  else
  {
    mode = lucTrans;
    
//    if(l_nbModuConfig.sendMode == 0)
//      coap_msgSend(l_uartBuf, l_cnt);
//    else
//      udp_msgSend(l_uartBuf, l_cnt);
  }
  
  return mode;
}


//初始配置
int config_L101C()
{
  int status = 0;
  char buf[128] = "";
  
  status = L101C_inAT(10, 20);
  status |= ATCMD_waitOK(ATECHO, 10, 50);
  
  if(status == 0)
  {
    snprintf(buf, sizeof(buf), "AT+AID=%08X\r\n", l_loraModuConfig.aid);
    if(ATCMD_waitOK(buf, 4, 100) != 0)
    {
      usart_write(USERCOM, "AID CONFIG FAIL\r\n", strlen("AID CONFIG FAIL\r\n"));
      status = -2;
    }
  }
  
  if(status == 0)
  {
    snprintf(buf, sizeof(buf), "AT+SPD=%d\r\n", l_loraModuConfig.spd);
    if(ATCMD_waitOK(buf, 4, 100) != 0)
    {
      usart_write(USERCOM, "SPD CONFIG FAIL\r\n", strlen("SPD CONFIG FAIL\r\n"));
      status = -3;
    }
  }
  
  if(status == 0)
  {
    snprintf(buf, sizeof(buf), "AT+SPD=%d\r\n", l_loraModuConfig.spd);
    if(ATCMD_waitOK(buf, 4, 100) != 0)
    {
      usart_write(USERCOM, "SPD CONFIG FAIL\r\n", strlen("SPD CONFIG FAIL\r\n"));
      status = -3;
    }
  }
  
  if(status == 0)
  {
    snprintf(buf, sizeof(buf), "AT+CH=%d\r\n", l_loraModuConfig.ch);
    if(ATCMD_waitOK(buf, 4, 100) != 0)
    {
      usart_write(USERCOM, "CH CONFIG FAIL\r\n", strlen("CH CONFIG FAIL\r\n"));
      status = -4;
    }
  }
  
  if(status == 0)
  {
    if(get_nid(&l_nid) != 0)
    {
      usart_write(USERCOM, "NID GET FAIL\r\n", strlen("NID GET FAIL\r\n"));
      status = -5;
    }
  }
  
  if(ATCMD_waitOK(ATENTM, 4, 50) != 0)
  {
    usart_write(USERCOM, "ATENTM CONFIG FAIL\r\n", strlen("ATENTM CONFIG FAIL\r\n"));
    status = -6;
  }
  
  if(status == 0)
  {
    usart_write(USERCOM, "INIT OK\r\n", strlen("INIT OK\r\n"));
    
  }
 
  return status;
}

static void main_entry(void *args)
{
	RCC_Configuration();
	GPIO_Configuration();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	tick_ms_init();
	RST_Configuration();
	config_init();

	//led light off
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	GPIO_SetBits(GPIOC, GPIO_Pin_14);

	GPIO_SetBits(GPIOB, GPIO_Pin_11);     //reload拉高
	GPIO_SetBits(GPIOA, GPIO_Pin_1);    //wake拉低

	load_config();

	L101CCOM = usart_init("uart2", L101CBAUDRATE, 1, 0);
	USERCOM = usart_init("uart1", l_loraModuConfig.baudrate, l_loraModuConfig.stopbit, l_loraModuConfig.parity);

	config_L101C();

	ModeToRun mode = start_mode();

	if(mode == atDebug)
	{
		USERCOM_direct_L101C();
	}
	else if(mode == gemhoConfig)
	{
		ghConfig();
	}

	HOSTWAKE_Configuration();


}

int main(void)
{
	
	rt_thread_t ht_main = rt_thread_create("main_entry", main_entry, RT_NULL, 2048, 2, rt_tick_from_millisecond(10));
	if (ht_main!= RT_NULL)
		rt_thread_startup(ht_main);
	
	while(1)
	{
		rt_thread_delay(rt_tick_from_millisecond(2*1000));
	}
}

void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line15) != RESET)
  {
    int count = 16;
    if(save_config(&l_default_loraMCFG) == 0)
    {
      while(count--)
      {
        GPIO_ResetBits(GPIOC, GPIO_Pin_14);
        delay_ms(100);
        GPIO_SetBits(GPIOC, GPIO_Pin_14);
        delay_ms(100);
      }
    }

    EXTI_ClearITPendingBit(EXTI_Line15);
  }

}

void EXTI0_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line0) != RESET)
  {


    EXTI_ClearITPendingBit(EXTI_Line0);
  }

}




