/*******************************************************************************
 * �ļ����ƣ�Flash_Ex.c
 * ��    �ܣ�CC253xϵ��Ƭ��ϵͳ����ʵ��--- Flash��дʵ��
 * ��    ������CC253xƬ��FLASH BANK7��ǰ8���ֽ�д��8�ֽ����ݡ�д��֮ǰ���Ƚ�����
 *           Ӧ��FLASHҳ(112ҳ)������Ȼ��ͨ��DMA FLASHд�����������ݵ�д�롣
  *          CC253xϵ��Ƭ��ϵͳоƬ����ΪCC2530F256��CC2531
 * ע    �⣺���CC253xϵ��Ƭ��ϵͳ��Ƭ��FLASH�Ĳ���/д����������޵ģ���Լ20000
 *           �Σ���˲������ν��б�ʵ�顣
 * ��    �ߣ�HJL
 * ��    �ڣ�2012-9-11
 ******************************************************************************/

/* ����ͷ�ļ� */
/********************************************************************/
#include "ioCC2530.h"    // CC2530��ͷ�ļ���������CC2530�ļĴ������ж������ȵĶ���
#include "stdio.h"       // C���Ա�׼����/������ͷ�ļ�
#include "String.h"      // C�����ַ������ͷ�ļ�
/********************************************************************/
//����˿ڣ�p1.2
#define SW1  P1_2     // P1_2����ΪSW1

/* ����ö������ */
/********************************************************************/
enum SYSCLK_SRC{XOSC_32MHz,RC_16MHz};      // ����ϵͳʱ��Դ(��ʱ��Դ)ö������
/********************************************************************/


/* ����DMA���ò������ݽṹ���� */
/********************************************************************/
#pragma bitfields=reversed  // ���ô�˸�ʽ
typedef struct 
{
  unsigned char srcAddrH;
  unsigned char srcAddrL;
  unsigned char dstAddrH;
  unsigned char dstAddrL;
  unsigned char xferLenV;
  unsigned char xferLenL;
  unsigned char ctrlA;
  unsigned char ctrlB;
} flashdrvDmaDesc_t;
#pragma bitfields=default  // �ָ�ΪĬ�ϵ�С�˸�ʽ
/********************************************************************/
/* ����DMA�������ݽṹ����� */
flashdrvDmaDesc_t flashdrvDmaDesc;

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
 * �������ƣ�initIO
 * ��    �ܣ���ʼ��ϵͳIO
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void initIO()
{    P1SEL &= ~0x04;     // ����LED1��SW1Ϊ��ͨIO��
     P1DIR &= ~0X04;	//Sw1������ P1.2,�趨Ϊ����   
}


/********************************************************************************
 * ��������:    FLASHDRV_Write
 * ��    �ܣ�   дflash 
 * ��ڲ�����   unsigned short addr,unsigned char *buf,unsigned short cnt  
 *  		     addr - flash�ֵ�ַ: �ֽڵ�ַ/4����addr=0x1000���ֽڵ�ַΪ0x4000
 *  		     buf - ���ݻ�����.
 *   		     cnt - ������ ʵ��д��cnt*4�ֽ�
 * ���ڲ�����		��
 * �� �� ֵ��		��
/****************************************************/
void FLASHDRV_Write(unsigned short addr,
                    unsigned char *buf,
                    unsigned short cnt)
{
  flashdrvDmaDesc.srcAddrH = (unsigned char) ((unsigned short)buf >> 8);
  flashdrvDmaDesc.srcAddrL = (unsigned char) (unsigned short) buf;
  flashdrvDmaDesc.dstAddrH = (unsigned char) ((unsigned short)&FWDATA >> 8);
  flashdrvDmaDesc.dstAddrL = (unsigned char) (unsigned short) &FWDATA;
  flashdrvDmaDesc.xferLenV =
    (0x00 << 5) |               // use length
    (unsigned char)(unsigned short)(cnt >> 6);  // length (12:8). Note that cnt is flash word
  flashdrvDmaDesc.xferLenL = (unsigned char)(unsigned short)(cnt * 4);
  flashdrvDmaDesc.ctrlA =
    (0x00 << 7) | // word size is byte
    (0x00 << 5) | // single byte/word trigger mode
    18;           // trigger source is flash
  flashdrvDmaDesc.ctrlB =
    (0x01 << 6) | // 1 byte/word increment on source address
    (0x00 << 4) | // zero byte/word increment on destination address
    (0x00 << 3) | // The DMA is to be polled and shall not issue an IRQ upon completion.
    (0x00 << 2) | // use all 8 bits for transfer count
    0x02; // DMA priority high

  DMAIRQ &= ~( 1 << 0 ); // clear IRQ
  DMAARM = (0x01 << 0 ); // arm DMA

  FADDRL = (unsigned char)addr;
  FADDRH = (unsigned char)(addr >> 8);
  FCTL |= 0x02;         // Trigger the DMA writes.
  while (FCTL & 0x80);  // Wait until writing is done.
}
void FLASHDRV_Init(void)
{
  DMA0CFGH = (unsigned char) ((unsigned short) &flashdrvDmaDesc >> 8);
  DMA0CFGL = (unsigned char) (unsigned short) &flashdrvDmaDesc;
  DMAIE = 1;
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
  /* ����Ҫд��FLASH��8�ֽ����� */
  /* ����DMA FLASHд��������˸�������׵�ַ��ΪDMAͨ����Դ��ַ */
  unsigned char FlashData[8] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
  unsigned char i;
  char s[31];
  char xdat[8];
   SystemClockSourceSelect(XOSC_32MHz);  // ѡ��32MHz����������Ϊϵͳʱ��Դ(��ʱ��Դ)
 
  initIO();  // IO�˿ڳ�ʼ��
  initUART0();  //��ʼ���˿� 
  FLASHDRV_Init();
  while(1)
  {    
   /* ��FLASH BANK7 ӳ�䵽XDATA�洢���ռ��XBANK���򣬼�XDATA�洢���ռ�0x8000-0xFFFF */
    MEMCTR |= 0x07;
   
   /* �ڴ�����������ʾ�����Ϣ */
    UART0SendString("XDATA�洢��ǰ8����ַ�е����ݣ�\r\n");
 
  /* ��ʾXDATA�洢���ռ�0x8000 - 0x8007��8����ַ�е����� */
  /* �����֮ǰ�Ѿ���FLASH BANK7ӳ�䵽��XDATA�洢���ռ��XBANK����(0x8000-0xFFFF), 
     ���ʵ������ʾ����FLASH BANK7��ǰ8���ֽڴ洢�����ݡ� */
    for(i=0;i<8;i++)
    {
      xdat[i] = *(unsigned char volatile __xdata *)(0x8000 + i);
    }
  
    sprintf(s,"ԭ�ȣ�%02X%02X%02X%02X%02X%02X%02X%02X \r\n\r\n",
              xdat[0],xdat[1],xdat[2],xdat[3],xdat[4],xdat[5],xdat[6],xdat[7]);
    UART0SendString(s);
    UART0SendString("��SW1������...... \r\n");
    /* �ȴ��û�����SW1��*/
    while(SW1 == 1);
    delay(100);
    while(SW1 == 0);  //�ȴ��û��ɿ�����
  /* ����һ��FLASHҳ */
  /* �˴�����FLASH BANK7�еĵ�һ��ҳ����112ҳ, P9=112<<1 */
    EA = 0;                // ��ֹ�ж�
    while (FCTL & 0x80);   // ��ѯFCTL.BUSY���ȴ�FLASH����������
    FADDRH = 112 << 1;     // ����FADDRH[7:1]��ѡ������Ҫ������ҳ
    FCTL |= 0x01;          // ����FCTL.ERASEΪ1������һ��ҳ��������
    asm("NOP");
    while (FCTL & 0x80);   // �ȴ�ҳ�����������(��Լ20ms)
    EA = 1;                // ʹ���ж�
  /* ��ʾXDATA�洢���ռ�0x8000 - 0x8007��8����ַ�е����� */
    for(i=0;i<8;i++)
     {
        xdat[i] = *(unsigned char volatile __xdata *)(0x8000 + i);
     }
    sprintf(s,"������%02X%02X%02X%02X%02X%02X%02X%02X \r\n\r\n",
              xdat[0],xdat[1],xdat[2],xdat[3],xdat[4],xdat[5],xdat[6],xdat[7]);
    UART0SendString(s);
  
    UART0SendString("��SW1����ʼд��'1122334455667788'...... \r\n");
   
     /* �ȴ��û�����SW1��*/
    while(SW1 == 1);
    delay(100);
    while(SW1 == 0);  //�ȴ��û��ɿ����� 
    //д���� ��ַ112*512(ʵ�ʵ�ַΪ112*2048��������FlashData���ֽ�2*4
    FLASHDRV_Write( (unsigned int)112*512, FlashData, 2);
 
   /* ��ʾXDATA�洢���ռ�0x8000 - 0x8007��8����ַ�е����� */
   for(i=0;i<8;i++)
   {
      xdat[i] = *(unsigned char volatile __xdata *)(0x8000 + i);
   }
  
    sprintf(s,"д���%02X%02X%02X%02X%02X%02X%02X%02X \r\n\r\n",
            xdat[0],xdat[1],xdat[2],xdat[3],xdat[4],xdat[5],xdat[6],xdat[7]);
    UART0SendString(s);
     /* �ȴ��û�����SW1��*/
    UART0SendString("��SW1�����¿�ʼ...\r\n");
    while(SW1 == 1);
    delay(100);
    while(SW1 == 0);  //�ȴ��û��ɿ����� 
  }
}