/*******************************************************************************
 * 文件名称：LEDTest.c
 * 功    能：CC2530基础实验 --- 点亮LED实验
 *           
 * 硬件连接：led灯端口：  LED1（D3）-p1.0
                          LED2（D4）-p1.1
                          LED3（D5）-p1.3
                          LED4（D6）-p1.4
 * 作    者：HJL
 * 日    期：2012-8-13
 ******************************************************************************/
/* 包含头文件 */
/********************************************************************/
#include "ioCC2530.h"  // 引用头文件
/********************************************************************/
//定义led灯端口：p1.0, p1.1, p1.3, p1.4：
#define LED1 P1_0     // P1_0定义为P1.0
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
    P1SEL &= ~(0x01 << 0); // 设置P1.0为普通IO口,0为IO口，1为外设功能
    P1DIR |= 0x01<<0;    // 设置为输出, P1DIR 为P1端口的方向寄存器，
                           // 0：I/O引脚切换成输入模式；1：I/O引脚切换成输出模式                              
    while(1)
    {
      LED1=!LED1;     
      delay(20000);
     
   }    
}
