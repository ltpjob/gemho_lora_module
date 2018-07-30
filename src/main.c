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
static void *l_hMsgFifo = NULL;
static rt_sem_t l_sem_urx = RT_NULL;
static rt_sem_t l_sem_key = RT_NULL;
static rt_mq_t  l_mq_midLight = RT_NULL;
static rt_mq_t  l_mq_sendTiming = RT_NULL;
static DeviceStatus l_devStatus = DEVICEOK;

static const loraModu_config l_default_loraMCFG = 
{
  .aid = 0x01,
  .spd = 5,
  .ch = 72,
  .baudrate = 115200,
  .stopbit = 1,
  .parity = 0,
	.watchdog = 0,
	.msgSave = 5,
};


static loraModu_config l_loraModuConfig;

static uint32_t l_nid = 0;

//初始载入配置
static int load_config()
{
  int ret = 0;
  
  ret = read_config(&l_loraModuConfig);
  
  if(ret != 0)
  {
    memcpy(&l_loraModuConfig, &l_default_loraMCFG, sizeof(l_loraModuConfig));
  }
  
  return ret;
}

static void L101C_reset()
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_10);
  delay_ms(10);
  GPIO_SetBits(GPIOB, GPIO_Pin_10);
}

static int wait_OK_noEnter(int timeout)
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


static int wait_OK(int timeout)
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

static int L101C_inAT(int loopTime, int timeout)
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

static int ATCMD_waitOK(char *cmd, int loopTime, int timeout)
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

//获取版本号
static int ATVER_cmd(char *cmd, int len)
{
	char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
	
	memcpy(buf, cmd, len);
	
	if(strcmp(buf, ATVER) == 0)
  {
    snprintf(buf, sizeof(buf), "+VER:%s\r\n", VERSION);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
	
	return 0;
}

//获取nid
static int get_nid(uint32_t *nid)
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

static int ATMSGS_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATMSGS) == 0)
  {
    snprintf(buf, sizeof(buf), "+MSGS:%d\r\n", 
             l_loraModuConfig.msgSave);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATMSGSEQ, strlen(ATMSGSEQ)) == 0)
  {
    int msgSave = -1;

    if(sscanf(buf+strlen(ATMSGSEQ), "%d", &msgSave) == 1)
    {
      if(msgSave <1 || msgSave>5)
      {
        usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
      }
      else
      {
        l_loraModuConfig.msgSave = msgSave;
        usart_write(USERCOM, OKSTR, strlen(OKSTR));
      }
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
    uint32_t chn;
    
    if(checkConfigCHN(buf+strlen(ATCHNEQ), &chn) == 0)
    {
      l_loraModuConfig.ch = chn;
      
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

//设置看门狗使能
static int ATWDT_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATWDT) == 0)
  {
    snprintf(buf, sizeof(buf), "+WDT:%d\r\n", 
             l_loraModuConfig.watchdog);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATWDTEQ, strlen(ATWDTEQ)) == 0)
  {
    int watchdog = -1;

    if(sscanf(buf+strlen(ATWDTEQ), "%d", &watchdog) == 1)
    {
      if(watchdog <0 || watchdog>1)
      {
        usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
      }
      else
      {
        l_loraModuConfig.watchdog = watchdog;
        usart_write(USERCOM, OKSTR, strlen(OKSTR));
      }
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


static cmdExcute cmdExe[] = 
{
	{ATVER, ATVER_cmd},
  {ATRS232, ATRS232_cmd},
	{ATMSGS, ATMSGS_cmd},
  {ATSPEED, ATSPEED_cmd},
  {ATCHN, ATCHN_cmd},
  {ATAPPID, ATAPPID_cmd},
  {ATNID, ATNID_cmd},
	{ATWDT, ATWDT_cmd},
  {ATSAVE, ATSAVE_cmd},
  {ATDELO, ATDELO_cmd},
};


//时钟设置
static void RCC_Configuration(void)
{
  /* Enable GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_I2C1, ENABLE);
}

//io设置
static void GPIO_Configuration(void)
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

static void RST_Configuration(void)
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

static void HOSTWAKE_Configuration(void)
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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

//BC95COM直连USERCOM
static void USERCOM_direct_L101C()
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
static void ghConfig()
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
static ModeToRun start_mode()
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
static int config_L101C()
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

static int setDevicStatus(DeviceStatus status)
{
	l_devStatus = status;
	
	return 0;
}

static void wdt_entry(void *args)
{
	int wdt_en = l_loraModuConfig.watchdog;
	
	if(wdt_en == 1)
	{
		IWDG_Init(6, 0xfff);
	}
	
	while(1)
	{
		if(wdt_en == 1)
		{
			IWDG_Feed();
		}
		
		rt_thread_delay(rt_tick_from_millisecond(10*1000));
	}
}

//控制中间的灯
static int midLight_action(uint32_t interval_time, 
	uint32_t blink_times, uint32_t stop_time, uint32_t urgent)
{
	lightAction la;
	int ret = 0;
	
	la.interval_time = interval_time;
	la.blink_times = blink_times;
	la.stop_time = stop_time;
	
	if(urgent == 1)
	{
		ret = rt_mq_urgent(l_mq_midLight, &la, sizeof(la));
		if(ret != RT_EOK)
		{
			lightAction tmp;
			rt_mq_recv(l_mq_midLight, &tmp, sizeof(tmp), RT_WAITING_NO);
			ret = rt_mq_urgent(l_mq_midLight, &la, sizeof(la));
		}
	}
	else
	{
		ret = rt_mq_send(l_mq_midLight, &la, sizeof(la));
	}
	
	return ret;
}

static DeviceStatus getDevicStatus()
{
	return l_devStatus;
}

//按键响应线程
static void key_entry(void *args)
{
	l_sem_key = rt_sem_create("l_sem_key", 0, RT_IPC_FLAG_PRIO);
	RST_Configuration();
	
	while(1)
	{
		if(rt_sem_take(l_sem_key, rt_tick_from_millisecond(1000)) == RT_EOK)
		{
			uint64_t uTouchDownTime = get_timestamp();
			uint64_t uHoldTime = 0;
			while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == Bit_RESET)
			{
				rt_thread_delay(rt_tick_from_millisecond(50));
			}
			
			uHoldTime = get_timestamp() - uTouchDownTime + 1;
			if(uHoldTime>5*1000 && uHoldTime<10*1000)
			{
				if(save_config(&l_default_loraMCFG) == 0)
				{
					//控制灯闪16下
					midLight_action(100, 16, 1000, 1);
				}
			}
			else if(uHoldTime>0 && uHoldTime<1*500)
			{
				msg_push(l_hMsgFifo, KPMFT, strlen(KPMFT));
			}
		}
	}
}

//中间灯闪烁管理
static void midLight_entry(void *args)
{
	while(1)
	{
		lightAction la;
		if(rt_mq_recv(l_mq_midLight, &la, sizeof(la), rt_tick_from_millisecond(1000)) == RT_EOK)
		{
			for(int i=0; i<la.blink_times; i++)
			{
				GPIO_ResetBits(GPIOC, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(la.interval_time));
				GPIO_SetBits(GPIOC, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(la.interval_time));
			}
			rt_thread_delay(rt_tick_from_millisecond(la.stop_time));
		}
	}
}

//状态表示线程
static void status_entry(void *args)
{
	while(1)
	{
		DeviceStatus status = getDevicStatus();
		int blinkInter = 200;
		
		if(status == DEVICEOK)
		{
			GPIO_SetBits(GPIOC, GPIO_Pin_13);
		}
		else if(status == L101CDONTWORK)
		{
			for(int i=0; i<1; i++)
			{
				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
			}
		}
		
		rt_thread_delay(rt_tick_from_millisecond(1500));
	}
		
}

//消息发送线程
static void thread_msgSend(void *args)
{
	l_mq_sendTiming = rt_mq_create("l_mq_sendTiming", sizeof(sendTimeing), 10, RT_IPC_FLAG_PRIO);
	HOSTWAKE_Configuration();
	while(1)
	{
		sendTimeing stiming;
		if(rt_mq_recv(l_mq_sendTiming, &stiming, sizeof(stiming), rt_tick_from_millisecond(1000)) == RT_EOK)
		{
			uint8_t msgData[512] = "";
			uint16_t msgSize = 0;
			int ret = 0;
			
			if(get_timestamp() - stiming.irq_timeStamp >= 5)
			{
				continue;
			}
//			else
//			{
//				char buf[64] = "";
//				snprintf(buf, sizeof(buf), "tmpTime:%llu\r\n", get_timestamp() - stiming.irq_timeStamp);
//				usart_write(USERCOM, buf, strlen(buf));
//			}
			
			ret = msg_pop(l_hMsgFifo, &msgData, &msgSize, RT_WAITING_NO);
//			char buf[64] = "";
//			snprintf(buf, sizeof(buf), "msgSize:%d\r\n", msgSize);
//			usart_write(USERCOM, buf, strlen(buf));
			if(ret == 0)
			{
				usart_write(L101CCOM, msgData, msgSize);
				midLight_action(50, 8, 300, 0);
			}
		}
		
	}
}

static rt_err_t urx_input(rt_device_t dev, rt_size_t size)
{
	
	rt_sem_release(l_sem_urx);
	
	return RT_EOK;
} 

static void main_entry(void *args)
{


	L101CCOM = usart_init("uart2", L101CBAUDRATE, 1, 0);
	USERCOM = usart_init("uart1", l_loraModuConfig.baudrate, l_loraModuConfig.stopbit, l_loraModuConfig.parity);

	if(config_L101C() != 0)
	{
		setDevicStatus(L101CDONTWORK);
	}
	
	midLight_action(2000, 1, 200, 1);
	
	l_hMsgFifo = msg_init(MSG_MAXLEN, l_loraModuConfig.msgSave);
	rt_thread_t ht_msgSend = rt_thread_create("thread_msgSend", thread_msgSend, RT_NULL, 1024+512, 3, rt_tick_from_millisecond(10));
	if (ht_msgSend!= RT_NULL)
		rt_thread_startup(ht_msgSend);

	ModeToRun mode = start_mode();

	if(mode == atDebug)
	{
		USERCOM_direct_L101C();
	}
	else if(mode == gemhoConfig)
	{
		ghConfig();
	}
	
	l_sem_urx = rt_sem_create("l_sem_urx", 0, RT_IPC_FLAG_PRIO);
	rt_device_set_rx_indicate(USERCOM, urx_input);
	
	while(1)
	{
		if(rt_sem_take(l_sem_urx, rt_tick_from_millisecond(200)) == RT_EOK)
		{
			int len;
			char buf[512] = "";
			
			len = usart_read(USERCOM, buf, sizeof(buf), 100);
			if(len > 0)
			{
				msg_push(l_hMsgFifo, buf, len);
				midLight_action(50, 2, 500, 0);
			}
		}
	}

}





int main(void)
{
	RCC_Configuration();
	GPIO_Configuration();
	
	//led light off
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	GPIO_SetBits(GPIOC, GPIO_Pin_14);

	GPIO_SetBits(GPIOB, GPIO_Pin_11);     //reload拉高
	GPIO_SetBits(GPIOA, GPIO_Pin_1);    //wake拉低
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	tick_ms_init();
	config_init();

	load_config();
	
	l_mq_midLight = rt_mq_create("l_mq_midLight", sizeof(lightAction), 10, RT_IPC_FLAG_PRIO);
	
	rt_thread_t ht_key = rt_thread_create("key_entry", key_entry, RT_NULL, 256, 1, rt_tick_from_millisecond(10));
	if (ht_key!= RT_NULL)
		rt_thread_startup(ht_key);
	
	rt_thread_t ht_wdt = rt_thread_create("wdt_entry", wdt_entry, RT_NULL, 128, 1, rt_tick_from_millisecond(10));
	if (ht_wdt!= RT_NULL)
		rt_thread_startup(ht_wdt);
	
	rt_thread_t ht_main = rt_thread_create("main_entry", main_entry, RT_NULL, 2048, 2, rt_tick_from_millisecond(10));
	if (ht_main!= RT_NULL)
		rt_thread_startup(ht_main);
	
	rt_thread_t ht_status = rt_thread_create("status_entry", status_entry, RT_NULL, 256, 5, rt_tick_from_millisecond(10));
	if (ht_status!= RT_NULL)
		rt_thread_startup(ht_status);
	
	rt_thread_t ht_midLight = rt_thread_create("ht_midLight", midLight_entry, RT_NULL, 256, 5, rt_tick_from_millisecond(10));
	if (ht_midLight!= RT_NULL)
		rt_thread_startup(ht_midLight);
	
	while(1)
	{
		rt_thread_delay(rt_tick_from_millisecond(2*1000));
	}
}

void EXTI15_10_IRQHandler(void)
{
	rt_interrupt_enter();
	
  if(EXTI_GetITStatus(EXTI_Line15) != RESET)
  {
		rt_sem_release(l_sem_key);

    EXTI_ClearITPendingBit(EXTI_Line15);
  }

	rt_interrupt_leave();
}

void EXTI0_IRQHandler(void)
{
	rt_interrupt_enter();
	
  if(EXTI_GetITStatus(EXTI_Line0) != RESET)
  {
		sendTimeing stiming;
		rt_err_t ret;
		stiming.irq_timeStamp = get_timestamp();
		ret = rt_mq_urgent(l_mq_sendTiming, &stiming, sizeof(stiming));
		if(ret != RT_EOK)
		{
			sendTimeing tmp;
			rt_mq_recv(l_mq_sendTiming, &tmp, sizeof(tmp), RT_WAITING_NO);
			rt_mq_urgent(l_mq_sendTiming, &stiming, sizeof(stiming));
		}
		
    EXTI_ClearITPendingBit(EXTI_Line0);
  }

	rt_interrupt_leave();
}




