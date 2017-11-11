/*******************************************************************************
 * �ļ����ƣ�hal_rfid.c
 * ��    �ܣ�RFIDоƬTRF7960����
 * Ӳ�����ӣ�TRF7960��CC253x��Ӳ�����ӹ�ϵ���£�
 *
 *          TRF7960                 CC253x
 *           IRQ                     P0.1
 *           CLK                     P1.0
 *           MOSI                    P0.7
 *           MISO                    P1.4
 *           SS                      P0.0
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_rfid.h"
#include "osal.h"
#include "hal_board.h"
/********************************************************************/


#ifdef RFID_SENSOR

/* �궨�� */
/*********************************************************************/
#define BUF_LENGTH                      100 // ���֡�ֽ�����


/* ���ر��� */
/*********************************************************************/
unsigned char rfid_buf[BUF_LENGTH];  // ����MSP430΢��������TRF7961ͨ�Ž��ջ�����
signed   char RXTXstate;
unsigned char RXErrorFlag; // ������մ����־
unsigned char rfid_flags;  // ����洢��־λ(�ڷ³�ײ��ʹ��)
unsigned char i_reg;       // �жϼĴ�������
unsigned char CollPoss;    // �����ײ����λ�ñ���
unsigned int mask = 0x80;
unsigned char UIDString[9];
bool uidfound;
/*********************************************************************/


/* ���ر��� */
/*********************************************************************/
void halMcuWaitUs(uint16 usec);
void SPIStartCondition(void);
void SPIStopCondition(void);
void WriteCont(unsigned char *pbuf, unsigned char lenght);
void DirectCommand(unsigned char *pbuf);
void RAWwrite(unsigned char *pbuf, unsigned char lenght);
void InitialSettings(void);
void ReInitialize15693Settings(void);
void InterruptHandlerReader(unsigned char *Register);
HAL_ISR_FUNCTION( halRFIDPort0Isr, P0INT_VECTOR );
void TRF7960_IO_Init_SPI(void);
void EnableSlotCounter(void);
void DisableSlotCounter(void);
void Check_ireg(void);
void InventoryRequest(unsigned char *mask, unsigned char lenght);
unsigned char RequestCommand(unsigned char *pbuf, unsigned char lenght, unsigned char brokenBits, char noCRC);
/*********************************************************************/


/*******************************************************************************
 * �������ƣ� halMcuWaitUs
 * ��    �ܣ�æ�ȴ����ܡ��ȴ�ָ����΢��������ͬ��ָ�������ʱ��������
 *           ����ͬ��һ�����ڵĳ���ʱ��ȡ����MCLK��
 * ��ڲ�����usec  ��ʱʱ������λWie΢��
 * ���ڲ�������
 * �� �� ֵ����
 * ע    �⣺�˹��ܸ߶�������MCU�ܹ��ͱ�������
 ******************************************************************************/
#pragma optimize=none
void halMcuWaitUs(uint16 usec)
{
  usec>>= 1;
  while(usec--)
  {
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
  }
}


/*******************************************************************************
 * �������ƣ� halMcuWaitMs
 * ��    �ܣ�æ�ȴ����ܡ��ȴ�ָ���ĺ�������
 * ��ڲ�����msec  ��ʱʱ������λΪ����
 * ���ڲ�������
 * �� �� ֵ����
 * ע    �⣺�˹��ܸ߶�������MCU�ܹ��ͱ�������
 ******************************************************************************/
#pragma optimize=none
void halMcuWaitMs(uint16 msec)
{
  while(msec--)
    halMcuWaitUs(1000);
}


/*******************************************************************************
* �������ƣ�SPIStartCondition()
* ��    �ܣ�����(SPI)ģʽ��ʼͨ������
* ��ڲ�������
* ���ڲ�������
* ˵    ����MSP430΢��������TRF7961ʹ�ô���(SPI)ģʽͨ�ţ���ʼ��������
*******************************************************************************/
void SPIStartCondition(void)
{
  CLKGPIOset();  // ����CLKΪ��ͨGPIO
  CLKPOUTset();  // ����CLK����Ϊ���
  CLKON();       // CLKʱ�ӿ������ߣ�
}


/*******************************************************************************
* �������ƣ�SPIStopCondition()
* ��    �ܣ�����(SPI)ģʽֹͣͨ������
* ��ڲ�������
* ���ڲ�������
* ˵    ����MSP430΢��������TRF7961ʹ�ô���(SPI)ģʽͨ�ţ�ֹͣ����
*******************************************************************************/
void SPIStopCondition(void)
{
  CLKGPIOset(); // ����CLKΪ��ͨGPIO
  CLKOFF();     // CLKʱ�ӹرգ��ͣ�
  CLKPOUTset(); // ����CLK����Ϊ���
  CLKOFF();     // CLKʱ�ӹرգ��ͣ�
}


/*******************************************************************************
* �������ƣ�WriteSingle()
* ��    �ܣ�д�����Ĵ������������ַ�Ķ���Ĵ�������
* ��ڲ�����*pbuf            ��Ҫд�������           
*           lenght           д�����ݵĳ��� 
* ���ڲ�������
* ˵    ����д���
*******************************************************************************/
void WriteSingle(unsigned char *pbuf, unsigned char lenght)
{
  unsigned char i,j;

  /* IO��ģ��SPIģʽ */
  L_SlaveSelect(); // SS�ܽ�����ͣ�SPI����
  CLKOFF();        // CLKʱ�ӹرգ��ͣ�
        
  while(lenght > 0)
  {
    *pbuf = (0x1f & *pbuf); // ȡ��5λB0-B4 �Ĵ�����ַ���� ��ʽΪ000XXXXX
            
    for(i = 0; i < 2; i++)  // �Ե����Ĵ������ȷ��͵�ַ���ٷ������ݻ�����
    {
      for(j = 0; j < 8; j++)
      {
        if (*pbuf & mask) // ��������λ
          SIMOON();
        else
          SIMOOFF();
                   
        CLKON(); // ����CLKʱ����Ϣ����������
        CLKOFF();
        mask >>= 1;  // ��־λ����
      }
                
      mask = 0x80;
      pbuf++;
      lenght--;
    }
  }

  H_SlaveSelect(); // SS�ܽ�����ߣ�SPIֹͣ
}


/*******************************************************************************
* �������ƣ�WriteCont()
* ��    �ܣ�����д�Ĵ������������ַ�Ķ���Ĵ�������
* ��ڲ�����*pbuf            ��Ҫд�������           
*           lenght           д�����ݵĳ��� 
* ���ڲ�������
* ˵    ����������ַд���
*******************************************************************************/
void WriteCont(unsigned char *pbuf, unsigned char lenght)
{
  unsigned char j;

  /* IO��ģ��SPIģʽ */
  L_SlaveSelect(); // SS�ܽ�����ͣ�SPI����
  CLKOFF(); // CLKʱ�ӹرգ��ͣ�

  *pbuf = (0x20 | *pbuf); // ȡλB5 �Ĵ�����ַ���� ������־λ ��ʽΪ001XXXXX
  *pbuf = (0x3f &*pbuf);  // ȡ��6λB0-B5 �Ĵ�����ַ����
  
  while(lenght > 0)
  {
    for(j=0;j<8;j++)
    {
      if (*pbuf & mask) // ��������λ
        SIMOON();
      else
        SIMOOFF();

      CLKON(); // ����CLKʱ����Ϣ����������
      CLKOFF();
      mask >>= 1; // ��־λ����
    }

    mask = 0x80;                            
    pbuf++;
    lenght--;
  }

  H_SlaveSelect(); // SS�ܽ�����ߣ�SPIֹͣ
}


/*******************************************************************************
* �������ƣ�ReadSingle()
* ��    �ܣ��������Ĵ���
* ��ڲ�����*pbuf            ��Ҫ��ȡ������           
*           lenght           ��ȡ���ݵĳ��� 
* ���ڲ�������
* ˵    ������
*******************************************************************************/
void ReadSingle(unsigned char *pbuf, unsigned char lenght)
{ 
  unsigned char j;

  /* IO��ģ��SPIģʽ */
  L_SlaveSelect(); // SS�ܽ�����ͣ�SPI����

  while(lenght > 0)
  {
    *pbuf = (0x40 | *pbuf); // ȡλB6 �Ĵ�����ַ���� ������־λ ��ʽΪ01XXXXXX
    *pbuf = (0x5f &*pbuf);  // ȡ��7λB0-B6 �Ĵ�����ַ����
    
    for(j = 0; j < 8; j++)
    {
      if (*pbuf & mask)  // ��������λ
        SIMOON();
      else
        SIMOOFF();
                
      CLKON();  // ����CLKʱ����Ϣ����������
      CLKOFF();
      mask >>= 1;  // ��־λ����
    } 
    
    mask = 0x80;
    *pbuf = 0;     // ��ʼ��ȡ����
            
    for(j = 0; j < 8; j++)
    {
      *pbuf <<= 1; // ��������
      CLKON();     // ����CLKʱ����Ϣ����������
      CLKOFF();

      if (SOMISIGNAL) // �ж�SOMI����
        *pbuf |= 1;  // ��Ϊ�ߵ�ƽ�������ݻ��� 1
    } 

    pbuf++;
    lenght--;
  }

  H_SlaveSelect(); // SS�ܽ�����ߣ�SPIֹͣ
} 


/*******************************************************************************
* �������ƣ�ReadCont()
* ��    �ܣ��������Ĵ������������ַ�Ķ���Ĵ�������
* ��ڲ�����*pbuf            ��Ҫ��ȡ������           
*           lenght           ��ȡ���ݵĳ��� 
* ���ڲ�������
* ˵    ����������ַд���
*******************************************************************************/
void ReadCont(unsigned char *pbuf, unsigned char lenght)
{
  unsigned char j;
  
  /* IO��ģ��SPIģʽ */
  L_SlaveSelect();        // SS�ܽ�����ͣ�SPI����
  *pbuf = (0x60 | *pbuf); // ȡλB6B5 �Ĵ�����ַ���� ������־λ ��ʽΪ011XXXXX
  *pbuf = (0x7f & *pbuf); // ȡ��7λB0-B6 �Ĵ�����ַ����

  for(j = 0; j < 8; j++)  // д�Ĵ�����ַ
  {
    if (*pbuf & mask)     // ��������λ
      SIMOON();
    else
      SIMOOFF();
     
    CLKON();    // ����CLKʱ����Ϣ����������
    CLKOFF();
    mask >>= 1; // ��־λ����
  }
  
  mask = 0x80;

  while(lenght > 0)  // ��ʼ��ȡ����
  {
    *pbuf = 0;  // ��ջ�����
    
    for(j = 0; j < 8; j++)
    {
      *pbuf <<= 1;  // ��������
      CLKON();      // ����CLKʱ����Ϣ����������
      CLKOFF();
      if (SOMISIGNAL)
        *pbuf |= 1;
    }

    pbuf++;   // ����������
    lenght--;
  }

  H_SlaveSelect(); // SS�ܽ�����ߣ�SPIֹͣ
}  


/*******************************************************************************
* �������ƣ�DirectCommand()
* ��    �ܣ�ֱ������ɷ���һ������Ķ���оƬ
* ��ڲ�����*pbuf            ��Ҫ���͵���������           
* ���ڲ�������
* ˵    ����ֱ�����
*******************************************************************************/
void DirectCommand(unsigned char *pbuf)
{
  unsigned char j;

  /* IO��ģ��SPIģʽ */
  L_SlaveSelect();        // SS�ܽ�����ͣ�SPI����
  *pbuf = (0x80 | *pbuf); // ȡλB7 �Ĵ�����ַ���� �����־λ ��ʽΪ1XXXXXXX
  *pbuf = (0x9f & *pbuf); // ����ֵ

  for(j = 0; j < 8; j++)  // д�Ĵ�����ַ
  {
    if (*pbuf & mask)  // ��������λ
      SIMOON();
    else
      SIMOOFF();

    CLKON();  // ����CLKʱ����Ϣ����������
    CLKOFF();
    mask >>= 1; // ��־λ����
  } 
  mask = 0x80;

  CLKON();   // ���Ӷ���ʱ������
  CLKOFF();
  H_SlaveSelect();  // SS�ܽ�����ߣ�SPIֹͣ
}   


/*******************************************************************************
* �������ƣ�RAWwrite()
* ��    �ܣ�ֱ��д���ݻ�����Ķ���оƬ
* ��ڲ�����*pbuf           ��Ҫ���͵���������    
*           lenght          д�����ݻ�����ĳ���    
* ���ڲ�������
* ˵    ����ֱ��д��
*******************************************************************************/
void RAWwrite(unsigned char *pbuf, unsigned char lenght)
{ 
  unsigned char j;

  /* IO��ģ��SPIģʽ */
  L_SlaveSelect(); // SS�ܽ�����ͣ�SPI����
  
  while(lenght > 0)
  {
    for(j = 0; j < 8; j++) // д�Ĵ�����ַ
    {
      if (*pbuf & mask)  // ��������λ
        SIMOON();
      else
        SIMOOFF();

      CLKON();  // ����CLKʱ����Ϣ����������
      CLKOFF();
      mask >>= 1; // ��־λ����
    }
            
    mask = 0x80;
 
    pbuf++;
    lenght--;
  }  

  H_SlaveSelect(); // SS�ܽ�����ߣ�SPIֹͣ
}


/*******************************************************************************
* �������ƣ�InitialSettings()
* ��    �ܣ���ʼ��TRF7960����
* ��ڲ������� 
* ���ڲ�������
* ˵    ��������Ƶ��������������
*******************************************************************************/
void InitialSettings(void)
{
  unsigned char command[2];

  command[0] = ModulatorControl;                  
  command[1] = 0x21;  // ���ƺ�ϵͳʱ�ӿ��ƣ�0x21 - 6.78MHz OOK(100%)
  //command[1] = 0x31; // ���ƺ�ϵͳʱ�ӿ��ƣ�0x31 - 13.56MHz OOK(100%)

  WriteSingle(command, 2);
}


/*******************************************************************************
* �������ƣ�ReInitialize15693Settings()
* ��    �ܣ����³�ʼ��ISO15693����
* ��ڲ������� 
* ���ڲ�������
* ˵    �����������³�ʼ��������Ӧ��ȴ�ʱ��ͽ��յȴ�ʱ��
*******************************************************************************/
void ReInitialize15693Settings(void)
{
  unsigned char command[2];

  command[0] = RXNoResponseWaitTime;  // ������Ӧ��ȴ�ʱ��
  command[1] = 0x14;
  WriteSingle(command, 2);

  command[0] = RXWaitTime; // ���յȴ�ʱ��                   
  command[1] = 0x20;
  WriteSingle(command, 2);
}


/*******************************************************************************
* �������ƣ�InterruptHandlerReader()
* ��    �ܣ��Ķ����жϴ������
* ��ڲ�����*Register           �ж�״̬�Ĵ��� 
* ���ڲ�������
* ˵    ���������ⲿ�жϷ������
*           IRQ�ж�״̬�Ĵ���˵�����£�
*
*   λ      λ����      ����                         ˵��
*   B7      Irq_tx      TX������IRQ��λ         ָʾTX���ڴ����С��ñ�־λ��TX��ʼʱ�����ã������ж���������TX���ʱ�ŷ��͡�
*   B6      Irq_srx     RX��ʼ��IRQ��λ         ָʾRX SOF�Ѿ������յ�����RX���ڴ����С��ñ�־λ��RX��ʼʱ�����ã������ж���������RX���ʱ�ŷ��͡�
*   B5      Irq_fifo    ָʾFIFOΪ1/3>FIFO>2/3  ָʾFIFO�߻��ߵͣ�С��4���ߴ���8����
*   B4      Irq_err1    CRC����                 ����CRC
*   B3      Irq_err2    ��żУ�����            (��ISO15693��Tag-itЭ����δʹ��)
*   B2      Irq_err3    �ֽڳ�֡����EOF���� 
*   B1      Irq_col     ��ײ����                ISO14443A��ISO15693�����ز���
*   B0      Irq_noresp  ����Ӧ�ж�              ָʾMCU���Է�����һ�������
*******************************************************************************/
void InterruptHandlerReader(unsigned char *Register)
{
  unsigned char len;

  if(*Register == 0xA0) // A0 = 10100000 ָʾTX���ͽ�����������FIFO��ʣ��3�ֽ�����
  {                
    i_reg = 0x00;
  }

  else if(*Register == BIT7) // BIT7 = 10000000 ָʾTX���ͽ���
  {            
    i_reg = 0x00;
    *Register = Reset;   // ��TX���ͽ����� ִ�и�λ����
    DirectCommand(Register);
  }

  else if((*Register & BIT1) == BIT1) // BIT1 = 00000010 ��ײ����
  {                           
    i_reg = 0x02; // ����RX����

    *Register = StopDecoders; // ��TX���ͽ�����λFIFO
    DirectCommand(Register);

    CollPoss = CollisionPosition;
    ReadSingle(&CollPoss, 1);

    len = CollPoss - 0x20; // ��ȡFIFO�е���Ч�����ֽ�����

    if((len & 0x0f) != 0x00) 
      len = len + 0x10; // ������յ��������ֽڣ������һ���ֽ�
    len = len >> 4;

    if(len != 0x00)
    {
      rfid_buf[RXTXstate] = FIFO; // �����յ�������д���������ĵ�ǰλ��                               
      ReadCont(&rfid_buf[RXTXstate], len);
      RXTXstate = RXTXstate + len;
    }

    *Register = Reset;       // ִ�и�λ����
    DirectCommand(Register);

    *Register = IRQStatus;  // ��ȡIRQ�ж�״̬�Ĵ�����ַ
    *(Register + 1) = IRQMask;

    if (SPIMODE)  // ��ȡ�Ĵ���
      ReadCont(Register, 2);
    else
      ReadSingle(Register, 1);

    IRQCLR();  // ���ж�
  }
  else if(*Register == BIT6) // BIT6 = 01000000 ���տ�ʼ
  {   
    if(RXErrorFlag == 0x02) // RX���ձ�־λָʾEOF�Ѿ������գ�����ָʾ��FIFO�Ĵ�����δ�����ֽڵ�����
    {
      i_reg = 0x02;
      return;
    }

    *Register = FIFOStatus;
    ReadSingle(Register, 1); // ��ȡ��FIFO��ʣ���ֽڵ�����
    *Register = (0x0F & *Register) + 0x01;
    rfid_buf[RXTXstate] = FIFO; // �����յ�������д���������ĵ�ǰλ��
                                                                                	
    ReadCont(&rfid_buf[RXTXstate], *Register);
    RXTXstate = RXTXstate +*Register;

    *Register = TXLenghtByte2; // ��ȡ�Ƿ��в��������ֽڼ���λ����
    ReadCont(Register, 1);

    if((*Register & BIT0) == BIT0) // 00000001 ����Ӧ�ж�
    {
      *Register = (*Register >> 1) & 0x07; // ���ǰ5λ
      *Register = 8 - *Register;
      rfid_buf[RXTXstate - 1] &= 0xFF << *Register;
    }

    *Register = Reset;   // ���һ���ֽڱ���ȡ��λFIFO
    DirectCommand(Register);

    i_reg = 0xFF; // ָʾ���պ�����Щ�ֽ��Ѿ�������ֽ�
  }
  else if(*Register == 0x60) // 0x60 = 01100000 RX�Ѿ���� ������9���ֽ���FIFO��
  {                            
    i_reg = 0x01; // ���ñ�־λ
    rfid_buf[RXTXstate] = FIFO;
    ReadCont(&rfid_buf[RXTXstate], 9); // ��FIFO�ж�ȡ9���ֽ�����
    RXTXstate = RXTXstate + 9;

    if(IRQPORT & IRQPin) // ����жϹܽ�Ϊ�ߵ�ƽ
    {
      *Register = IRQStatus; // ��ȡIRQ�ж�״̬�Ĵ�����ַ
      *(Register + 1) = IRQMask;
      
      if (SPIMODE) // ��ȡ�Ĵ���
        ReadCont(Register, 2);
      else
        ReadSingle(Register, 1);
            
      IRQCLR(); // ���ж�

      if(*Register == 0x40) // 0x40 = 01000000 ���ս���
      {  
        *Register = FIFOStatus;
        ReadSingle(Register, 1); // ��ȡ��FIFO��ʣ���ֽڵ�����
        *Register = 0x0F & (*Register + 0x01);
        rfid_buf[RXTXstate] = FIFO; // �����յ�������д���������ĵ�ǰλ��
                                                                                	
        ReadCont(&rfid_buf[RXTXstate], *Register);
        RXTXstate = RXTXstate +*Register;

        *Register = TXLenghtByte2; // ��ȡ�Ƿ��в��������ֽڼ���λ����
        ReadSingle(Register, 1);         

        if((*Register & BIT0) == BIT0) // 00000001 ����Ӧ�ж�
        {
          *Register = (*Register >> 1) & 0x07; // ���ǰ5λ
          *Register = 8 -*Register;
          rfid_buf[RXTXstate - 1] &= 0xFF << *Register;
        }   

        i_reg = 0xFF;      // ָʾ���պ�����Щ�ֽ��Ѿ�������ֽ�
        *Register = Reset; // ������ֽڱ���ȡ��λFIFO
        DirectCommand(Register);
      }
      else if(*Register == 0x50) // 0x50 = 01010000���ս������ҷ���CRC����
      {        
        i_reg = 0x02;
      }
    } 
    else                                        
    {
      Register[0] = IRQStatus; // ��ȡIRQ�ж�״̬�Ĵ�����ַ
      Register[1] = IRQMask;
      
      if (SPIMODE)
        ReadCont(Register, 2); // ��ȡ�Ĵ���
      else
        ReadSingle(Register, 1);
            
      if(Register[0] == 0x00) 
        i_reg = 0xFF; // ָʾ���պ�����Щ�ֽ��Ѿ�������ֽ�
        }
    }
    else if((*Register & BIT4) == BIT4) // BIT4 = 00010000 ָʾCRC����
    {                      
      if((*Register & BIT5) == BIT5) // BIT5 = 00100000 ָʾFIFO����9���ֽ�
      {
        i_reg = 0x01;   // �������
        RXErrorFlag = 0x02;
      }
      else
        i_reg = 0x02; // ֹͣ����        
    }
    else if((*Register & BIT2) == BIT2) // BIT2 = 00000100  �ֽڳ�֡����EOF����
    {                       
      if((*Register & BIT5) == BIT5) // BIT5 = 00100000 ָʾFIFO����9���ֽ�
      {
        i_reg = 0x01;  // �������
        RXErrorFlag = 0x02;
      }
      else
        i_reg = 0x02; // ֹͣ���� 
    }
    else if(*Register == BIT0) // BIT0 = 00000001 �ж���Ӧ��
    {                      
      i_reg = 0x00;
    }
    else
    {   
      i_reg = 0x02;

      *Register = StopDecoders; // ��TX���ͽ��պ�λFIFO
      DirectCommand(Register);

      *Register = Reset;
      DirectCommand(Register);

      *Register = IRQStatus;  // ��ȡIRQ�ж�״̬�Ĵ�����ַ
      *(Register + 1) = IRQMask;

      if (SPIMODE)
        ReadCont(Register, 2); // ��ȡ�Ĵ���
      else
        ReadSingle(Register, 1);
        
      IRQCLR(); // ���ж�
  }
}


/*******************************************************************************
 * �������ƣ�HAL_ISR_FUNCTION
 * ��    �ܣ�P0�������жϵ��жϷ������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ******************************************************************************/
HAL_ISR_FUNCTION( halRFIDPort0Isr, P0INT_VECTOR )
{
  unsigned char Register[4];
  
  if (HAL_RFID_PORT_PXIFG & HAL_RFID_BIT)
  {
    do
    {
      IRQCLR(); // ��˿�2�жϱ�־λ
      Register[0] = IRQStatus; // ��ȡIRQ�ж�״̬�Ĵ�����ַ
      Register[1] = IRQMask;   // ����� Dummy read
  
      if (SPIMODE)  // ��ȡ�Ĵ���
        ReadCont(Register, 2);
      else
        ReadSingle(Register, 1); 

      if(*Register == 0xA0) // A0 = 10100000 ָʾTX���ͽ�����������FIFO��ʣ��3�ֽ�����
      {   
        goto FINISH; // ��ת��FINISH��
      }
        
      InterruptHandlerReader(&Register[0]); // ִ���жϷ������
    }while((IRQPORT & IRQPin) == IRQPin);
  }
FINISH:
  ;  
}


/*******************************************************************************
 * �������ƣ�TRF7960_IO_Init_SPI
 * ��    �ܣ��Կ���TRF7960��IO���г�ʼ��,����SPI��ʽ
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ******************************************************************************/
void TRF7960_IO_Init_SPI(void)
{
  /* P0.0,P0.1,P0.7Ϊͨ��IO����Ӱ��P0���������� */
  P0SEL &= ~0x83;
  
  /* P1.0,P1.4Ϊͨ��IO����Ӱ��P1���������� */
  P1SEL &= ~0x11; 
  
  /* P0.0,P0.7����Ϊ��� */
  P0DIR |= 0x81;
  
  /* P1.0����Ϊ��� */
  P1DIR |= 0x01;
  
  /* P0.1����Ϊ���� */
  P0DIR &= ~0x02;
   
  /* P1.4����Ϊ���� */
  P1DIR &= ~0x10;
  
  /* P0.1Ϊ�������� */
  P0INP &= ~0x02;  // P0.1ѡ��Ϊ����/������������̬��ģʽ
  P2INP &= ~0x20;  // ��P0������ѡ��Ϊ����/������������̬��ģʽ����������Ϊ���� 
  
  /* P1.4Ϊ�������� */
  P1INP &= ~0x10;  // P1.4ѡ��Ϊ����/������������̬��ģʽ
  P2INP &= ~0x40;  // ��P1������ѡ��Ϊ����/������������̬��ģʽ����������Ϊ����
  
  /* P0.1�����ж�Ϊ�����ش�������ʹ��P0.1�жϣ�δ��ȫ���жϣ�*/
  PICTL &= ~0x01;  // P0�������ж�Ϊ�����ش���
  P0IEN |= 0x02;   // P0.1�ж�ʹ��
} 


/*******************************************************************************
 * �������ƣ�HalRFIDInit
 * ��    �ܣ�CC253x����TRF7960�ܽ����ü�TRF7960оƬ��ʼ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 *******************************************************************************/
void HalRFIDInit( void )
{
  TRF7960_IO_Init_SPI();
  //InitialSettings(); // ��ʼ�����ã�����MSP430ʱ��Ƶ��Ϊ6.78MHz��OOK����ģʽ
}


/*******************************************************************************
* �������ƣ�EnableSlotCounter()
* ��    �ܣ�ʹ�ܲۼ������ܡ�
* ��ڲ�������
* ���ڲ�������     
* ˵    �����ú���ʹ�ܲۼ������ܣ����ڶ����ʱ��
*******************************************************************************/
void EnableSlotCounter(void)
{
  rfid_buf[41] = IRQMask;  // �¸�������
  rfid_buf[40] = IRQMask;
  ReadSingle(&rfid_buf[41], 1); // ��ȡ����������
  rfid_buf[41] |= BIT0;         // �ڻ������Ĵ���0x41λ������BIT0��Ч
  WriteSingle(&rfid_buf[40], 2);
}


/*******************************************************************************
* �������ƣ�DisableSlotCounter()
* ��    �ܣ���ֹ�ۼ������ܡ�
* ��ڲ�������
* ���ڲ�������     
* ˵    �����ú���ʹ�ۼ�������ֹͣ��
*******************************************************************************/
void DisableSlotCounter(void)
{
  rfid_buf[41] = IRQMask;  // �¸�������
  rfid_buf[40] = IRQMask;
  ReadSingle(&rfid_buf[41], 1); // ��ȡ����������
  rfid_buf[41] &= 0xfe;         // �ڻ������Ĵ���0x41λ������BIT0��Ч
  WriteSingle(&rfid_buf[40], 2);
}


/*******************************************************************************
* �������ƣ�InventoryRequest()
* ��    �ܣ�ISO15693Э�鿨Ƭ�����������
* ��ڲ�����*mask       �������
*           lenght      �����
* ���ڲ�������     
* ˵    ����ִ�иú�������ʹISO15693Э���׼��������ѭ��16ʱ��ۻ���1��ʱ���.
*           ���У�0x14��ʾ16�ۣ�0x17��ʾ1���ۡ�
*           ע�⣺���ѻ�ģʽ�£����յ�UID�뽫����ʾ��LCMͼ����ʾ���ϡ�
*******************************************************************************/
void InventoryRequest(unsigned char *mask, unsigned char lenght)
{
  unsigned char i = 1, j, command[2], NoSlots, found = 0;
  unsigned char *PslotNo, slotNo[17];
  unsigned char NewMask[8], NewLenght, masksize;
  int size;
  unsigned int k = 0;

  rfid_buf[0] = ModulatorControl; // ���ƺ�ϵͳʱ�ӿ��ƣ�0x21 - 6.78MHz OOK(100%)
  rfid_buf[1] = 0x21;
  WriteSingle(rfid_buf, 2);
 
  /* ���ʹ��SPI����ģʽ�ĵ������ʣ���ô RXNoResponseWaitTime ��Ҫ���������� */
  if(SPIMODE)
  {
    if((rfid_flags & BIT1) == 0x00) // �����ݱ�����
    {
      rfid_buf[0] = RXNoResponseWaitTime;
      rfid_buf[1] = 0x2F;
      WriteSingle(rfid_buf, 2);
    }
    else // �����ݱ�����
    {
      rfid_buf[0] = RXNoResponseWaitTime;
      rfid_buf[1] = 0x13;
      WriteSingle(rfid_buf, 2);
    }
  }
       
  slotNo[0] = 0x00;

  if((rfid_flags & BIT5) == 0x00) // λ5��־λָʾ�۵�����
  {                       
    NoSlots = 17; // λ5Ϊ0x00����ʾѡ��16��ģʽ
    EnableSlotCounter();
  }
  else // ���λ5��Ϊ0x00����ʾѡ��1����ģʽ
    NoSlots = 2;

  PslotNo = &slotNo[0]; // ������ָ��
    
  /* ���lenght��4����8����ômasksize ��Ǵ�СֵΪ 1  */
  /* ���lenght��12����16����ômasksize ��Ǵ�СֵΪ 2������������ */
  masksize = (((lenght >> 2) + 1) >> 1);
  size = masksize + 3; // mask value + mask lenght + command code + rfid_flags

  rfid_buf[0] = 0x8f;
  rfid_buf[1] = 0x91;  // ���ʹ�CRCУ��
  rfid_buf[2] = 0x3d;  // ����дģʽ
  rfid_buf[3] = (char) (size >> 8);
  rfid_buf[4] = (char) (size << 4);
  rfid_buf[5] = rfid_flags;  // ISO15693 Э���־rfid_flags
  rfid_buf[6] = 0x01;        // �³�ײ����ֵ

  /* �����ڴ˼���AFIӦ�����ʶ�� */
  
  rfid_buf[7] = lenght; // ��ǳ��� masklenght
    
  if(lenght > 0)
  {
    for(i = 0; i < masksize; i++) 
      rfid_buf[i + 8] = *(mask + i);
  }                   

  command[0] = IRQStatus;
  command[1] = IRQMask;  // �����(Dummy read)
  ReadCont(command, 1);

  IRQCLR();  // ���жϱ�־λ
  IRQON();   // �жϿ���

  RAWwrite(&rfid_buf[0], masksize + 8); // ������д�뵽FIFO��������

  i_reg = 0x01;  // �����жϱ�־ֵ
  halMcuWaitMs(20);

  for(i = 1; i < NoSlots; i++) // Ѱ��ѭ��1���ۻ���16����
  {       
    /* ��ʼ��ȫ�ּ����� */
    RXTXstate = 1; // ���ñ�־λ�������λ�洢��rfid_buf[1]��ʼλ��

    k = 0;
    halMcuWaitMs(20);
                
    while(i_reg == 0x01) // �ȴ�RX���ս���
    {           
      k++;

      if(k == 0xFFF0)
      {
        i_reg = 0x00;
        RXErrorFlag = 0x00;
        break;
      }
    }

    command[0] = RSSILevels; // ��ȡ�ź�ǿ��ֵ RSSI
    ReadSingle(command, 1);

    if(i_reg == 0xFF) // ����ֽ��Ѿ�������ֽڣ����յ�UID����
    {     
      found = 1;
                 
      for(j = 3; j < 11; j++)
      {
        UIDString[j-3] = rfid_buf[j];  // ����ISO15693 UID��
      }

      UIDString[8] = command[0];
    }
    else if(i_reg == 0x02) // ����г�ײ����
    { 
      PslotNo++;
      *PslotNo = i;
    }
    else if(i_reg == 0x00) // �����ʱʱ�䵽���жϷ���
      ;
    else
      ;

    command[0] = Reset;    // �ڽ�����һ����֮ǰ��ʹ��ֱ�����λFIFO
    DirectCommand(command);

    if((NoSlots == 17) && (i < 16)) // �����16����ģʽ�£�δѭ��16���ۣ�����Ҫ����EOF����(�¸���)
    {                   
      command[0] = StopDecoders;
      DirectCommand(command);  // ֹͣ����
      command[0] = RunDecoders;               
      DirectCommand(command);             
      command[0] = TransmitNextSlot;
      DirectCommand(command); // ������һ����
    }
    else if((NoSlots == 17) && (i == 16)) // �����16����ģʽ�£�ѭ����16���ۣ�����Ҫ����ֹͣ�ۼ�������
    {                   
      DisableSlotCounter(); // ֹͣ�ۼ���
    }
    else if(NoSlots == 2) // ����ǵ�����ģʽ���������� for ѭ��
      break;
  }

  if(found)
  {                       
    uidfound = 1;
  }
  else
  {
    found = 0;
    uidfound = 0;
  }

  NewLenght = lenght + 4;  // ��ǳ���Ϊ4����λ����
  masksize = (((NewLenght >> 2) + 1) >> 1) - 1;

  /* �����16����ģʽ������ָ�벻Ϊ0x00, ��ݹ���ñ��������ٴ�Ѱ�ҿ�Ƭ */
  while((*PslotNo != 0x00) && (NoSlots == 17)) 
  {
    *PslotNo = *PslotNo - 1;
    for(i = 0; i < 8; i++) 
      NewMask[i] = *(mask + i); //���Ƚ����ֵ�������±��������

    if((NewLenght & BIT2) == 0x00)
      *PslotNo = *PslotNo << 4;

    NewMask[masksize] |= *PslotNo;  // ���ֵ���ı�
    InventoryRequest(&NewMask[0], NewLenght); // �ݹ���� InventoryRequest ����
    PslotNo--; // �۵ݼ�
  }  
       
  IRQOFF(); // �³�ײ���̽������ر��ж�
}  


/*******************************************************************************
* �������ƣ�RequestCommand()
* ��    �ܣ���ƬЭ�������������ͼ�ʱ�Ķ�������Ƭ��Ӧ��
* ��ڲ�����*prfid_buf           ����ֵ
*           lenght          �����
*           brokenBits      �������ֽڵ�λ����
*           noCRC           �Ƿ���CRCУ��
* ���ڲ�����1     
* ˵    �����ú���ΪЭ���������������1����˵���ú����ɹ�ִ�У�
*           ������0���߲����أ����쳣��
*******************************************************************************/
unsigned char RequestCommand(unsigned char *pbuf, unsigned char lenght, unsigned char brokenBits, char noCRC)
{
  unsigned char index, command;                

  RXTXstate = lenght;                             
  *pbuf = 0x8f;
  
  if(noCRC) 
    *(pbuf + 1) = 0x90; // ���䲻��CRCУ��
  else
    *(pbuf + 1) = 0x91; // �����CRCУ��
    
  *(pbuf + 2) = 0x3d;
  *(pbuf + 3) = RXTXstate >> 4;
  *(pbuf + 4) = (RXTXstate << 4) | brokenBits;

  if(lenght > 12)
    lenght = 12;

  if(lenght == 0x00 && brokenBits != 0x00)
  {
    lenght = 1;
    RXTXstate = 1;
  }

  RAWwrite(pbuf, lenght + 5); // ��ֱ��дFIFOģʽ��������

  IRQCLR(); // ���жϱ�־λ
  IRQON();

  RXTXstate = RXTXstate - 12;
  index = 17;
  i_reg = 0x01;
    
  while(RXTXstate > 0)
  {
    if(RXTXstate > 9) // ��RXTXstateȫ�ֱ�����δ���͵��ֽ������������9             
      lenght = 10;  // ����Ϊ10�����а���FIFO�е�9���ֽڼ�1���ֽڵĵ�ֵַ
    else if(RXTXstate < 1)  // �����ֵС��1����˵�����е��ֽ��Ѿ����͵�FIFO�У������жϷ���
      break;
    else   // ���е�ֵ�Ѿ�ȫ��������
      lenght = RXTXstate + 1;         

    rfid_buf[index - 1] = FIFO; // ��FIFO��������д��9�����߸����ֽڵ����ݣ������ڷ���
    WriteCont(&rfid_buf[index - 1], lenght);
    RXTXstate = RXTXstate - 9; // д9���ֽڵ�FIFO��
    index = index + 9;
  } 

  RXTXstate = 1; // ���ñ�־λ�������λ�洢��rfid_buf[1]��ʼλ��

  while(i_reg == 0x01) // �ȴ����ͽ���
    halMcuWaitMs(60);

  i_reg = 0x01;
  halMcuWaitMs(60);

  /* ����жϱ�־λ�������ȸ�λ�����¸������� */
  if((((rfid_buf[5] & BIT6) == BIT6) && ((rfid_buf[6] == 0x21) || (rfid_buf[6] == 0x24) || (rfid_buf[6] == 0x27) || (rfid_buf[6] == 0x29)))
    || (rfid_buf[5] == 0x00 && ((rfid_buf[6] & 0xF0) == 0x20 || (rfid_buf[6] & 0xF0) == 0x30 || (rfid_buf[6] & 0xF0) == 0x40)))
  {
    halMcuWaitMs(60);
    command = Reset;
    DirectCommand(&command);
    command = TransmitNextSlot;
    DirectCommand(&command);
  }  
  
  while(i_reg == 0x01); // �ȴ��������

  IRQOFF(); // �ر��ж�
  
  return(1); // ����ȫ��ִ����ϣ����� 1
}

#endif



