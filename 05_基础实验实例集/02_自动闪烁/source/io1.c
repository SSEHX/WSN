/*******************************************************************************
  * 文件名称：io1.c
 * 功    能：CC253x系列片上系统基础实验 ---自动闪烁
 * 描    述：使用CC253x系列片上系统的数字I/O作为通用I/O来控制LED闪烁。当通用I/O
 *           输出高电平时，所控制的LED被点亮；当通用I/O输出低电平时，所控制的LED
 *           熄灭。
 * 硬件连接： LED与CC253x的硬件连接关系如下：
 *                LED                          CC253x       
 *             LED1（D3）                       P1.0
 *             LED2（D4）                       P1.1
 *             LED3（D5）                       P1.3
 *             LED4（D6）                       P1.4

 * 作    者：HJL
 * 日    期：2012-8-13
 ******************************************************************************/
/* 包含头文件 */
/********************************************************************/
#include "ioCC2530.h"  // 引用头文件,包含对CC2530的寄存器、中断向量等的定义
/********************************************************************/
//定义led灯端口：p1.0, p1.1：
#define LED1 P1_0     // P1_0定义为P1.0
#define LED2 P1_1     // P1_1定义为P1.1
/*********************************************************************
* 函数名称：delay
 * 功    能：软件延时
 * 入口参数：无
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void delay(unsigned int time)
{ unsigned int i;
  unsigned char j;
  for(i = 0; i < time; i++)
  {  for(j = 0; j < 240; j++)
      {   asm("NOP");    // asm是内嵌汇编，nop是空操作,执行一个指令周期
          asm("NOP");
          asm("NOP");
       }  
   }  
}

/*********************************************************************
 * 函数名称：main
 * 功    能：main函数入口
 * 入口参数：无
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void main(void)
{
    P1SEL &= ~(0x03); // 设置LED1、LED2为普通IO口
    P1DIR |= 0x03 ;    // 设置LED1、LED2为输出                              
    while(1)
    {
      LED1 = 1;     //高电平点亮
      LED2 = 0;     //低电平熄灭
      delay(30000);
      LED1 = 0;    
      LED2 = 1;     
      delay(30000);
   }    
}
