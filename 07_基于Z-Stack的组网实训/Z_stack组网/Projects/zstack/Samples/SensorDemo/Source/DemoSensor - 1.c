/*******************************************************************************
 * �ļ����ƣ�DemoSensor.c
 * ��    �ܣ�SensorApp ���� ���ߴ����������ʾʵ�顣
 * ʵ�����ݣ����ؽڵ�ΪЭ������������󣬴������ڵ㷢�ָ����粢���뵽�������У�
 *           ���ҽ��ɼ�ת���õ����¶�ֵ��ʪ��ֵ����ѹֵ�ȴ��͸��ɼ��豸��
 *           �������豸ת��������PC��λ�����WSN Monitor����GUI��ʽ��ʾ��
 * ʵ���豸��SK-SmartRF05BB(װ����SK-CC2530EM)
 *           SK-SmartRF05BB-DOWN����    �򸸽ڵ㷢�ʹ���������
 *
 *           D1(��ɫ)��                 ��ɼ��ڵ㷢������ʱ��˸
 *           D3(��ɫ)��                 �ɹ�����Э�����������������˸  
 *    
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "sapi.h"
#include "hal_key.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_mcu.h"
#include "hal_uart.h"
#include "DemoApp.h"
/********************************************************************/


#define REPORT_FAILURE_LIMIT                4
#define ACK_REQ_INTERVAL                    5 // each 5th packet is sent with ACK request

// General UART frame offsets
#define FRAME_SOF_OFFSET                    0
#define FRAME_LENGTH_OFFSET                 1 
#define FRAME_CMD0_OFFSET                   2
#define FRAME_CMD1_OFFSET                   3
#define FRAME_DATA_OFFSET                   4

// ZB_RECEIVE_DATA_INDICATION offsets
#define ZB_RECV_SRC_OFFSET                  0
#define ZB_RECV_CMD_OFFSET                  2
#define ZB_RECV_LEN_OFFSET                  4
#define ZB_RECV_DATA_OFFSET                 6
#define ZB_RECV_FCS_OFFSET                  8

// ZB_RECEIVE_DATA_INDICATION frame length
#define ZB_RECV_LENGTH                      15

// PING response frame length and offset
#define SYS_PING_RSP_LENGTH                 7 
#define SYS_PING_CMD_OFFSET                 1

// Stack Profile
#define ZIGBEE_2007                         0x0040
#define ZIGBEE_PRO_2007                     0x0041

#ifdef ZIGBEEPRO
#define STACK_PROFILE                       ZIGBEE_PRO_2007             
#else 
#define STACK_PROFILE                       ZIGBEE_2007
#endif

#define CPT_SOP                             0xFE
#define SYS_PING_REQUEST                    0x0021
#define SYS_PING_RESPONSE                   0x0161
#define ZB_RECEIVE_DATA_INDICATION          0x8746

// Application States
#define APP_INIT                            0    // Initial state
#define APP_START                           1    // Sensor has joined network
#define APP_BIND                            2    // Sensor is in process of binding
#define APP_REPORT                          4    // Sensor is in reporting state

// Application osal event identifiers
// Bit mask of events ( from 0x0000 to 0x00FF )
#define MY_START_EVT                        0x0001
#define MY_REPORT_EVT                       0x0002
#define MY_FIND_COLLECTOR_EVT               0x0004
#define MY_SEND_EVT               0x0010

// ADC definitions for CC2430/CC2530 from the hal_adc.c file

#define HAL_ADC_REF_125V    0x00    /* Internal 1.25V Reference */
#define HAL_ADC_DEC_064     0x00    /* Decimate by 64 : 8-bit resolution */
#define HAL_ADC_DEC_128     0x10    /* Decimate by 128 : 10-bit resolution */
#define HAL_ADC_DEC_512     0x30    /* Decimate by 512 : 14-bit resolution */
#define HAL_ADC_CHN_VDD3    0x0f    /* Input channel: VDD/3 */
#define HAL_ADC_CHN_TEMP    0x0e    /* Temperature sensor */

/* �궨�� */
/********************************************************************/

/* ���ر��� */
/********************************************************************/
static uint8 appState =           APP_INIT;
static uint8 reportState =        FALSE;
static uint8 reportFailureNr =    0;
static uint16 myReportPeriod =    2000;         // ����
static uint16 myBindRetryDelay =  2000;         // ����
static uint16 parentShortAddr;
/********************************************************************/

/*
  �������豸�����������
 */
#define NUM_OUT_CMD_SENSOR                2
#define NUM_IN_CMD_SENSOR                 0

/* 
  �������豸��������б� 
 */
const cId_t zb_OutCmdList[NUM_OUT_CMD_SENSOR] =
{
  SENSOR_REPORT_CMD_ID,
  DUMMY_REPORT_CMD_ID
};

/* 
  �������豸�ļ������� 
 */
const SimpleDescriptionFormat_t zb_SimpleDesc =
{
  MY_ENDPOINT_ID,             //  �˵��
  MY_PROFILE_ID,              //  Profile ID
  DEV_ID_SENSOR,              //  �豸ID
  DEVICE_VERSION_SENSOR,      //  �豸�汾
  0,                          //  Ԥ��
  NUM_IN_CMD_SENSOR,          //  ������������
  (cId_t *) NULL,             //  ���������б�
  NUM_OUT_CMD_SENSOR,         //  �����������
  (cId_t *) zb_OutCmdList     //  ��������б�
};
/******************************************************************************/


/* ���غ��� */
/********************************************************************/
void uartRxCB( uint8 port, uint8 event );
static void sendReport(void);
static uint8 readTemp(void);
static uint8 readVoltage(void);
/********************************************************************/



/*********************************************************************
 * �������ƣ�zb_HandleOsalEvent
 * ��    �ܣ�����ϵͳ�¼���������
 * ��ڲ�����event  ��OSAL������¼�event����λ��������˱����õ��¼���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_HandleOsalEvent( uint16 event )
{
  if( event & ZB_ENTRY_EVENT )
  { 
    /* ��˸LED1 ��ָʾ����/����һ������ */
    HalLedBlink ( HAL_LED_1, 0, 50, 250 );
     
    /* �����豸 */  
    zb_StartRequest();
  }
  
  if ( event & MY_REPORT_EVT )
  {
    if ( appState == APP_REPORT ) 
    {
      HAL_TOGGLE_LED4();
      sendReport(); // ���ͱ���
      osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT, myReportPeriod );
    }
  }
  
  if ( event & MY_FIND_COLLECTOR_EVT )
  {
    /* ɾ��ԭ���İ� */
    if ( appState==APP_REPORT ) 
    {
      zb_BindDevice( FALSE, SENSOR_REPORT_CMD_ID, (uint8 *)NULL );
    }
    
    appState = APP_BIND;
    
    // ��˸LED2(��ɫ)��ָʾ���ֺͰ�
    HalLedBlink ( HAL_LED_2, 0, 50, 500 );
    
    /* ���Ҳ��󶨵�һ���ɼ��豸 */
    zb_BindDevice( TRUE, SENSOR_REPORT_CMD_ID, (uint8 *)NULL );
    //zb_BindDevice( TRUE, DUMMY_REPORT_CMD_ID, (uint8 *)NULL );   // ****************
   osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT,myReportPeriod+1000 );
    reportState = TRUE;
  }
}


/*********************************************************************
 * �������ƣ�zb_HandleKeys
 * ��    �ܣ������¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_3     SK-SmartRF05BB��DOWN��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_3 )
  {
    /* ��ʼ���� */
    osal_set_event( sapi_TaskID, MY_REPORT_EVT );
    reportState = TRUE;
  } 
}


/*********************************************************************
 * �������ƣ�zb_StartConfirm
 * ��    �ܣ�����ȷ�Ϻ������������ڿ���һ���豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����status  ����������״ֵ̬�����ֵΪZB_SUCCESS��ָʾ����
 *           �豸�����ɹ���ɡ�������ǣ�����ʱmyStartRetryDelay��
 *           �����ϵͳ����MY_START_EVT�¼���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_StartConfirm( uint8 status )
{
  /* ����豸�ɹ��������ı�Ӧ��״̬ */
  if ( status == ZB_SUCCESS ) 
  {
    appState = APP_START;
    
    /* ����LED1��ָʾ�ڵ��Ѿ����� */
    HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
    
    /* �洢���ڵ�̵�ַ */
    zb_GetDeviceInfo(ZB_INFO_PARENT_SHORT_ADDR, &parentShortAddr);
    
    /* �����¼��󶨵��ɼ��ڵ� */
    osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );  
  }
}


/*********************************************************************
 * �������ƣ�zb_SendDataConfirm
 * ��    �ܣ���������ȷ�Ϻ������������ڷ������ݲ��������ZigBeeЭ��ջ
 *           �����á�
 * ��ڲ�����handle  ���ݴ���ʶ������
 *           status  ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_SendDataConfirm( uint8 handle, uint8 status )
{
  if(status != ZB_SUCCESS) 
  {
    /* ����������ݲ��ɹ��������ӷ���ʧ�ܼ������������趨ֵ�Ƚ� */
    if ( ++reportFailureNr >= REPORT_FAILURE_LIMIT ) 
    {
       /* ֹͣ�����¼� */
       osal_stop_timerEx( sapi_TaskID, MY_REPORT_EVT );
       
       /* ʧ�ܱ�����豸�󶨵��µĸ��ڵ��Ͻ��Զ�����*/
       reportState=TRUE; // ���������Ϊ�����ư������������¼�
        
       /* ���԰󶨵��µĸ��ڵ� */
       osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );
       reportFailureNr=0;
    }
  }
  else // �����ɹ� status == SUCCESS
  {
    /* ����ʧ�ܼ����� */
    reportFailureNr=0;
  }
}


/*********************************************************************
 * �������ƣ�zb_BindConfirm
 * ��    �ܣ���ȷ�Ϻ������������ڰ󶨲��������ZigBeeЭ��ջ�����á�
 * ��ڲ�����commandId  ��ȷ���������ID��
 *           status     ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_BindConfirm( uint16 commandId, uint8 status )
{
  if( status == ZB_SUCCESS )
  {   
    appState = APP_REPORT; // �󶨳ɹ�
    HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
    
    if ( reportState ) 
    {
      /* ��ʼ�����¼� */
      osal_set_event( sapi_TaskID, MY_REPORT_EVT );
    }
  }
  else  // ����󶨲��ɹ��������Ѱ�Ҳɼ��ڵ�
  {
    osal_start_timerEx( sapi_TaskID, MY_FIND_COLLECTOR_EVT, myBindRetryDelay );
  }
}


/*********************************************************************
 * �������ƣ�zb_AllowBindConfirm
 * ��    �ܣ������ȷ�Ϻ�����ָʾ����һ���豸��ͼ�󶨵����豸��
 * ��ڲ�����source   ��Դ��ַ�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_AllowBindConfirm( uint16 source )
{
}


/*********************************************************************
 * �������ƣ�zb_FindDeviceConfirm
 * ��    �ܣ������豸ȷ�Ϻ������������ڲ����豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����searchType  �������͡� 
 *           searchKey   ����ֵ
 *           result      ���ҽ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result )
{
}


/*********************************************************************
 * �������ƣ�zb_ReceiveDataIndication
 * ��    �ܣ��������ݴ���������������ZigBeeЭ��ջ���ã�����֪ͨӦ��
 *           ���������Ѿ��Ӹ����豸�н��յ���
 *           Э��ջ�����á�
 * ��ڲ�����source   ����Դ��ַ�� 
 *           command  ����ֵ��
 *           len      ���ݳ��ȡ�
 *           pData    �豸���͹��������ݴ���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{
}


/*********************************************************************
 * �������ƣ�sendReport
 * ��    �ܣ����ͱ��溯����
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
#define SENSOR_TEMP_OFFSET                0
#define SENSOR_VOLTAGE_OFFSET             1
#define SENSOR_PARENT_OFFSET              2
#define SENSOR_REPORT_LENGTH              4

static void sendReport(void)
{
  uint8 pData[SENSOR_REPORT_LENGTH];
  static uint8 reportNr=0;
  uint8 txOptions;
  
  /* ��ȡ�������¶�ֵ */
  pData[SENSOR_TEMP_OFFSET] =  readTemp();
  
  /* ��ȡ�������ѹֵ */
  pData[SENSOR_VOLTAGE_OFFSET] = readVoltage(); 
    
  /* ��ȡ���ڵ�̵�ַ */
  pData[SENSOR_PARENT_OFFSET] =  HI_UINT16(parentShortAddr);
  pData[SENSOR_PARENT_OFFSET + 1] =  LO_UINT16(parentShortAddr);
  
  /* ��ÿ��ACK_INTERVAL�������ACK���� */
  /* ������淢��ʧ�ܣ���ô�ڽ��¸������з���ACK���� */
  if ( ++reportNr<ACK_REQ_INTERVAL && reportFailureNr==0 ) 
  {
    txOptions = AF_TX_OPTIONS_NONE;
  }
  else 
  {
    txOptions = AF_MSG_ACK_REQUEST;
    reportNr = 0;
  }

  /* ��ʵ����ڰ󶨷�ʽ��������,Ŀ�ĵ�ַΪ 0xFFFE */
  /* Ϊ����ID������ */
  
  zb_SendDataRequest( 0xFFFE, SENSOR_REPORT_CMD_ID, SENSOR_REPORT_LENGTH, pData, 0, txOptions, 0 );
  //zb_SendDataRequest( 0xFFFE, DUMMY_REPORT_CMD_ID, SENSOR_REPORT_LENGTH, pData, 0, txOptions, 0 );  //********
}


/* �궨�� */
/********************************************************************/
/*
   CC253xƬ���¶ȴ������¶Ȼ���ϵ����
   ��ҪУ׼�����Ǻ�׼ȷ
 */
#define ADCVALUE_AT_TEMP_ZERO     1024     // ������ȶ�Ӧ��ADCֵ
#define TEMP_COEFFICIENT          20       // ϵ��
/********************************************************************/
/*********************************************************************
 * �������ƣ�readTemp
 * ��    �ܣ���ȡADC�����¶�ֵ������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ��(tt / TEMP_COEFFICIENT)  �¶�ֵ
 ********************************************************************/
static uint8 readTemp(void)
{
  volatile unsigned char tmp,n;
  signed short adcvalue;
  float tt;

  /*
     ϵͳ�ϵ縴λ�󣬵�һ�δ�ADC��ȡ��ת��ֵ�ܱ���Ϊ��GND��ƽ������ִ������Ĵ�
     �����ƹ����BUG��
   */
/************************************************************/
  /* ��ȡADCL��ADCH������ADCCON1.EOC*/
  tmp = ADCL;     
  tmp = ADCH;
  /* �������ε��β������ƹ�BUG */
  for(n=0;n<2;n++)
  {  
    /* ���û�׼��ѹ����ȡ�ʺ͵�������ͨ�� */
    ADCCON3 = ((0x02 << 6) |  // ����AVDD5�����ϵĵ�ѹΪ��׼��ѹ
               (0x00 << 4) |  // ��ȡ��Ϊ64����Ӧ����ЧλΪ7λ(���λΪ����λ)
               (0x0C << 0));  // ѡ�񵥶�����ͨ��12(GND)
  
    /* �ȴ�ת����� */
    while ((ADCCON1 & 0x80) != 0x80);

    /* ��ȡADCL��ADCH������ADCCON1.EOC */
    tmp = ADCL;     
    tmp = ADCH;  
  }
/************************************************************/

  /* ���Ӳ�ʹ��Ƭ���¶ȴ����� */
  TR0 |= 0x01;    // ����Ƭ���¶ȴ����������Ƭ��ADC������ͨ��14
  ATEST |= 0x01;  // ʹ��Ƭ���¶ȴ�����
  ADCIF = 0;      // ��ADC�жϱ�־λ
  
  /* ���û�׼��ѹ����ȡ�ʺ͵�������ͨ�� */
  ADCCON3 = ((0x00 << 6) |  // �����ڲ���׼��ѹ1.15V
             (0x03 << 4) |  // ��ȡ��Ϊ512����Ӧ����ЧλΪ12λ(���λΪ����λ)
             (0x0E << 0));  // ѡ�񵥶�����ͨ��14(Ƭ���¶ȴ��������)

  /* �ȴ�ת����� */
  while ((ADCCON1 & 0x80) != 0x80);

  /* ��ADCL��ADCH��ȡת��ֵ���˲���������ADCCON1.EOC */
  adcvalue = (signed short)ADCL;
  adcvalue |= (signed short)(ADCH << 8); 

  /* ��adcvalueС��0������Ϊ��Ϊ0 */
  if(adcvalue < 0) 
    adcvalue = 0;
    
  adcvalue >>= 4;  // ȡ��12λ��Чλ
    
  /* ��������¶���ʾΪ0���϶� */
  if(adcvalue < ADCVALUE_AT_TEMP_ZERO)
    tt = 0;
    
  tt = adcvalue - ADCVALUE_AT_TEMP_ZERO;
    
  /* ��������¶���ʾΪ99���϶� */
  if(tt > (TEMP_COEFFICIENT * 99))
    tt = TEMP_COEFFICIENT * 99;
       
  ATEST = 0x00;  // ��ֹƬ���¶ȴ�����
  TR0 = 0x00;    // ���ص���������״̬
  
  /* ��������¶�ֵ */
  return (uint8)(tt / TEMP_COEFFICIENT);   // �����¶�ֵ
}


/*********************************************************************
 * �������ƣ�readVoltage
 * ��    �ܣ���ȡADC���е�ѹֵ������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ��voltagevalue  ��ѹֵ
 ********************************************************************/
static uint8 readVoltage(void)
{
  volatile unsigned char tmp,n;
  signed short adcvalue;
  float voltagevalue;

  /*
     ϵͳ�ϵ縴λ�󣬵�һ�δ�ADC��ȡ��ת��ֵ�ܱ���Ϊ��GND��ƽ��ִ��
     ����Ĵ������ƹ����BUG��
   */
/************************************************************/
  /* ��ȡADCL��ADCH������ADCCON1.EOC*/
  tmp = ADCL;     
  tmp = ADCH;
  /* �������ε��β������ƹ�BUG */
  for(n=0;n<2;n++)
  {  
    /* ���û�׼��ѹ����ȡ�ʺ͵�������ͨ�� */
    ADCCON3 = ((0x02 << 6) |  // ����AVDD5�����ϵĵ�ѹΪ��׼��ѹ
               (0x00 << 4) |  // ��ȡ��Ϊ64����Ӧ����ЧλΪ7λ(���λΪ����λ)
               (0x0C << 0));  // ѡ�񵥶�����ͨ��12(GND)
  
    /* �ȴ�ת����� */
    while ((ADCCON1 & 0x80) != 0x80);

    /* ��ȡADCL��ADCH������ADCCON1.EOC */
    tmp = ADCL;     
    tmp = ADCH;  
  }
/************************************************************/

  /* ���û�׼��ѹ����ȡ�ʺ͵�������ͨ�� */
  ADCCON3 = ((0x00 << 6) |  // �����ڲ���׼��ѹ1.15V
             (0x03 << 4) |  // ��ȡ��Ϊ512����Ӧ����ЧλΪ12λ(���λΪ����λ)
             (0x0F << 0));  // ѡ�񵥶�����ͨ��15(VDD/3)
  
  /* �ȴ�ת����� */
  while ((ADCCON1 & 0x80) != 0x80);

  /* ��ADCL��ADCH��ȡת��ֵ���˲���������ADCCON1.EOC */
  adcvalue = (signed short)ADCL;
  adcvalue |= (signed short)(ADCH << 8); 

  /* ��adcvalueС��0������Ϊ��Ϊ0 */
  if(adcvalue < 0) 
    adcvalue = 0;
    
  adcvalue >>= 4;  // ȡ��12λ��Чλ
    
  /* ��ת��ֵ����Ϊʵ�ʵ�ѹֵ */
  voltagevalue = ((adcvalue * 1.15) / 2047);  // 2047��ģ������ﵽVREFʱ��������ֵ
                                              // ������Чλ��12λ(���λΪ����λ)��
                                              // ��������������ֵΪ2047
                                              // �˴���VREF = 1.15V(�ڲ���׼��ѹ)

  /* ת��Ϊʵ�ʹ����ѹ */
  voltagevalue *= 3; 
  voltagevalue *= 10;  // ��ѹֵ�Ŵ�10����������λ���������

  return (uint8)voltagevalue;
}


/******************************************************************************
 * @fn          uartRxCB
 *
 * @brief       Callback function for UART 
 *
 * @param       port - UART port
 *              event - UART event that caused callback 
 *
 * @return      none
 */
void uartRxCB( uint8 port, uint8 event )
{
  uint8 pBuf[RX_BUF_LEN];
  uint16 cmd;
  uint16 len;
  //EA=0;
  if ( event != HAL_UART_TX_EMPTY ) 
  {
  
    // Read from UART
    len = HalUARTRead( HAL_UART_PORT_0, pBuf, RX_BUF_LEN );
    
    if ( len>0 ) 
    {
      
       //HalUARTWrite(HAL_UART_PORT_0, pBuf, len);
      
      cmd = BUILD_UINT16(pBuf[SYS_PING_CMD_OFFSET+ 1], pBuf[SYS_PING_CMD_OFFSET]);
  
      if( (pBuf[FRAME_SOF_OFFSET] == CPT_SOP) && (cmd == SYS_PING_REQUEST) ) 
      {
        //sysPingReqRcvd();
      }
      // ��������
      configset(pBuf,len, LOG_TYPE); 
      
    }
  }
  // EA=1;
}
