/*******************************************************************************
  * 文件名称：uart1.c
 * 功    能：CC253x系列片上系统基础实验 ---单片机串口发数
 * 描    述：实现从 CC2530 上通过串口每秒向PC机发送字串"UART0发送数据""，
            在PC端实验串口助手来接收数据。实验使用 CC2530 的UART0，波特率57600。
 * 硬件连接： LED与CC253x的硬件连接关系如下：
 *                LED                          CC253x  
 *               LED1（D3）                       P1.0
 *               LED2（D4）                       P1.1
 *               LED3（D5）                       P1.3
 *               LED4（D6）                       P1.4
                  SW1                             P1.2
 * 作    者：HJL
 * 日    期：2012-8-14
 ******************************************************************************/
/* 包含头文件 */
/********************************************************************/
#include "ioCC2530.h"  // 引用头文件,包含对CC2530的寄存器、中断向量等的定义
/********************************************************************/
//定义led灯端口
#define LED1 P1_0     // P1_0定义为P1.0
unsigned int counter=0; //统计溢出次数
/********************************************************************/
/* 定义枚举类型 */
/********************************************************************/
enum SYSCLK_SRC{XOSC_32MHz,RC_16MHz};  // 定义系统时钟源(主时钟源)枚举类型
/*********************************************************************
 * 函数名称：SystemClockSourceSelect
 * 功    能：选择系统时钟源(主时钟源)
 * 入口参数：source
 *             XOSC_32MHz  32MHz晶体振荡器
 *             RC_16MHz    16MHz RC振荡器
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void SystemClockSourceSelect(enum SYSCLK_SRC source)
{
   unsigned char clkconcmd,clkconsta;
   if(source == RC_16MHz)
  {             
    CLKCONCMD &= 0x80;
    CLKCONCMD |= 0x49;    //01001001   
  }
  else if(source == XOSC_32MHz)
  {
    CLKCONCMD &= 0x80;
  }  
  
  /* 等待所选择的系统时钟源(主时钟源)稳定 */
  clkconcmd = CLKCONCMD;             // 读取时钟控制寄存器CLKCONCMD
  do
  {
    clkconsta = CLKCONSTA;           // 读取时钟状态寄存器CLKCONSTA
  } while(clkconsta != clkconcmd);  // 直到选择的系统时钟源(主时钟源)已经稳定 
}

/*********************************************************************
 * 函数名称：InitUART0
 * 功    能：UART0初始化
 *           P0.2  RX                  
 *           P0.3  TX
 *           波特率：57600
 *           数据位：8
 *           停止位：1
 *           奇偶校验：无
 * 入口参数：无
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void initUART0(void)
{
  /* 片内外设引脚位置采用上电复位默认值，即PERCFG寄存器采用默认值 */
    PERCFG = 0x00;	//位置 1 P0 口     
    /* UART0相关引脚初始化 
       P0.2——RX，      P0.3——TX      P0.4——CT，      P0.5——RT   */     
    P0SEL = 0x3c;	//P0 用作串口, P0.2、P0.3、P0.4、P0.5作为片内外设I/O  
     
  /* P0口外设优先级采用上电复位默认值，即P2DIR寄存器采用默认值 */
  /* 第一优先级：USART0
     第二优先级：USART1
     第三优先级：Timer1   */
    
  /* UART0波特率设置 */
  /* 波特率：57600   
     当使用16MHz 晶体振荡器作为系统时钟时，要获得57600波特率需要如下设置：
         UxBAUD.BAUD_M = 216
         UxGCR.BAUD_E = 11
         该设置误差为0.03%
   */
  U0BAUD = 216;
  U0GCR = 11;
  
  /* USART模式选择 */
  U0CSR |= 0x80;  // UART模式
  
  /* UART0配置 ，以下配置参数采用上电复位默认值：
         硬件流控：无
         奇偶校验位(第9位)：奇校验
         第9位数据使能：否
         奇偶校验使能：否
         停止位：1个
         停止位电平：高电平
         起始位电平：低电平
   */
   U0UCR |= 0x80;  // 进行USART清除
   
  /* 用於发送的位顺序采用上电复位默认值，即U0GCR寄存器采用上电复位默认值 */
  /* LSB先发送 */
  
   UTX0IF = 0;  // 清零UART0 TX中断标志
   EA = 1;   //使能全局中断
}

/*********************************************************************
 * 函数名称：inittTimer1
 * 功    能：初始化定时器T1控制状态寄存器
 * 入口参数：无
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void inittTimer1()
{  
     /* 配置定时器1的16位计数器的计数频率,定时1S，采用正计数/倒计数模式
       Timer Tick    分频    定时器1的计数频率   T1CC0的值     时长    
       16MHz         /128       125KHz            62500       0.5s  */  
       T1CC0L =62500 & 0xFF;               // 把62500的低8位写入T1CC0L
       T1CC0H = ((62500 & 0xFF00) >> 8);   // 把62500的高8位写入T1CC0H
       T1CTL = 0x0F;      // 配置128分频，正计数/倒计数模式，并开始启动 
       
       T1IF=0;           //清除timer1中断标志(同IRCON &= ~0x02)
       IEN1 |= 0x02;    //使能定时器1的中断  
       EA = 1;        //使能全局中断        
}


/*********************************************************************
 * 函数名称：UART0SendByte
 * 功    能：UART0发送一个字节
 * 入口参数：c
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void UART0SendByte(unsigned char c)
{
  U0DBUF = c;       // 将要发送的1字节数据写入U0DBUF(串口 0 收发缓冲器)  
  while (!UTX0IF);  // 等待TX中断标志，即U0DBUF就绪
  UTX0IF = 0;       // 清零TX中断标志 
}


/*********************************************************************
 * 函数名称：UART0SendString
 * 功    能：UART0发送一个字符串
 * 入口参数：无
 * 出口参数：无
 * 返 回 值：无
 ********************************************************************/
void UART0SendString(unsigned char *str)
{
  while(1)
  {
    if(*str == '\0') break;  // 遇到结束符，退出
    UART0SendByte(*str++);   // 发送一字节
  } 
}

/*********************************************************************
 * 功    能：定时器T1中断服务子程序
 ********************************************************************/
#pragma vector = T1_VECTOR //中断服务子程序
__interrupt void T1_ISR(void) 
  { 
    EA = 0;   //禁止全局中断
    LED1 = !LED1;
    UART0SendString("UART0发送数据\n");  // 从UART0发送字符串
    T1IF=0;   //清T1的中断请求
    EA = 1;   //使能全局中断    
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
   P1DIR |= 0x01;   /* 配置P1.0的方向为输出 */
   SystemClockSourceSelect(RC_16MHz);
   inittTimer1();  //初始化Timer1
   initUART0();  // UART0初始化  
   while(1) ; 
}