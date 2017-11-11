#include "hal_defs.h"
#include "hal_cc8051.h"
#include "hal_int.h"
#include "hal_mcu.h"
#include "hal_board.h"
#include "hal_led.h"
#include "hal_rf.h"
#include "basic_rf.h"
#include "hal_uart.h" 
#include "UART_PRINT.h" 
#include "TIMER.h"
#include "get_adc.h"
#include "sh10.h"
#include "get_swsensor.h"
#include <string.h>

#define MAX_SEND_BUF_LEN  128  
#define MAX_RECV_BUF_LEN  128
static uint8 pTxData[MAX_SEND_BUF_LEN]; //�������߷��ͻ������Ĵ�С
static uint8 pRxData[MAX_RECV_BUF_LEN]; //�������߽��ջ������Ĵ�С

#define MAX_UART_SEND_BUF_LEN  128
#define MAX_UART_RECV_BUF_LEN  128
uint8 uTxData[MAX_UART_SEND_BUF_LEN]; //���崮�ڷ��ͻ������Ĵ�С
uint8 uRxData[MAX_UART_RECV_BUF_LEN]; //���崮�ڽ��ջ������Ĵ�С
uint16 uTxlen = 0;
uint16 uRxlen = 0;


/*****��Ե�ͨѶ��ַ����******/
#define RF_CHANNEL                20           // Ƶ�� 11~26
#define PAN_ID                    0x1379      //����id 
#define MY_ADDR                   0xacef      // ����ģ���ַ
#define SEND_ADDR                 0x1234     //���͵�ַ
/**************************************************/
static basicRfCfg_t basicRfConfig;
uint8   APP_SEND_DATA_FLAG;
/******************************************/

/**************************************************/
// ����RF��ʼ��
void ConfigRf_Init(void)
{
    basicRfConfig.panId       =   PAN_ID;        //zigbee��ID������
    basicRfConfig.channel     =   RF_CHANNEL;    //zigbee��Ƶ������
    basicRfConfig.myAddr      =  MY_ADDR;   //���ñ�����ַ
    basicRfConfig.ackRequest  =   TRUE;          //Ӧ���ź�
    while(basicRfInit(&basicRfConfig) == FAILED); //���zigbee�Ĳ����Ƿ����óɹ�
    basicRfReceiveOn();                // ��RF
}

/********************MAIN************************/
void main(void)    
{
    uint16 sensor_val,sensor_tem;
    uint16  len = 0;
    halBoardInit();  //ģ�������Դ�ĳ�ʼ��
    ConfigRf_Init(); //�����շ����������ó�ʼ�� 
    halLedSet(1);
    halLedSet(2);  
    Timer4_Init(); //��ʱ����ʼ��
    Timer4_On();  //�򿪶�ʱ��
    while(1)
    {
        APP_SEND_DATA_FLAG = GetSendDataFlag();           
        if(APP_SEND_DATA_FLAG == 1)   //��ʱʱ�䵽
        {   /*���������ɼ������� ��ʼ*/
#if defined (GD_SENDOR)  //���մ�����
            sensor_val=get_adc();    //ȡģ���ѹ
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"���մ�������ѹ��%d.%02dV\r\n",sensor_val/100,sensor_val%100);
#endif  
#if defined (CO_SENDOR)  //һ����̼������
            sensor_val=get_adc();    //ȡģ���ѹ
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"һ����̼��������ѹ��%d.%02dV\r\n",sensor_val/100,sensor_val%100);
#endif  
#if defined (KRQ_SENDOR)  //��ȼ��������
            sensor_val=get_adc();    //ȡģ���ѹ
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"��ȼ����������ѹ��%d.%02dV\r\n",sensor_val/100,sensor_val%100);
#endif  
#if defined (FIR_SENDOR)  //���洫����
            sensor_val=get_adc();    //ȡģ���ѹ
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"���洫������ѹ��%d.%02dV\r\n",sensor_val/100,sensor_val%100);
#endif            
#if defined (RT_SENDOR)  //���崫����
            sensor_val=get_swsensor();    //ȡ������
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"���崫������ƽ��%d\r\n",sensor_val);
#endif      
#if defined (TEM_SENDOR)  //��ʪ�ȴ�����
            call_sht11(&sensor_tem,&sensor_val);    //ȡ��ʪ������
            //�Ѳɼ����ݴ������ַ������Ա����ڴ�������ʾ�۲�
            printf_str(pTxData,"��ʪ�ȴ��������¶ȣ�%d.%d, ʪ�ȣ�%d.%d\r\n",
                       sensor_tem/10,sensor_tem%10,sensor_val/10,sensor_val%10);
#endif            
            halLedToggle(3);       // �̵�ȡ�������߷���ָʾ
            //������ͨ��zigbee���ͳ�ȥ
            basicRfSendPacket(SEND_ADDR, pTxData,strlen(pTxData ));
            //����Ҳ�������ش��ڣ����ڵ���
            halUartWrite(pTxData,strlen(pTxData));
            Timer4_On();  //�򿪶�ʱ
         }  /*���������ɼ������� ����*/           
    }
}
/************************main end ****************************/
