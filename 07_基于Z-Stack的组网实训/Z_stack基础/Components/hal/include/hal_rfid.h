/*******************************************************************************
 * �ļ����ƣ�hal_rfid.h
 * ��    �ܣ�RFIDоƬTRF7960����ͷ�ļ�
 * Ӳ�����ӣ�TRF7960��CC253x��Ӳ�����ӹ�ϵ���£�
 *
 *          TRF7960                 CC253x
 *           IRQ                     P0.1
 *           CLK                     P1.0
 *           MOSI                    P0.7
 *           MISO                    P1.4
 *           SS                      P0.0
 ******************************************************************************/


#ifndef HAL_RFID_H
#define HAL_RFID_H

#ifdef __cplusplus
extern "C"
{
#endif

/* TRF7960����/����ͨ��ģʽѡ�� */
/********************************************************************/
#define PAR                 0
#define SPI                 1
#define SPIMODE             SPI     
/********************************************************************/
  
  
/* �ⲿ�������� */
/********************************************************************/
extern unsigned char rfid_flags; // ����洢��־λ(�ڷ³�ײ��ʹ��)
extern unsigned char UIDString[9];
extern bool uidfound;


/* �ⲿ�������� */
/********************************************************************/
extern void halMcuWaitMs(uint16 msec);
extern void WriteSingle(unsigned char *pbuf, unsigned char lenght);
extern void ReadSingle(unsigned char *pbuf, unsigned char lenght);
extern void ReadCont(unsigned char *pbuf, unsigned char lenght);
extern void InitialSettings(void);
extern void HalRFIDInit( void );
extern void InventoryRequest(unsigned char *mask, unsigned char lenght);
/********************************************************************/


/* �궨�� */
/********************************************************************/

/* 
  λ���� 
 */
#define BIT0                (0x0001)
#define BIT1                (0x0002)
#define BIT2                (0x0004)
#define BIT3                (0x0008)
#define BIT4                (0x0010)
#define BIT5                (0x0020)
#define BIT6                (0x0040)
#define BIT7                (0x0080)
#define BIT8                (0x0100)
#define BIT9                (0x0200)
#define BITA                (0x0400)
#define BITB                (0x0800)
#define BITC                (0x1000)
#define BITD                (0x2000)
#define BITE                (0x4000)
#define BITF                (0x8000)


/* TRF7961ʱ��CLK�ܽ����� - CC2530�ܽ�P1.0 */
/* CLK          -       P1.0 */
#define DATA_CLK_PORT       P1_0   //CLK�ܽŶ���
#define DATA_CLK_BIT        BV(0)
#define DATA_CLK_SEL        P1SEL
#define DATA_CLK_DIR        P1DIR

/*-----------------------------------------------------------------------------*/
#define CLKGPIOset()        DATA_CLK_SEL &= ~DATA_CLK_BIT  //����ΪGPIO����
#define CLKFUNset()         DATA_CLK_SEL |= DATA_CLK_BIT   //����Ϊ�����ܺ���
#define CLKPOUTset()        DATA_CLK_DIR |= DATA_CLK_BIT   //���÷���Ϊ���
#define CLKON()             DATA_CLK_PORT = 1              //����ߵ�ƽ
#define CLKOFF()            DATA_CLK_PORT = 0              //����͵�ƽ
/*-----------------------------------------------------------------------------*/


/* TRF7960����SlaveSelect�ܽ����� - CC2530 CS�ܽ�P0.0 */
/* SlaveSelect  -       P0.0 */
/*-----------------------------------------------------------------------------*/
#define SlaveSelectPin          BV(0)
#define SlaveSelectPort         P0_0
#define SlaveSelectGPIOset()    P0DIR &= ~SlaveSelectPin    //����ΪGPIO
#define SlaveSelectPortSet()    P0DIR |= SlaveSelectPin     //���÷���Ϊ���
#define H_SlaveSelect()         SlaveSelectPort = 1     //����ߵ�ƽ
#define L_SlaveSelect()         SlaveSelectPort = 0     //����͵�ƽ
/*-----------------------------------------------------------------------------*/


/* TRF7960����SIMO�ܽ����� - CC2530�ܽ�P0.7 */
/* SIMO         -       P0.7 */
/*-----------------------------------------------------------------------------*/
#define SIMOPin             BV(7)
#define SIMOPort            P0_7
#define SIMOGPIOset()       P1SEL &= ~SIMOPin  //����ΪGPIO����
#define SIMOFUNset()        P1SEL |= SIMOPin   //����Ϊ�����ܺ���
#define SIMOSet()           P1DIR |= SIMOPin           //���÷���Ϊ���
#define SIMOON()            SIMOPort = 1               //����ߵ�ƽ
#define SIMOOFF()           SIMOPort = 0               //����͵�ƽ
/*-----------------------------------------------------------------------------*/


/* TRF7960����SOMI�ܽ����� - CC2530�ܽ�P1.4 */
/* SOMI         -       P1.4 */
/*-----------------------------------------------------------------------------*/
#define SOMIPin             BV(4) 
#define SOMISIGNAL          P1_4
#define SOMIGPIOset()       P1SEL &= ~SOMIPin  //����ΪGPIO����
#define SOMIFUNset()        P1SEL |= SOMIPin   //����Ϊ�����ܺ���
#define SOMISet()           P1DIR |= SOMIPin           //���÷���Ϊ���
/*-----------------------------------------------------------------------------*/


/* 
  �жϹܽŶ��� 
  TRF7960�ж�IRQ�ܽ����� - CC2530�ܽ�P0.1
  IRQ          -       P0.1
 */
#define IRQ                    P0_1   //IRQ�жϹܽŶ���
#define HAL_RFID_PORT          P0
#define HAL_RFID_BIT           BV(1)
#define HAL_RFID_SEL           P0SEL
#define HAL_RFID_DIR           P0DIR

#define HAL_RFID_PORT_EDGEBIT  BV(0)  // �жϱ��ض���
#define HAL_RFID_PORT_EDGE     HAL_KEY_FALLING_EDGE// �½���

#define HAL_RFID_PORT_IEN      IEN1   // CPU�жϼĴ�����־
#define HAL_RFID_PORT_IENBIT   BV(5)  // �˿�0�жϱ�־λ
#define HAL_RFID_PORT_ICTL     P0IEN  // �˿��жϿ��ƼĴ���
#define HAL_RFID_PORT_ICTLBIT  BV(1)  // P0IEN - P0.1 ʹ��/��ֹλ
#define HAL_RFID_PORT_PXIFG    P0IFG  // �жϱ�־
/********************************************************************/

#define IRQPin              BV(1)                        //��1��
#define IRQPORT             P0                           //�˿�2
#define IRQPinset()         P0DIR &= ~IRQPin             //���÷���Ϊ����
#define IRQON()             HAL_RFID_PORT_ICTL |= HAL_RFID_PORT_ICTLBIT; \
                            HAL_RFID_PORT_IEN |= HAL_RFID_PORT_IENBIT;    //IRQ �жϿ���

#define IRQOFF()            HAL_RFID_PORT_ICTL &= ~HAL_RFID_PORT_ICTLBIT; \
                            HAL_RFID_PORT_IEN &= ~HAL_RFID_PORT_IENBIT;   //IRQ �жϹر�

#define IRQEDGEset()        PICTL &= ~HAL_RFID_PORT_EDGEBIT;            //�������ж�

#define IRQCLR()            HAL_RFID_PORT_PXIFG &= ~0x02; \
                            P0IF = 0;
                              

/* TRF7960оƬ��ַ���� */
/*=============================================================================*/
#define ChipStateControl        0x00                        //оƬ״̬���ƼĴ���
#define ISOControl              0x01                        //ISOЭ����ƼĴ���
#define ISO14443Boptions        0x02                        //ISO14443BЭ���ѡ�Ĵ���
#define ISO14443Aoptions        0x03                        //ISO14443AЭ���ѡ�Ĵ���
#define TXtimerEPChigh          0x04                        //TX���͸��ֽڼĴ���
#define TXtimerEPClow           0x05                        //TX���͵��ֽڼĴ���
#define TXPulseLenghtControl    0x06                        //TX�������峤�ȿ��ƼĴ���
#define RXNoResponseWaitTime    0x07                        //RX������Ӧ��ȴ�ʱ��Ĵ���
#define RXWaitTime              0x08                        //RX���յȴ�ʱ��Ĵ���
#define ModulatorControl        0x09                        //��������ϵͳʱ�ӼĴ���
#define RXSpecialSettings       0x0A                        //RX�����������üĴ���
#define RegulatorControl        0x0B                        //��ѹ������I/O���ƼĴ���
#define IRQStatus               0x0C                        //IRQ�ж�״̬�Ĵ���
#define IRQMask                 0x0D                        //��ײλ�ü��жϱ�ǼĴ���
#define CollisionPosition       0x0E                        //��ײλ�üĴ���
#define RSSILevels              0x0F                        //�źŽ���ǿ�ȼĴ���
#define RAMStartAddress         0x10                        //RAM ��ʼ��ַ����7�ֽڳ� (0x10 - 0x16)
#define NFCID                   0x17                        //��Ƭ��ʶ��
#define NFCTargetLevel          0x18                        //Ŀ�꿨ƬRSSI�ȼ��Ĵ���
#define NFCTargetProtocol       0x19                        //Ŀ�꿨ƬЭ��Ĵ���
#define TestSetting1            0x1A                        //�������üĴ���1
#define TestSetting2            0x1B                        //�������üĴ���2
#define FIFOStatus              0x1C                        //FIFO״̬�Ĵ���
#define TXLenghtByte1           0x1D                        //TX���ͳ����ֽ�1�Ĵ���
#define TXLenghtByte2           0x1E                        //TX���ͳ����ֽ�2�Ĵ���
#define FIFO                    0x1F                        //FIFO���ݼĴ���

/* TRF7960оƬ����� */
/*=============================================================================*/
#define Idle                    0x00                        //��������
#define SoftInit                0x03                        //�����ʼ������
#define InitialRFCollision      0x04                        //��ʼ��RF��ײ����
#define ResponseRFCollisionN    0x05                        //Ӧ��RF��ײN����
#define ResponseRFCollision0    0x06                        //Ӧ��RF��ײ0����
#define Reset                   0x0F                        //��λ����
#define TransmitNoCRC           0x10                        //������CRCУ������
#define TransmitCRC             0x11                        //���ʹ�CRCУ������
#define DelayTransmitNoCRC  	0x12                        //��ʱ������CRCУ������
#define DelayTransmitCRC    	0x13                        //��ʱ���ʹ�CRCУ������
#define TransmitNextSlot        0x14                        //������һ��������
#define CloseSlotSequence       0x15                        //�رղ���������
#define StopDecoders            0x16                        //ֹͣ��������
#define RunDecoders             0x17                        //���н�������
#define ChectInternalRF         0x18                        //����ڲ�RF��������
#define CheckExternalRF         0x19                        //����ⲿRF��������
#define AdjustGain              0x1A                        //�����������
/*=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif


