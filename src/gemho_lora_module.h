#ifndef __GEMHO_CMD_H
#define __GEMHO_CMD_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>   

   
#pragma pack(1)

typedef struct tag_loraModu_config
{
  uint32_t aid;   
  uint32_t spd;
  uint32_t ch;
  uint32_t baudrate;
  uint8_t stopbit;
  uint8_t parity;
	uint8_t watchdog;
}loraModu_config;

typedef struct tag_confSaveUnit
{
  loraModu_config config;
  uint32_t crc32;
}confSaveUnit;

#pragma pack()

typedef struct tag_lightAction
{
  uint32_t interval_time;
  uint32_t blink_times;
	uint32_t stop_time;
}lightAction;

typedef enum tag_ModeToRun{
  atDebug = 0,
  gemhoConfig,
  lucTrans,
}ModeToRun; 

typedef enum tag_DeviceStatus{
	DEVICEOK = 0,
  L101CDONTWORK = -1,
}DeviceStatus;


typedef int (*ghCmd_excute)(char *, int);

typedef struct tag_cmdExcute
{
  char *cmd;
  ghCmd_excute ce_fun;
}cmdExcute;


//#define USERCOM USART3
//#define BC95COM USART1
#define UCOMBAUDRATE 115200
#define L101CBAUDRATE 115200

#define ATVER "AT+VER" //获取版本号

#define MSG_MAXLEN (200)
#define ATDEBUG "MODE+ATDEBUG\r\n"
#define GEMHOCFG "MODE+GEMHOCFG\r\n"
#define OKSTR "OK\r\n"
#define OKSTRNOENTER "OK"
#define ERRORSTR "ERROR\r\n"
#define ENDFLAG "\r\n"
#define ATSTR "AT\r\n"
#define ENDOK "\r\n\r\nOK"
#define ATECHO "AT+E=ON\r\n"
#define ATENTM "AT+ENTM\r\n"
#define NIDGET "AT+NID\r\n"
#define NIDRTN "+NID:"

#define PPPA "+++a"

#define ATRS232 "AT+RS232"
#define ATRS232EQ "AT+RS232="

#define ATSPEED "AT+SPEED" //设置/查询速率等级
#define ATSPEEDEQ "AT+SPEED=" 

#define ATCHN "AT+CHN"    //设置/查询信道
#define ATCHNEQ "AT+CHN=" 

#define ATAPPID "AT+APPID"    //设置/查询应用 ID
#define ATAPPIDEQ "AT+APPID=" 

#define ATNID "AT+NID"    //查询应用 ID

#define ATWDT "AT+WDT"    //是否开启看门狗  
#define ATWDTEQ "AT+WDT=" 

#define ATSAVE "AT+SAVE"   //存储配置
#define ATDELO "AT+DELO"  //还原默认配置
#define UDCMD "UNDEFINED CMD\r\n"

#define KPMFT "key push message for test"

#define VERSION "1.0.0.0"
   
#ifdef __cplusplus
 }
#endif

#endif /* __GEMHO_CMD_H */



