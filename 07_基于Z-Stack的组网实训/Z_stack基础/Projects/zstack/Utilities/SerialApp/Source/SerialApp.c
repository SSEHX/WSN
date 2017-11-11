/*******************************************************************************
 * �ļ����ƣ�SerialApp.c
 * ��    �ܣ�SerialAppʵ�顣
 * ʵ�����ݣ���ʵ����Ҫ���û���ʾ����ZigBee����������͸������ϵͳ����ZigBee����
 *           ����ģ��������������ߡ�PCͨ��RS232���нӿڷ������ݸ�ZigBee������
 *           ��ģ�飬ģ��������ݺ�ͨ����Ƶ�շ��������ݷ��͸���һ�˵�ZigBee����
 *           ����ģ�飬��ģ��ͨ����Ƶ�շ����������ݺ�ֱ�ӷ��͵�RS232���нӿڹ�PC
 *           �����ա�
 * ʵ���豸��SK-SmartRF05EB��װ����SK-CC2530EM��
 *           LEFT����     ���͡�ƥ������������
 *           RIGHT����    ���͡��ն��豸������         
 *           LED_Y(��ɫ)�����ɹ�������������������LED����
 *           LED_B(��ɫ)��������һ�豸�ɹ�����Ӧ�ò���߼����Ӻ��LED����
 *           
 *           ���Ƕ������豸��Ӧ�ò㽨���߼�������������˵��������A��B�������е�
 *           �����豸������ϣ�������ǵ�Ӧ�ò㽨���߼����ӣ����Բ����������ַ�����
 *           ��1����ƥ������������
 *           �����豸A�ϵ�LEFT������ʼ��Ӧ�ò㽨���߼����ӣ�����ɹ����豸A�ϵ�
 *           LED_B(��ɫ)��������ʱ���Ǿͽ����˵����A->B���߼����ӡ��˺��豸A
 *           �������豸B������Ϣ��
 *           �����豸B�ϵ�LEFT������ʼ��Ӧ�ò㽨���߼����ӣ�����ɹ����豸B�ϵ�
 *           LED_B(��ɫ)��������ʱ���Ǿͽ����˵����B->A���߼����ӡ��˺��豸B
 *           �������豸A������Ϣ��
 *���������� ����ǰ�������Ĳ������豸A���豸B֮��ͽ�������Ӧ�ò���߼����ӣ�֮
 *           �����ǿ��Ի�����Ϣ��
 *           ��2�����ն��豸������
 *            ��APS_DEFAULT_MAXBINDING_TIME�涨ʱ���ڣ�Ĭ��Ϊ16�룩�ֱ����豸
 *            A���豸B�ϵ�RIGHT�����������豸A���豸B֮��İ󶨣����ɹ����豸A��
 *            �豸B�ϵ�LED_B(��ɫ)��������ʱ�豸A���豸B֮��ͽ�������Ӧ�ò����
 *            �����ӣ�֮�����ǿ��Ի�����Ϣ����û�гɹ������糬���涨ʱ�䣩������
 *            ���ϵ�LED_B(��ɫ)��˸����ʾ��û�гɹ���
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "AF.h"
#include "OnBoard.h"
#include "OSAL_Tasks.h"
#include "SerialApp.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "hal_drivers.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
  #include "hal_lcd.h"
#endif
#include "hal_led.h"
#include "hal_uart.h"
/********************************************************************/


//#define SERIAL_APP_LOOPBACK  TRUE  // ��ʹ�ô��ڻ��ز�����ȥ�����п�ͷ��"//"


/* �� */
/* 
   ����������
   �������Ķ�������������otaBuf����ʱ����Ҫע�ͣ���otaBuf�����Ĳ�����
   ��ǿ��ʹ�øú����ֱ��ʹ��osal_mem_free()������
 */
/********************************************************************/
#define FREE_OTABUF() { \
  if ( otaBuf ) \
  { \
    osal_mem_free( otaBuf ); \
  } \
  if ( otaBuf2 ) \
  { \
    SerialApp_SendData( otaBuf2, otaLen2 ); \
    otaBuf2 = NULL; \
  } \
  else \
  { \
    otaBuf = NULL; \
  } \
}
/********************************************************************/



/* ���� */
/********************************************************************/
/*
   ���ں�
   ���û�û�ж���SERIAL_APP_PORT���Ĭ�϶���SERIAL_APP_PORT��ֵΪ0��
 */
#if !defined( SERIAL_APP_PORT )
  #define SERIAL_APP_PORT  0
#endif


/*
   ����ͨ�Ų�����
   ���û�û�ж���SERIAL_APP_BAUD���Ĭ�϶���SERIAL_APP_BAUDΪHAL_UART_BR_38400��
   ��Ĭ��ʹ�õĲ�����Ϊ38400bps������CC2430,ֻ֧��38400bps��115200bps��
 */
#if !defined( SERIAL_APP_BAUD )
  #define SERIAL_APP_BAUD  HAL_UART_BR_38400
  //#define SERIAL_APP_BAUD  HAL_UART_BR_115200
#endif


/*
   ���ջ�������ֵ
   ���û�û�ж���SERIAL_APP_THRESH����Ĭ�϶���SERIAL_APP_THRESH��ֵΪ48��
   �����ջ������ռ����ڸ���ֵʱ�����ô��ڽ��ջص�����rxCB()��
 */
#if !defined( SERIAL_APP_THRESH )
  #define SERIAL_APP_THRESH  48
#endif


/*
   ����ÿһ�������Խ��յ������������ֽ�Ϊ��λ��
   ���û�û�ж���SERIAL_APP_RX_MAX����ʹ��DMA��ʽʱĬ�϶���SERIAL_APP_RX_MAX��
   ֵΪ128����ʹ��DMA��ʽʱĬ�϶����ֵΪ64��
 */
#if !defined( SERIAL_APP_RX_MAX )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
    #define SERIAL_APP_RX_MAX  128
  #else
    #define SERIAL_APP_RX_MAX  64
  #endif
#endif


/*
   ����ÿһ�������Է��͵������������ֽ�Ϊ��λ��
   ���û�û�ж���SERIAL_APP_TX_MAX����ʹ��DMA��ʽʱĬ�϶���SERIAL_APP_TX_MAX��
   ֵΪ128����ʹ��DMA��ʽʱĬ�϶����ֵΪ64��
 */
#if !defined( SERIAL_APP_TX_MAX )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
  #define SERIAL_APP_TX_MAX  128
  #else
    #define SERIAL_APP_TX_MAX  64
  #endif
#endif


/*
   һ���ֽڱ����յ����ڵ���rxCB()�ص�����֮ǰ�Ŀ���ʱ�䣨�Ժ���Ϊ��λ��
   ���û�û�ж���SERIAL_APP_IDLE����Ĭ�϶���SERIAL_APP_IDLE��ֵΪ6��
 */
#if !defined( SERIAL_APP_IDLE )
  #define SERIAL_APP_IDLE  6
#endif


/*
   ÿ��OTA��Ϣ�������շ�����Ϣ����Ӧ�����ݲ��ֵ���������������ֽ�Ϊ��λ��
   ���û�û�ж���SERIAL_APP_RX_CNT����ʹ��DMA��ʽʱĬ�϶���SERIAL_APP_RX_CNT��
   ֵΪ80����ʹ��DMA��ʽʱĬ�϶����ֵΪ6��
 */
#if !defined( SERIAL_APP_RX_CNT )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
    #define SERIAL_APP_RX_CNT  80
  #else
    #define SERIAL_APP_RX_CNT  6
  #endif
#endif


/*
   ���ز���ʹ��
   ���ز�����ָ�û�PCͨ�����ڷ����ݸ�CC2430��CC2430�յ���ͨ�����ڽ������ٴ��ظ�
   �û�PC�����ز�������������Ӧ���޹أ�������Ϊ�����û�PC��CC2430���д���ͨ����
   ��ɹ��ļ�鹤�ߡ�
   ���û�û�ж���SERIAL_APP_LOOPBACK����Ĭ�϶���SERIAL_APP_LOOPBACKΪFALSE������
   ʹ�ܻ��ز��ԡ�
 */
#if !defined( SERIAL_APP_LOOPBACK )
  #define SERIAL_APP_LOOPBACK  FALSE
#endif


/*
   ���ز�����ʹ�õ��¼�����ʱʱ��
   ��ʹ���˻��ز��ԣ����廷�ز�����ʹ�õ��¼�����ʱʱ��
 */
#if SERIAL_APP_LOOPBACK
  #define SERIALAPP_TX_RTRY_EVT      0x0010  // �����ط����¼�
  #define SERIALAPP_TX_RTRY_TIMEOUT  250     // �����ط�����ʱʱ��
#endif


/*
   ��Ӧ��Ϣ��������
   ���û�û�ж���SERIAL_APP_RSP_CNT����Ĭ�϶���SERIAL_APP_RSP_CNT��ֵΪ4��
 */
#define SERIAL_APP_RSP_CNT  4


/* 
   ��ID�б� 
  ���б�Ӧ���ñ�Ӧ��ר�õĴ�ID�����
*/

const cId_t SerialApp_ClusterList[SERIALAPP_MAX_CLUSTERS] =
{
  SERIALAPP_CLUSTERID1,  // ����Ӧ������
  SERIALAPP_CLUSTERID2   // ������Ӧ��Ϣ
};


/* 
   �豸�ļ������� 
 */
const SimpleDescriptionFormat_t SerialApp_SimpleDesc =
{
  SERIALAPP_ENDPOINT,              //  �˵��
  SERIALAPP_PROFID,                //  Profile ID
  SERIALAPP_DEVICEID,              //  �豸ID
  SERIALAPP_DEVICE_VERSION,        //  �豸�汾
  SERIALAPP_FLAGS,                 //  ��ʶ
  SERIALAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)SerialApp_ClusterList,  //  ������б�
  SERIALAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)SerialApp_ClusterList   //  ������б�
};


/* 
   �˵������� 
*/
const endPointDesc_t SerialApp_epDesc =
{
  SERIALAPP_ENDPOINT,
 &SerialApp_TaskID,
  (SimpleDescriptionFormat_t *)&SerialApp_SimpleDesc,
  noLatencyReqs
};
/********************************************************************/



/* ȫ�ֱ��� */
/********************************************************************/
/*
   ��Ӧ�õ�����ID��������SerialApp_Init()����������ʱ��   �ñ�������
   �������IDֵ��
*/
uint8 SerialApp_TaskID;    

/*
   �������
 */
static uint8 SerialApp_MsgID;

/*
   ������Ϣ��Ŀ�ĵ�ַ
 */
static afAddrType_t SerialApp_DstAddr;

/*
   �������
 */
static uint8 SerialApp_SeqTx;

/*
   ��������ָ�򻺳�����ָ��otaBuf��otaBuf2
   ������ָ����ָ��Ļ�������Ҫ��OSAL��̬���䡣���ڱ�Ӧ�ã��ͷ�������ָ����ָ��
   �Ļ������ռ䲻Ҫʹ��osal_mem_free()������Ӧ��ʹ�ñ��ļ���ʼ������ĺ�
   FREE_OTABUF()
 */
static uint8 *otaBuf;
static uint8 *otaBuf2;
static uint8 otaLen;
static uint8 otaLen2;

/*
   �ش�����
 */
static uint8 rtryCnt;

/*
   �������
 */
static uint8 SerialApp_SeqRx;

/*
   ��Ӧ��Ϣ��Ŀ�ĵ�ַ
 */
static afAddrType_t SerialApp_RspDstAddr;

/*
   �����Ӧ��Ϣ�Ļ�����
 */
static uint8 rspBuf[ SERIAL_APP_RSP_CNT ];

/*
   ���ز�����ʹ�õĻ�����������
   ��ʹ���˻��ز��ԣ����廷�ز�����ʹ�õĻ�����������
 */
#if SERIAL_APP_LOOPBACK
static uint8 rxLen;  // ���ջ������е����ݳ���
static uint8 rxBuf[SERIAL_APP_RX_CNT];  // ���ջ�����
#endif
/********************************************************************/


/* ���غ��� */
/********************************************************************/
static void SerialApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void SerialApp_HandleKeys( uint8 shift, uint8 keys );
static void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt );
static void SerialApp_SendData( uint8 *buf, uint8 len );

#if SERIAL_APP_LOOPBACK
static void rxCB_Loopback( uint8 port, uint8 event );
#else
static void rxCB( uint8 port, uint8 event );
#endif
/********************************************************************/



/*********************************************************************
 * �������ƣ�SerialApp_Init
 * ��    �ܣ�SerialApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SerialApp_Init( uint8 task_id )
{
  halUARTCfg_t uartConfig;     // ���崮�����ýṹ�����

  SerialApp_MsgID = 0x00;      // ��ʼ���������
  SerialApp_SeqRx = 0xC3;      // ��ʼ���������Ϊʮ����195
  SerialApp_TaskID = task_id;  // ��ȡӦ������ID

  /* ��ʼ��������ϢĿ�ĵ�ַ */
  SerialApp_DstAddr.endPoint = 0;
  SerialApp_DstAddr.addr.shortAddr = 0;
  SerialApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  
  /* ��ʼ����Ӧ��Ϣ��Ŀ�ĵ�ַ */
  SerialApp_RspDstAddr.endPoint = 0;
  SerialApp_RspDstAddr.addr.shortAddr = 0;
  SerialApp_RspDstAddr.addrMode = (afAddrMode_t)AddrNotPresent;

  /* ע��˵������� */
  afRegister( (endPointDesc_t *)&SerialApp_epDesc );

  /* ע�ᰴ���¼��������а����¼����͸���Ӧ������SerialApp_TaskID */
  RegisterForKeys( task_id );

  /* ���ڳ�ʼ�� */
  uartConfig.configured           = TRUE;              
  uartConfig.baudRate             = SERIAL_APP_BAUD;   // ������
  uartConfig.flowControl          = TRUE;              // ����ʹ��
  uartConfig.flowControlThreshold = SERIAL_APP_THRESH; // ������ֵ
  uartConfig.rx.maxBufSize        = SERIAL_APP_RX_MAX; // ��������
  uartConfig.tx.maxBufSize        = SERIAL_APP_TX_MAX; // �������
  uartConfig.idleTimeout          = SERIAL_APP_IDLE;   // ����ʱ��
  uartConfig.intEnable            = TRUE;              // �ж�ʹ��
/* ��ʹ���˻��ز��Թ��� */
#if SERIAL_APP_LOOPBACK
  uartConfig.callBackFunc         = rxCB_Loopback;     // �ص�����
/* ��δʹ�ܻ��ز��Թ��� */
#else
  uartConfig.callBackFunc         = rxCB;              // �ص�����
#endif
  HalUARTOpen (SERIAL_APP_PORT, &uartConfig);          // �򿪴���

  /* ��������LCD_SUPPORTED����ѡ�����LCD�Ͻ�����Ӧ����ʾ */
#if defined ( LCD_SUPPORTED )
#if defined ( ZIGBEEPRO )
  HalLcdWriteString( "SerialApp(ZigBeePRO)", HAL_LCD_LINE_2 );
#else
  HalLcdWriteString( "SerialApp(ZigBee2007)", HAL_LCD_LINE_2 );
#endif
#endif

  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�End_Device_Bind_rsp�����յ���End_Device_Bind_rsp�¼�
     ���͸���Ӧ������SerialApp_TaskID
   */    
  ZDO_RegisterForZDOMsg( SerialApp_TaskID, End_Device_Bind_rsp );
  
  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�Match_Desc_rsp�����յ���Match_Desc_rsp�¼����͸���Ӧ
     ������SerialApp_TaskID
   */  
  ZDO_RegisterForZDOMsg( SerialApp_TaskID, Match_Desc_rsp );
}


/*********************************************************************
 * �������ƣ�SerialApp_ProcessEvent
 * ��    �ܣ�SerialApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 SerialApp_ProcessEvent( uint8 task_id, UINT16 events )
{
  /* ϵͳ��Ϣ�¼� */
  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *MSGpkt;

    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(
                                                          SerialApp_TaskID )))
    {
      switch ( MSGpkt->hdr.event )
      {
        /* ZDO��Ϣ�����¼� */
        case ZDO_CB_MSG:
          // ����ZDO��Ϣ�����¼�������
          SerialApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
        
        /* �����¼� */  
        case KEY_CHANGE:
          // ���ð����¼�������
          SerialApp_HandleKeys( ((keyChange_t *)MSGpkt)->state,
                                ((keyChange_t *)MSGpkt)->keys );
          break;
        
        /* AF������Ϣ�¼� */
        case AF_INCOMING_MSG_CMD:
          // ����������Ϣ�¼�������
          SerialApp_ProcessMSGCmd( MSGpkt );
          break;
  
        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );  // �ͷŴ洢��
    }

    return ( events ^ SYS_EVENT_MSG );  // ����δ������¼�
  }

  /* ���������¼� */
  if ( events & SERIALAPP_MSG_SEND_EVT )
  {
    SerialApp_SendData( otaBuf, otaLen );  // ���÷������ݴ�����

    return ( events ^ SERIALAPP_MSG_SEND_EVT );
  }

  /* ���������ش��¼� */
  if ( events & SERIALAPP_MSG_RTRY_EVT )
  {
    /* ���ش�������Ϊ0 */
    if ( --rtryCnt )
    {
      /* ����OTA��Ϣ����Ҫ�ش��ķ������ݣ� */
      AF_DataRequest( &SerialApp_DstAddr,
                      (endPointDesc_t *)&SerialApp_epDesc,
                       SERIALAPP_CLUSTERID1, otaLen, otaBuf,
                      &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );
      
      /* ��ָ��ʱ��SERIALAPP_MSG_RTRY_TIMEOUT��ʱ�󴥷����������ش��¼� */
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT,
                                            SERIALAPP_MSG_RTRY_TIMEOUT );
    }
    else
    {
      FREE_OTABUF();  // ��������
    }

    return ( events ^ SERIALAPP_MSG_RTRY_EVT );
  }

  /* ��Ӧ��Ϣ�ش��¼� */
  if ( events & SERIALAPP_RSP_RTRY_EVT )
  {
    /* ����OTA��Ϣ����Ҫ�ش�����Ӧ��Ϣ��*/
    afStatus_t stat = AF_DataRequest( &SerialApp_RspDstAddr,
                                      (endPointDesc_t *)&SerialApp_epDesc,
                                       SERIALAPP_CLUSTERID2,
                                       SERIAL_APP_RSP_CNT, rspBuf,
                                      &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

    /* ������OTA��Ϣ����Ҫ�ش�����Ӧ��Ϣ�����ɹ�*/
    if ( stat != afStatus_SUCCESS )
    {
      /* ��ָ��ʱ��SERIALAPP_RSP_RTRY_TIMEOUT��ʱ�󴥷���Ӧ��Ϣ�ش��¼� */
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_RSP_RTRY_EVT,
                                            SERIALAPP_RSP_RTRY_TIMEOUT );
    }

    return ( events ^ SERIALAPP_RSP_RTRY_EVT );
  }

  /* ��ʹ���˻��ز��� */
#if SERIAL_APP_LOOPBACK
  /* �����ط����¼� */
  if ( events & SERIALAPP_TX_RTRY_EVT )
  { /* �����ջ������������� */
    if ( rxLen )
    { /* �������ջ������е�����д�뵽���ڲ��ɹ� */
      if ( !HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
      { /* ��ָ��ʱ��SERIALAPP_TX_RTRY_TIMEOUT�󴥷������ط����¼� */
        osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                              SERIALAPP_TX_RTRY_TIMEOUT );
      }
      /* �������ջ������е�����д�뵽���ڳɹ� */
      else
      {
        rxLen = 0;  // ������ջ����������ݳ��ȱ���
      }
    }

    return ( events ^ SERIALAPP_TX_RTRY_EVT );
  }
#endif

  /* ����δ֪�¼� */ 
  return ( 0 ); 
}


/*********************************************************************
 * �������ƣ�SerialApp_ProcessZDOMsgs
 * ��    �ܣ�SerialApp��ZDO��Ϣ�����¼���������
 * ��ڲ�����inMsg  ָ�������ZDO��Ϣ��ָ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void SerialApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  /* ZDO��Ϣ�еĴ�ID */
  switch ( inMsg->clusterID )
  {
    /* ZDO�Ĵ�End_Device_Bind_rsp */
    case End_Device_Bind_rsp:
      /* ���󶨳ɹ� */
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        /* ����LED4 */
        /* ����SK-SmartRF05EB���ԣ�LED4��LED_B����ɫ�� */
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
/* ��������BLINK_LEDS�� */
#if defined(BLINK_LEDS)
      /* ����ʧ�� */
      else
      {
        /* ��˸LED4 */
        /* ����SK-SmartRF05EB���ԣ�LED4��LED_B����ɫ�� */
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    /* ZDO�Ĵ�Match_Desc_rsp */      
    case Match_Desc_rsp:
      {
        /* ��ȡָ����Ӧ��Ϣ��ָ�� */
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        /* ��ָ����Ӧ��Ϣ��ָ�����pRsp��ֵ��Ϊ��(NULL) */
        if ( pRsp )
        { /* ָ����Ӧ��Ϣ��ָ�����pRsp��ֵ��Ч */
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            /* ���÷�����Ϣ��Ŀ�ĵ�ַ */
            SerialApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            SerialApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // ���õ�һ���˵㣬�û������޸Ĵ˴��������������ж˵�
            SerialApp_DstAddr.endPoint = pRsp->epList[0];
            
            /* ����LED4 */
            /* ����SK-SmartRF05EB���ԣ�LED4��LED_B����ɫ�� */
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );  // �ͷ�ָ�����pRsp��ռ�õĴ洢��
        }
      }
      break;
  }
}


/*********************************************************************
 * �������ƣ�SerialApp_HandleKeys
 * ��    �ܣ�SerialApp�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ�����ĿΪ��
 *                  HAL_KEY_SW_2     SK-SmartRF05EB��RIGHT��
 *                  HAL_KEY_SW_4     SK-SmartRF05EB��LEFT��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SerialApp_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t dstAddr;
  (void)shift;
  
  /* HAL_KEY_SW_2�����£����͡��ն��豸������ */
  /* HAL_KEY_SW_2��ӦSK-SmartRF05EB��RIGHT�� */  
  if ( keys & HAL_KEY_SW_2 )
  {
    /* Ϩ��LED4 */
    /* ����SK-SmartRF05EB���ԣ�LED4��LED_B����ɫ�� */     
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      
    /* Ϊǿ�ƶ˵㷢��һ�����ն��豸�����󡱣�������*/
    dstAddr.addrMode = Addr16Bit;
    dstAddr.addr.shortAddr = 0x0000; // Ŀ�ĵ�ַΪ������Э������16λ�̵�ַ0x0000
    ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(), 
                          SerialApp_epDesc.endPoint,
                          SERIALAPP_PROFID,
                          SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                          SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                          FALSE );
  }

  /* HAL_KEY_SW_4�����£����͡�ƥ������������ */
  /* HAL_KEY_SW_4��ӦSK-SmartRF05EB��LEFT�� */   
  if ( keys & HAL_KEY_SW_4 )
  {
    /* Ϩ��LED4 */
    /* ����SK-SmartRF05EB���ԣ�LED4��LED_B����ɫ�� */
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
   
    /* ����һ����ƥ�����������󡱣��㲥��*/
    dstAddr.addrMode = AddrBroadcast;
    dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
    ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                      SERIALAPP_PROFID,
                      SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                      SERIALAPP_MAX_CLUSTERS, (cId_t *)SerialApp_ClusterList,
                      FALSE );
  }
}


/*********************************************************************
 * �������ƣ�SerialApp_ProcessMSGCmd
 * ��    �ܣ�SerialApp��������Ϣ�¼����������������������������豸��
 *           �κ��������ݡ���ˣ��ɻ��ڴ�IDִ��Ԥ���Ķ�����
 * ��ڲ�����pkt  ָ��������Ϣ��ָ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt )
{
  uint8 stat;
  uint8 seqnb;
  uint8 delay;

  /* AF������Ϣ�еĴ� */
  switch ( pkt->clusterId )
  {
  /* ��SERIALAPP_CLUSTERID1 */
  /* �������� */
  case SERIALAPP_CLUSTERID1:
    seqnb = pkt->cmd.Data[0];  // ȡ���������ݵķ������

    /* �����յ��ķ������ݲ����ظ����� */
    if ( (seqnb > SerialApp_SeqRx) ||                    // Normal
        ((seqnb < 0x80 ) && ( SerialApp_SeqRx > 0x80)) ) // Wrap-around
    {
      /* �������յ��ķ������ݳɹ�д�봮�� */
      if ( HalUARTWrite( SERIAL_APP_PORT, pkt->cmd.Data+1,
                                         (pkt->cmd.DataLength-1) ) )
      {
        /*
           ���ղŷ������ݵķ��������Ϊ��������Ա����´ν��յ�
           ��������ʱ��Ϊ�ж�����
         */
        SerialApp_SeqRx = seqnb;  
        stat = OTA_SUCCESS;  // ״̬��־Ϊ�ɹ�
      }
      /* �������յ��ķ�������д�봮��ʧ�ܣ�˵������æ */
      else
      {
        stat = OTA_SER_BUSY;  // ״̬��־Ϊ����æ 
      }
    }
    /* �����յ��ķ����������ظ����� */
    else
    {
      stat = OTA_DUP_MSG;  // ״̬��־Ϊ�ظ�����
    }

    /* ��OTA��Ϣ����ѡ��һ��ǡ������ʱ */
    delay = (stat == OTA_SER_BUSY) ? SERIALAPP_NAK_DELAY : SERIALAPP_ACK_DELAY;

    /* ���첢������Ӧ��Ϣ */
    rspBuf[0] = stat;                  // ��Խ��յ��ķ������ݸ����Ĵ���״̬
    rspBuf[1] = seqnb;                 // ȷ����ţ����յ��ķ������ݵķ�����ţ�
    rspBuf[2] = LO_UINT16( delay );    // OTA��Ϣ������ʱ
    rspBuf[3] = HI_UINT16( delay );
    stat = AF_DataRequest( &(pkt->srcAddr), (endPointDesc_t*)&SerialApp_epDesc,
                            SERIALAPP_CLUSTERID2, SERIAL_APP_RSP_CNT , rspBuf,
                           &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

    /* ��OTA��Ϣ����Ӧ��Ϣ�����Ͳ��ɹ� */
    if ( stat != afStatus_SUCCESS )
    {
      /* ��ָ��ʱ��SERIALAPP_RSP_RTRY_TIMEOUT��ʱ�󴥷���Ӧ��Ϣ�ش��¼� */
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_RSP_RTRY_EVT,
                                            SERIALAPP_RSP_RTRY_TIMEOUT );

      /* Ϊ��Ӧ��Ϣ�ش��洢�����Ŀ�ĵ�ַ */
      osal_memcpy(&SerialApp_RspDstAddr, &(pkt->srcAddr), sizeof( afAddrType_t ));
    }
    break;

  /* ��SERIALAPP_CLUSTERID2 */
  /* ��Ӧ��Ϣ */
  case SERIALAPP_CLUSTERID2:
    /* ���Է���������Ӧ��Ϣ�������������ѱ��Է��ɹ����ջ�
       �������ݶ��ڶԷ���˵���ظ�����
    */
    if ( (pkt->cmd.Data[1] == SerialApp_SeqTx) &&
        ((pkt->cmd.Data[0] == OTA_SUCCESS) ||
         (pkt->cmd.Data[0] == OTA_DUP_MSG)) )
    {
      /* ֹͣ���������ش��¼��Ĵ��� */
      osal_stop_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT );
      
      FREE_OTABUF();  // ����������
    }
    /* ���Է���������Ӧ��Ϣ������������δ���Է��ɹ�����
       ��ע�⣺�˴��ɹ�������ָ�����յ������ݳɹ�д�봮�ڡ�
    */
    else
    {
      /* �ӶԷ���������Ӧ��Ϣ��ȡ���Է���Ҫ�����OTA��Ϣ���ص���ʱ */
      delay = BUILD_UINT16( pkt->cmd.Data[2], pkt->cmd.Data[3] );
      
      /* ��ָ��ʱ��delay��ʱ�󴥷����������ش��¼� */
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT, delay );
    }
    break;

    default:
      break;
  }
}


/*********************************************************************
 * �������ƣ�SerialApp_SendData
 * ��    �ܣ�SerialApp�ķ����������ݺ���
 * ��ڲ�����buf  ָ�������ݻ�������ָ��
 *           len  �������ݳ���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void SerialApp_SendData( uint8 *buf, uint8 len )
{
  afStatus_t stat;

  *buf = ++SerialApp_SeqTx;  // ���㷢�����

  otaBuf = buf;   // ��otaBufָ�򱣴淢�����ݵĻ�����
  otaLen = len+1; // ���������˷���������1�ֽڵ��ֶΣ����Է������ݳ��ȼ�1

  /* ����OTA��Ϣ���������ݣ�*/
  stat = AF_DataRequest( &SerialApp_DstAddr,
                         (endPointDesc_t *)&SerialApp_epDesc,
                          SERIALAPP_CLUSTERID1,
                          otaLen, otaBuf,
                          &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

  /* ���������ݱ��ɹ����ͻ�����˴洢������ */
  if ( (stat == afStatus_SUCCESS) || (stat == afStatus_MEM_FAIL) )
  {
    /* ��ָ��ʱ��SERIALAPP_MSG_RTRY_TIMEOUT��ʱ�󴥷����������ش��¼� */
    osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT,
                      SERIALAPP_MSG_RTRY_TIMEOUT );
    
    rtryCnt = SERIALAPP_MAX_RETRIES;  // ������ش�������ֵ���ش�������������
  }
  /* ���������� */
  else
  {
    FREE_OTABUF();
  }
}


/* ��ʹ���˻��ز��Թ��� */
#if SERIAL_APP_LOOPBACK
/*********************************************************************
 * �������ƣ�rxCB_Loopback
 * ��    �ܣ����ڽ��ջص����������ز���ʱʹ�ã�
 * ��ڲ�����port   ���ں�
 *           event  �����¼�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rxCB_Loopback( uint8 port, uint8 event )
{
  /* �����ջ������������� */
  if ( rxLen )
  {
    /* �������ջ������е�����д�뵽���ڲ��ɹ� */
    if ( !HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
    {
      /* ��ָ��ʱ��SERIALAPP_TX_RTRY_TIMEOUT�󴥷������ط����¼� */
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                            SERIALAPP_TX_RTRY_TIMEOUT );
      return;  // ����
    }
    /* �������ջ������е�����д�뵽���ڳɹ� */
    else
    { /* ֹͣ�����ط����¼� */
      osal_stop_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT );
    }
  }

  /* ���Ӵ��ڶ�ȡ���ݲ��ɹ�(���������ݳ���Ϊ0) */
  if ( !(rxLen = HalUARTRead( port, rxBuf, SERIAL_APP_RX_CNT )) )
  {
    return;  // ����
  }

  /* �����ѴӴ��ڶ�ȡ�����ݻ�д�����ڳɹ� */
  if ( HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
  {
    rxLen = 0;  // ������ջ����������ݳ��ȱ���
  }
  /* �����ѴӴ��ڶ�ȡ�����ݻ�д�����ڲ��ɹ� */
  else
  { /* ��ָ��ʱ��SERIALAPP_TX_RTRY_TIMEOUT�󴥷������ط����¼� */ 
    osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                          SERIALAPP_TX_RTRY_TIMEOUT );
  }
}


/* ��δʹ�ܻ��ز��Թ��� */
#else
/*********************************************************************
 * �������ƣ�rxCB
 * ��    �ܣ����ڽ��ջص�����������Ӧ��ʱʹ�ã�
 * ��ڲ�����port   ���ں�
 *           event  �����¼�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rxCB( uint8 port, uint8 event )
{
  uint8 *buf, len;

  /*
     ��Ӧ�����ֻʹ����OSAL��̬�����2���洢���ռ����洢�����͵�OTA��Ϣ��
     �������洢���ռ�ֱ���otaBuf��otaBuf2ָ����ָ��
     ��Ӧ�����ǽ�otaBufָ��ָ��Ĵ洢���ռ���Ϊ���߷��ͻ�����������otaBuf2ָ��
     ָ��Ĵ洢���ռ���Ϊ�󱸻���������otaBufָ��Ļ������е����ݱ�������󣬽�
     �ͷŸû������ռ䣬����ʱotaBuf2��ΪNULL��˵��otaBuf2ָ��Ļ������а������ݣ�
     ��ôotaBuf��ָ��û���������otaBuf2������ΪNULL����otaBuf��otaBuf2����ΪNULL��
     ʱ������ʱ�û�PCͨ�����ڷ��������͵�OTA��Ϣ����ʱ����������д������ֱ����
     ����������������Ϊ��ʱ�����������ж������ݣ��޷��ٽ��������û�PC���ڵ����ݡ�
     ����������CC2430��UART���������û�PC�Ĵ��ڽ���Ӳ�����ء�
  */
  
  /* ��otaBuf2ָ�벻ΪNULL */
  if ( otaBuf2 )
  {
    return;  // ����
  }
  
  /* ������洢���ռ䲻�ɹ� */
  if ( !(buf = osal_mem_alloc( SERIAL_APP_RX_CNT )) )
  {
    return;  // ����
  }

  /* �Ӵ��ڶ������ݴ��뻺������������������һ���ֽ������洢������� */
  len = HalUARTRead( port, buf+1, SERIAL_APP_RX_CNT-1 );

  /* ���Ӵ���û�������� */
  if ( !len )  
  {
    osal_mem_free( buf );  // �ͷ��ѷ���Ļ�����
    return;                // ����
  }

  /* ��otaBuf��ΪNULL */
  if ( otaBuf )
  {
    otaBuf2 = buf;  // ��otaBuf2ָ��buf��ָ��Ĵ洢���ռ�
    otaLen2 = len;  // otaBuf2ָ��Ĵ洢���ռ�ĳ���
  }
  /* ��otaBufΪNULL */
  else
  {
    otaBuf = buf;  // ��otaBufָ��buf��ָ��Ĵ洢���ռ�
    otaLen = len;  // otaBufָ��Ĵ洢���ռ�ĳ���
    
    /*
       �������÷��������¼�
       ע�⣺��Ҫ�ڱ��ص�������ֱ�ӵ���SerialApp_SendData()����������OTA��Ϣ��
             ����Ӧ��ʹ�����÷��������¼��ķ�������������Ŀ����Ϊ�˷�ֹ���ص�
             ��������ʱ��������
     */
    osal_set_event( SerialApp_TaskID, SERIALAPP_MSG_SEND_EVT );
  }
}
#endif


