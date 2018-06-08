#ifndef __TIME_UTILS_H
#define __TIME_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdint.h>


//获取时间戳
uint64_t get_timestamp(void);

//systick初始化
void tick_ms_init(void);

//systick中断
void SysTick_Handler(void);

//延时函数（单位ms）
void delay_ms(uint64_t time_ms);

//延时函数（单位ms）
void loop_ms(uint64_t time);


/**
 * 初始化独立看门狗
 * prer:分频数:0~7(只有低 3 位有效!)
 * 分频因子=4*2^prer.但最大值只能是 256!
 * rlr:重装载寄存器值:低 11 位有效.
 * 时间计算(大概):Tout=((4*2^prer)*rlr)/40 (ms).
 */
void IWDG_Init(uint8_t prer,uint16_t rlr);

/**
 * 喂独立看门狗
 */
void IWDG_Feed(void);



#ifdef __cplusplus
}
#endif

#endif /* __TIME_UTILS_H */


