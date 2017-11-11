/*******************************************************************************
 * �ļ����ƣ�ChipInformation.c
 * ��    �ܣ�CC253xϵ��Ƭ��ϵͳ����ʵ��--- ��ȡоƬ����Ϣ
 * ��    ������ȡCC253xϵ��Ƭ��ϵͳ����Ϣ����PC������������ʾ��
 * ��    �ߣ�HJL
 * ��    �ڣ�2010-04-18
 ******************************************************************************/

/* ����ͷ�ļ� */
/********************************************************************/
#include "ioCC2530.h"    // CC2530��ͷ�ļ���������CC2530�ļĴ������ж������ȵĶ���
#include "stdio.h"       // C���Ա�׼����/������ͷ�ļ�
/********************************************************************/

/* ����ö������ */
/********************************************************************/
enum SYSCLK_SRC{XOSC_32MHz,RC_16MHz};      // ����ϵͳʱ��Դ(��ʱ��Դ)ö������
/********************************************************************/

/*********************************************************************
* �������ƣ�delay
 * ��    �ܣ������ʱ
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void delay(unsigned int time)
{ unsigned int i;
  unsigned char j;
  for(i = 0; i < time; i++)
  {  for(j = 0; j < 240; j++)
      {   asm("NOP");    // asm����Ƕ��࣬nop�ǿղ���,ִ��һ��ָ������
          asm("NOP");
          asm("NOP");
       }  
   }  
}

/*********************************************************************
 * �������ƣ�SystemClockSourceSelect
 * ��    �ܣ�ѡ��ϵͳʱ��Դ(��ʱ��Դ)
 * ��ڲ�����source
 *             XOSC_32MHz  32MHz��������
 *             RC_16MHz    16MHz RC����
 * ���ڲ�������
 * �� �� ֵ����
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
  
  /* �ȴ���ѡ���ϵͳʱ��Դ(��ʱ��Դ)�ȶ� */
  clkconcmd = CLKCONCMD;             // ��ȡʱ�ӿ��ƼĴ���CLKCONCMD
  do
  {
    clkconsta = CLKCONSTA;           // ��ȡʱ��״̬�Ĵ���CLKCONSTA
  } while(clkconsta != clkconcmd);  // ֱ��ѡ���ϵͳʱ��Դ(��ʱ��Դ)�Ѿ��ȶ� 
}



/*********************************************************************
 * �������ƣ�InitUART0
 * ��    �ܣ�UART0��ʼ��
 *           P0.2  RX                  
 *           P0.3  TX
 *           �����ʣ�57600
 *           ����λ��8
 *           ֹͣλ��1
 *           ��żУ�飺��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void initUART0(void)
{
  /* Ƭ����������λ�ò����ϵ縴λĬ��ֵ����PERCFG�Ĵ�������Ĭ��ֵ */
    PERCFG = 0x00;	//λ�� 1 P0 ��     
    /* UART0������ų�ʼ�� 
       P0.2����RX��      P0.3����TX      P0.4����CT��      P0.5����RT   */     
    P0SEL = 0x3c;	//P0 ��������, P0.2��P0.3��P0.4��P0.5��ΪƬ������I/O  
    
    /*������57600,8λ����λ��1λֹͣλ,����żУ��****/
    U0BAUD = 216; 
    U0GCR = 10;  //32M����
  
  /* USARTģʽѡ�� */
    U0CSR |= 0x80;  // UARTģʽ
    U0UCR |= 0x80;  // ����USART���
   
   UTX0IF = 0;  // ����UART0 TX�жϱ�־
   EA = 1;   //ʹ��ȫ���ж�
}


/*********************************************************************
 * �������ƣ�UART0SendByte
 * ��    �ܣ�UART0����һ���ֽ�
 * ��ڲ�����c
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void UART0SendByte(unsigned char c)
{
  U0DBUF = c;       // ��Ҫ���͵�1�ֽ�����д��U0DBUF(���� 0 �շ�������)  
  while (!UTX0IF);  // �ȴ�TX�жϱ�־����U0DBUF����
  UTX0IF = 0;       // ����TX�жϱ�־ 
}


/*********************************************************************
 * �������ƣ�UART0SendString
 * ��    �ܣ�UART0����һ���ַ���
 * ��ڲ�����str
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void UART0SendString( unsigned char *str)
{
  while(1)
  {
    if(*str == '\0') break;  // �������������˳�
    UART0SendByte(*str++);   // ����һ�ֽ�
  } 
}


/*********************************************************************
 * �������ƣ�main
 * ��    �ܣ�main�������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void main(void)
{
  char s[31];
  unsigned char i;
  char xdat[8];
  
  SystemClockSourceSelect(XOSC_32MHz);  // ѡ��32MHz����������Ϊϵͳʱ��Դ(��ʱ��Դ)
  initUART0(); //��ʼ���˿�
    
  /* �ڴ�����������ʾ�����Ϣ */
  UART0SendString("CC253x ��ȡоƬ����Ϣ...\r\n\r\n"); // ��ȡоƬ����Ϣ
  delay(30000);
  
  /* ��ȡ����ʾоƬ�ͺ� */ 
  if(CHIPID == 0xA5)
  { 
    UART0SendString("����:CC2530...\r\n\r\n");
  }
  else if(CHIPID == 0xB5)
  {
    UART0SendString("����:CC2531...\r\n\r\n");
  }
  else
  {
    UART0SendString("����:δ֪...\r\n\r\n");
  }
  delay(30000);
  
  /* ��ȡ����ʾоƬID */
  sprintf(s,"оƬID:0x%02X...\r\n\r\n",CHIPID);
  UART0SendString((unsigned char *)s);
  delay(30000);
  
  /* ��ȡ����ʾоƬ�汾 */
  sprintf(s,"оƬ�汾��Version:0x%02X...\r\n\r\n",CHVER);
  UART0SendString((unsigned char *)s);
  delay(30000);
  
  /* ��ȡ����ʾƬ��FLASH���� */
  if(((CHIPINFO0 & 0x70) >> 4) == 1)
  {
    UART0SendString("FLASH����:32 KB...\r\n\r\n");
  }
  else if(((CHIPINFO0 & 0x70) >> 4) == 2)
  {
    UART0SendString("FLASH����:64 KB...\r\n\r\n");
  }
  else if(((CHIPINFO0 & 0x70) >> 4) == 3)
  {
    UART0SendString("FLASH����:128 KB...\r\n\r\n");
  }
  else if(((CHIPINFO0 & 0x70) >> 4) == 4)
  { 
    UART0SendString("FLASH����:256 KB...\r\n\r\n");
  }
  else
  {
    UART0SendString("FLASH����:δ֪...\r\n\r\n");
  }
  delay(30000);
  
  /* ��ȡ����ʾƬ��SRAM���� */
  sprintf(s,"SRAM����:%2d KB...\r\n\r\n",(CHIPINFO1 & 0x07)+1);
  UART0SendString( (unsigned char *)s ); 
  delay(60000);
  
  /* ������ʾ�������� */
   UART0SendString("CC253x ��ȡоƬ��������Ϣ...\r\n\r\n");//��ȡоƬ����Ϣ
  delay(30000);
  
  /* ��ȡ����ʾƬ���Ƿ���USB������ */
  if(CHIPINFO0 & 0x08)
  {
    UART0SendString("��USB������...\r\n\r\n");
  }
  else
  {
    UART0SendString("��USB������...\r\n\r\n");
  }
 delay(30000);

  
  /* ��ȡ����ʾоƬ����ʱ���е�����TI�����е�IEEE��ַ��Χ��IEEE��ַ */
    
  for(i=0;i<8;i++)
  {
    xdat[i] = *(unsigned char volatile __xdata *)(0x780C + i);
  }
  
  sprintf(s,"IEEE��ַ��%02X%02X%02X%02X%02X%02X%02X%02X",
            xdat[0],xdat[1],xdat[2],xdat[3], xdat[4],xdat[5],xdat[6],xdat[7]);
  
  UART0SendString((unsigned char *)s); 
  delay(30000);
  while(1);
}