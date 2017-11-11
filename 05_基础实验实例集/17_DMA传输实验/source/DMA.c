/*******************************************************************************
 * �ļ����ƣ�ADC(Single Conversion)_Ex.c
 * ��    �ܣ�CC253xϵ��Ƭ��ϵͳ����ʵ��--- DMA������(�鴫��ģʽ  ����Դ���0)
 * ��    ������CC253xƬ��DMA��������һ�ַ�����Դ��ַת�Ƶ�Ŀ���ַ�����ÿ鴫��ģ
 *           ʽ�����䳤��Ϊ���ַ����ĳ��ȣ�Դ��ַ��Ŀ���ַ����������Ϊ1����PC��
             ��������ʾ��Ӧ��Ϣ��
 * ��    �ߣ�HJL
 * ��    �ڣ�2012-9-11
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ioCC2530.h"    // CC2530��ͷ�ļ���������CC2530�ļĴ������ж������ȵĶ���
#include "stdio.h"       // C���Ա�׼����/������ͷ�ļ�
#include "String.h"      // C�����ַ������ͷ�ļ�
/********************************************************************/
#define LED1 P1_0     
#define SW1 P1_2      // P1_2����ΪP1.2


/* ����DMA���ò������ݽṹ���� */
/********************************************************************/
#pragma bitfields=reversed    // ���ô�˸�ʽ���ֵ���˼�����ڼ�λ����˼
                              //֪ͨ����������ض��Ĳ���
typedef struct 
{
  unsigned char SRCADDRH;          // Դ��ַ���ֽ�
  unsigned char SRCADDRL;          // Դ��ַ���ֽ�
  unsigned char DESTADDRH;         // Ŀ���ַ���ֽ�
  unsigned char DESTADDRL;         // Ŀ���ַ���ֽ�
  unsigned char VLEN      : 3;     // �ɱ䳤�ȴ���ģʽѡ��
  unsigned char LENH      : 5;     // ���䳤�ȸ��ֽ�
  unsigned char LENL      : 8;     // ���䳤�ȵ��ֽ�
  unsigned char WORDSIZE  : 1;     // �ֽ�/�ִ���
  unsigned char TMODE     : 2;     // ����ģʽѡ��
  unsigned char TRIG      : 5;     // �����¼�ѡ��
  unsigned char SRCINC    : 2;     // Դ��ַ����
  unsigned char DESTINC   : 2;     // Ŀ���ַ����
  unsigned char IRQMASK   : 1;     // �ж�ʹ��
  unsigned char M8        : 1;     // 7/8 bits �ֽ�(ֻ����ֽڴ���ģʽ)
  unsigned char PRIORITY  : 2;     // ���ȼ�
} DMA_CONFIGURATIONPARAMETERS;
#pragma bitfields=default  // �ָ�ΪĬ�ϵ�С�˸�ʽ
/********************************************************************/


/* ����ö������ */
/********************************************************************/
enum SYSCLK_SRC{XOSC_32MHz,RC_16MHz};      // ����ϵͳʱ��Դ(��ʱ��Դ)ö������
/*********************************************************************

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
 * �������ƣ�init
 * ��    �ܣ���ʼ��ϵͳIO
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void initIO(void)
{   P1SEL &= ~0x05;     // ����LED1��SW1Ϊ��ͨIO��
    P1DIR |= 0x001 ;    // ����LED1��P1.0Ϊ���    
    P1DIR &= ~0X04;	//Sw1������ P1.2,�趨Ϊ����    
    LED1= 0;            // LED��
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
     
  /* P0���������ȼ������ϵ縴λĬ��ֵ����P2DIR�Ĵ�������Ĭ��ֵ */
  /* ��һ���ȼ���USART0
     �ڶ����ȼ���USART1
     �������ȼ���Timer1   */
    
  /* UART0���������� */
  /* �����ʣ�57600   
     ��ʹ��32MHz ����������Ϊϵͳʱ��ʱ��Ҫ���57600��������Ҫ�������ã�
         UxBAUD.BAUD_M = 216
         UxGCR.BAUD_E = 10
         ���������Ϊ0.03%
   */
  U0BAUD = 216;
  U0GCR = 10;
  
  /* USARTģʽѡ�� */
  U0CSR |= 0x80;  // UARTģʽ
  
  /* UART0���� ���������ò��������ϵ縴λĬ��ֵ��
         Ӳ�����أ���
         ��żУ��λ(��9λ)����У��
         ��9λ����ʹ�ܣ���
         ��żУ��ʹ�ܣ���
         ֹͣλ��1��
         ֹͣλ��ƽ���ߵ�ƽ
         ��ʼλ��ƽ���͵�ƽ
   */
   U0UCR |= 0x80;  // ����USART���
   
  /* ��춷��͵�λ˳������ϵ縴λĬ��ֵ����U0GCR�Ĵ��������ϵ縴λĬ��ֵ */
  /* LSB�ȷ��� */
  
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
void UART0SendString(unsigned char *str)
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
  SystemClockSourceSelect(XOSC_32MHz);  // ѡ��32MHz����������Ϊϵͳʱ��Դ(��ʱ��Դ)
  initIO();  // IO�˿ڳ�ʼ��
  initUART0();  //��ʼ���˿� 
   /* ����DMAԴ��ַ�ռ䲢��ʼ��Ϊ����DMA������ַ������� */
  unsigned char srcStr[]="�鴫�����ݣ�DMA transfer.";  
  /* ����DMAĿ���ַ�ռ� */
  unsigned char destStr[sizeof(srcStr)];
  
 /* ����DMA�������ݽṹ����� */
  DMA_CONFIGURATIONPARAMETERS dmaCH;
  
  /* ����DMA���ò��� */
  /* Դ��ַ */
  dmaCH.SRCADDRH = (unsigned char)(((unsigned short)(&srcStr)) >> 8);
  dmaCH.SRCADDRL = (unsigned char)(((unsigned short)(&srcStr)) & 0x00FF);
  /* Ŀ���ַ */
  dmaCH.DESTADDRH = (unsigned char)(((unsigned short)(&destStr)) >> 8);
  dmaCH.DESTADDRL = (unsigned char)(((unsigned short)(&destStr)) & 0x00FF);  
  /* �ɱ䳤��ģʽѡ�� */
  dmaCH.VLEN = 0x00;      // ʹ��DMA���䳤��
  /* ���䳤�� */
  dmaCH.LENH = (unsigned char)(((unsigned short)(sizeof(srcStr))) >> 8);
  dmaCH.LENL = (unsigned char)(((unsigned short)(sizeof(srcStr))) & 0x00FF);  
  /* �ֽ�/��ģʽѡ�� */
  dmaCH.WORDSIZE = 0x00;  // �ֽڴ���
  /* ����ģʽѡ�� */
  dmaCH.TMODE = 0x01;     // �鴫��ģʽ  
  /* ����Դѡ�� */
  dmaCH.TRIG = 0x00;      // ͨ��дDMAREQ������
  /* Դ��ַ���� */
  dmaCH.SRCINC = 0x01;    // ÿ�δ�����ɺ�Դ��ַ��1�ֽ�/�� 
  /* Ŀ���ַ���� */
  dmaCH.DESTINC = 0x01;   // ÿ�δ�����ɺ�Ŀ���ַ��1�ֽ�/��  
  /* �ж�ʹ�� */
  dmaCH.IRQMASK = 0x00;   // ��ֹDMA�����ж�
  /* M8 */
  dmaCH.M8 = 0x00;        // 1���ֽ�Ϊ8����  
  /* ���ȼ����� */
  dmaCH.PRIORITY = 0x02;  // �߼�

  /* ʹ��DMAͨ��0 */
  DMA0CFGH = (unsigned char)(((unsigned short)(&dmaCH)) >> 8);
  DMA0CFGL = (unsigned char)(((unsigned short)(&dmaCH)) & 0x00FF); 
   while(1)
  {
    /***************��srcStr��ֵ**************/
    sprintf(srcStr,"�鴫�����ݣ�DMA transfer.");  
    
    /* ��PC������������ʾDMA�����Դ��ַ��Ŀ���ַ */
    unsigned char s[31];
    sprintf(s,"����Դ��ַ: 0x%04X�� ",(unsigned short)(&srcStr));  //��ʽ���ַ���   
    UART0SendString(s);   
   
    sprintf(s,"Ŀ���ַ: 0x%04X��",(unsigned short)(&destStr));
    UART0SendString(s);
    memset(destStr, 0, sizeof(destStr));  // ���DMAĿ���ַ�ռ������
   
    UART0SendString("��SW1��ʼDMA����...\r\n\r\n");  // ��UART0�����ַ���
       
    /* ʹDMAͨ��0���빤��״̬ */
    DMAARM = 0x01;  
 
   /* ���DMA�жϱ�־ */
    DMAIRQ = 0x00; 
    
    LED1=0;
    
    /* �ȴ��û����������(����λ����)*/
    while(SW1 == 1);
    
    if(SW1 == 0)	//�͵�ƽ��Ч
        {     while(SW1 == 0); //�ȴ��û��ɿ�����              
               /* ����DMA���� */
               DMAREQ = 0x01;
  
               /* �ȴ�DMA������� */
               while((DMAIRQ & 0x01) == 0);
  
              /* ��֤DMA�������ݵ���ȷ�� */
              unsigned char i,errors = 0;
              for(i=0;i<sizeof(srcStr);i++)
                {
                    if(srcStr[i] != destStr[i]) errors++;     
                }
  
              /* ��PC������������ʾDMA������ */
              if(errors)
               {
                  LED1 = 0;
                  UART0SendString("�������\r\n\r\n\r\n");  // ��UART0�����ַ���
               }
              else
                {
                  LED1 =1;
                  sprintf(s,"%s  ����ɹ�!\r\n\r\n\r\n",destStr);  //��ʽ���ַ���
                  UART0SendString(s);  // ��UART0�����ַ���
                }           
           } //if
      } //while
}