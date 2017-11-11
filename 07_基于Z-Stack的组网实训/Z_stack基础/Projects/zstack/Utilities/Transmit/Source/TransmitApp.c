/*******************************************************************************
 * �ļ����ƣ�TransmitApp.c
 * ��    �ܣ�TransmitAppʵ�顣
 * ʵ�����ݣ�һ��ZigBee�����е�ĳ���豸�����ܿ������һ���豸�������ݰ���������
 *           ����һ�����ݰ�OTA���ͣ������Ƿ�ʹ����APS ACK��ֻҪ�յ�ȷ�Ͼ����̷�
 *           ����һ�����ݰ�����
 *           �����豸ͬʱ����㣺
 *               ÿ�뷢�͵�ƽ���ֽ�����
 *               �ۼƷ��͵��ֽ�����
 *           �����豸�ڽ��յ����ݰ������㣺
 *               ÿ����յ���ƽ���ֽ�����
 *               �ۼƽ��յ����ֽ�����
 * 
 *           ���û�ʹ����LCD��ʾ����LCD�Ͻ������·�ʽ��ʾ����4�����ݣ�
 *               aaaa   bbbb
 *               cccc   dddd
 *           ���У�
 *                 aaaa ��ʾÿ����յ���ƽ���ֽ�����
 *                 bbbb ��ʾ�ۼƽ��յ����ֽ�����
 *                 cccc ��ʾÿ�뷢�͵�ƽ���ֽ�����
 *                 dddd ��ʾ�ۼƷ��͵��ֽ�����
 *
 * ע    �⣺��ʵ��ɱ���������һ��Э������һ��·����֮���������������ԡ�Ĭ��
 *           ��ʹ��APS ACK���ƣ���ʹ�øû��ƣ�������������½��� 
 *           ��ʵ��Ҳ�ɱ���������һ��Э��������·��������һ���ն��豸֮������
 *           ���������ԡ������ն��豸�����ԣ�����RESPONSE_POLL_RATE����ֵ��
 *           f8wConfig.cfg�ļ��ж��壩��ÿ�����ݷ��ͱ���Ҫ������ʱ����ʱʱ��ҪԶ
 *           ����RESPONSE_POLL_RATE����ʵ����Ĭ��Ϊ2����RESPONSE_POLL_RATEʱ�䡣
 *           
 * ʵ���豸��SK-SmartRF05EB��װ����SK-CC2530EM��
 *           UP����       �л�����/�ȴ�״̬
 *           RIGHT����    ���͡��ն��豸������
 *           DOWN����     ������ʾ��Ϣ
 *           LEFT����     ���͡�ƥ������������
 *           LED_Y(��ɫ)�����ɹ�������������������LED����
 *           LED_B(��ɫ)��������һ�豸�ɹ�����Ӧ�ò���߼����Ӻ��LED����
 *           
 ******************************************************************************/



/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "AF.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "TransmitApp.h"
#include "OnBoard.h"

#include "DebugTrace.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
/********************************************************************/



/* ���� */
/********************************************************************/

/*
   ��Ӧ�õ�״̬
 */
#define TRANSMITAPP_STATE_WAITING   0  // �ȴ�
#define TRANSMITAPP_STATE_SENDING   1  // ����

/*
   �ն��豸����ʹ�õ���ʱ���������ݰ�֮�����ʱ�����Ժ���Ϊ��λ
 */
#if !defined ( RTR_NWK )
  #define TRANSMITAPP_DELAY_SEND
  #define TRANSMITAPP_SEND_DELAY    (RESPONSE_POLL_RATE * 2)
#endif

/*
   ���ݰ�����ѡ��
 */
/* ��ҪAPS ACK */
//#define TRANSMITAPP_TX_OPTIONS    (AF_DISCV_ROUTE | AF_ACK_REQUEST)  

/* ����ҪAPS ACK */
#define TRANSMITAPP_TX_OPTIONS    AF_DISCV_ROUTE  

/*
   ����һ�����ݷ�����Ҫ�������͵����ݰ�����
 */
#define TRANSMITAPP_INITIAL_MSG_COUNT  2

/*
   ������ʾ�¼��ĳ�ʱ����ʱ��
 */
#define TRANSMITAPP_DISPLAY_TIMER   (2 * 1000)

/*
   ������ݰ�����
   �������Ƭ����������ݰ�����Ϊ225�ֽڣ�����������ݰ�����Ϊ102�ֽ�
 */
#if defined ( TRANSMITAPP_FRAGMENTED )
#define TRANSMITAPP_MAX_DATA_LEN    225
#else
#define TRANSMITAPP_MAX_DATA_LEN    102
#endif

/* 
   ��ID�б� 
   ���б�Ӧ���ñ�Ӧ��ר�õĴ�ID�����
*/
const cId_t TransmitApp_ClusterList[TRANSMITAPP_MAX_CLUSTERS] =
{
  TRANSMITAPP_CLUSTERID_TESTMSG
};

/* 
   �豸�ļ������� 
 */
const SimpleDescriptionFormat_t TransmitApp_SimpleDesc =
{
  TRANSMITAPP_ENDPOINT,              //  �˵��
  TRANSMITAPP_PROFID,                //  Profile ID
  TRANSMITAPP_DEVICEID,              //  �豸ID
  TRANSMITAPP_DEVICE_VERSION,        //  �豸�汾
  TRANSMITAPP_FLAGS,                 //  ��ʶ
  TRANSMITAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)TransmitApp_ClusterList,  //  ������б�
  TRANSMITAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)TransmitApp_ClusterList   //  ������б�
};

/* 
   �˵������� 
*/
const endPointDesc_t TransmitApp_epDesc =
{
  TRANSMITAPP_ENDPOINT,
  &TransmitApp_TaskID,
  (SimpleDescriptionFormat_t *)&TransmitApp_SimpleDesc,
  noLatencyReqs
};
/********************************************************************/



/* ���� */
/********************************************************************/

/*
   ��Ӧ�õ�����ID��������TransmitApp_Init()����������ʱ��   �ñ�������
   �������IDֵ��
*/
uint8 TransmitApp_TaskID;

/*
   �������ݻ�����
 */
byte TransmitApp_Msg[ TRANSMITAPP_MAX_DATA_LEN ];

/*
   �豸������״̬������Э����/·����/�ն��豸/�豸����������
*/
devStates_t TransmitApp_NwkState;

/*
   �������
 */
static byte TransmitApp_TransID;  

/*
   Ŀ�ĵ�ַ
 */
afAddrType_t TransmitApp_DstAddr;

/*
   ��Ӧ�õ�״̬
 */
byte TransmitApp_State;

/*
   OSALϵͳʱ�ӵ�Ӱ�ӣ���������ʵ�ʵ���ʱ��
 */
static uint32 clkShdw;

/*
   �ӿ�ʼ��ǰ���е����ڽ���/���Ͳ������ݵ�������
 */
static uint32 rxTotal, txTotal;

/*
   ���ϴ���ʾ/���µ����ڽ���/�������ݵ�����
 */
static uint32 rxAccum, txAccum;

/*
   ��������������ʾ�¼��ĳ�ʱ��ʱ���Ĵ򿪱�־
 */
static byte timerOn;

/*
   ����һ�����ݷ�����Ҫ�����������ݰ��ļ���
 */
static byte timesToSend;

/*
   ���ݷ��ʹ�������
 */
uint16 pktCounter;

/*
   ���������ݰ���������ݳ���
 */
uint16 TransmitApp_MaxDataLength;
/********************************************************************/



/* ���غ��� */
/********************************************************************/
void TransmitApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void TransmitApp_HandleKeys( byte shift, byte keys );
void TransmitApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void TransmitApp_SendTheMessage( void );
void TransmitApp_ChangeState( void );
/********************************************************************/



/* ���к��� */
/********************************************************************/
void TransmitApp_DisplayResults( void );
/********************************************************************/



/*********************************************************************
 * �������ƣ�SerialApp_Init
 * ��    �ܣ�SerialApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_Init( byte task_id )
{
#if !defined ( TRANSMITAPP_FRAGMENTED )
  afDataReqMTU_t mtu;
#endif
  uint16 i;

  TransmitApp_TaskID = task_id;
  TransmitApp_NwkState = DEV_INIT;
  TransmitApp_TransID = 0;

  pktCounter = 0;

  TransmitApp_State = TRANSMITAPP_STATE_WAITING;

  /* ��ʼ��Ŀ�ĵ�ַ */
  TransmitApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  TransmitApp_DstAddr.endPoint = 0;
  TransmitApp_DstAddr.addr.shortAddr = 0;

  /* ע��˵������� */
  afRegister( (endPointDesc_t *)&TransmitApp_epDesc );

  /* ע�ᰴ���¼��������а����¼����͸���Ӧ������TransmitApp_TaskID */
  RegisterForKeys( TransmitApp_TaskID );

  /* ��������LCD_SUPPORTED����ѡ�����LCD�Ͻ�����Ӧ����ʾ */
#if defined ( LCD_SUPPORTED )
#if defined ( ZIGBEEPRO )
  HalLcdWriteString( "TransmitApp(ZB_PRO)", HAL_LCD_LINE_2 );
#else
  HalLcdWriteString( "TransmitApp(ZB_2007)", HAL_LCD_LINE_2 );
#endif
#endif

/* ���������ݷ�Ƭ */
#if defined ( TRANSMITAPP_FRAGMENTED )
  TransmitApp_MaxDataLength = TRANSMITAPP_MAX_DATA_LEN;
/* �����������ݷ�Ƭ */
#else
  mtu.kvp        = FALSE;
  mtu.aps.secure = FALSE;
  TransmitApp_MaxDataLength = afDataReqMTU( &mtu );
#endif

  /* ������������ */
  for (i=0; i<TransmitApp_MaxDataLength; i++)
  {
    TransmitApp_Msg[i] = (uint8) i;
  }

  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�End_Device_Bind_rsp�����յ���End_Device_Bind_rsp�¼�
     ���͸���Ӧ������SerialApp_TaskID
   */      
  ZDO_RegisterForZDOMsg( TransmitApp_TaskID, End_Device_Bind_rsp );
   
  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�Match_Desc_rsp�����յ���Match_Desc_rsp�¼����͸���Ӧ
     ������SerialApp_TaskID
   */    
  ZDO_RegisterForZDOMsg( TransmitApp_TaskID, Match_Desc_rsp );
}


/*********************************************************************
 * �������ƣ�TransmitApp_ProcessEvent
 * ��    �ܣ�TransmitApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 TransmitApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  /* �������ڱ���AF����ȷ����Ϣ���ֶ�ֵ�ı��� */
  ZStatus_t sentStatus;
  byte sentEP;

  /* ϵͳ��Ϣ�¼� */
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( TransmitApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      { /* ZDO��Ϣ�����¼� */
        case ZDO_CB_MSG:
          // ����ZDO��Ϣ�����¼�������
          TransmitApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
         
        /* �����¼� */  
        case KEY_CHANGE:
          // ���ð����¼�������
          TransmitApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, 
			          ((keyChange_t *)MSGpkt)->keys );
          break;

        /* AF����ȷ���¼� */
        case AF_DATA_CONFIRM_CMD:
          // ��ȡAF����ȷ����Ϣ������Ӧ����
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;

          /* �����ݱ��ɹ�������OTA���� */
          if ( (ZSuccess == sentStatus) && (TransmitApp_epDesc.endPoint == sentEP))
          {
/* �����ݳ��Ȳ�Ϊ������� */
#if !defined ( TRANSMITAPP_RANDOM_LEN )
            txAccum += TransmitApp_MaxDataLength;  // �ۼƷ������ݵ�����
#endif
            /* ����������������ʾ�¼��ĳ�ʱ��ʱ��δ���� */
            if ( !timerOn )
            {
              /* 
                 ��ָ����TRANSMITAPP_DISPLAY_TIMERʱ�䵽�ں󴥷�������ʾ
                 �¼�TRANSMITAPP_RCVTIMER_EVT
               */
              osal_start_timerEx( TransmitApp_TaskID,TRANSMITAPP_RCVTIMER_EVT,
                                                     TRANSMITAPP_DISPLAY_TIMER);
              
              clkShdw = osal_GetSystemClock();  // ��ȡ��ǰϵͳʱ�Ӽ���
              
              /* 
                 ������������������ʾ�¼��ĳ�ʱ��ʱ��״̬����ΪTURE
                 ����������������ʾ�¼��ĳ�ʱ��ʱ���ѱ���
               */
              timerOn = TRUE;
            }
          }

          TransmitApp_SetSendEvt();  // ���յ�ȷ�Ϻ�����һ�����ݰ�
          break;

        /* AF������Ϣ�¼� */
        case AF_INCOMING_MSG_CMD:
          // ����������Ϣ�¼�������
          TransmitApp_MessageMSGCB( MSGpkt );
          break;

        /* ZDO״̬�ı��¼� */
        case ZDO_STATE_CHANGE:
          // ��ȡ�豸״̬
          TransmitApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );  // �ͷŴ洢��

      // ��ȡ��һ��ϵͳ��Ϣ�¼�
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( TransmitApp_TaskID );
    }

    return (events ^ SYS_EVENT_MSG);  // ����δ������¼�
  }

  /* ���ݷ����¼� */
  if ( events & TRANSMITAPP_SEND_MSG_EVT )
  {
    /* ��Ϊ����״̬ */
    if ( TransmitApp_State == TRANSMITAPP_STATE_SENDING )
    { 
      /* �������ݷ����¼������� */
      TransmitApp_SendTheMessage();
    }

    return (events ^ TRANSMITAPP_SEND_MSG_EVT);
  }

  /* ���ʹ����¼� */
  if ( events & TRANSMITAPP_SEND_ERR_EVT )
  {
    TransmitApp_SetSendEvt();  // �������ݷ����¼���־

    return (events ^ TRANSMITAPP_SEND_ERR_EVT);
  }

  /* ������ʾ�¼� */
  if ( events & TRANSMITAPP_RCVTIMER_EVT )
  {
    /* 
       ��ָ����TRANSMITAPP_DISPLAY_TIMERʱ�䵽�ں󴥷�������ʾ
       �¼�TRANSMITAPP_RCVTIMER_EVT
     */
    osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT,
                                            TRANSMITAPP_DISPLAY_TIMER );
    TransmitApp_DisplayResults();  // ������ʾ���

    return (events ^ TRANSMITAPP_RCVTIMER_EVT);
  }
  
  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�TransmitApp_ProcessZDOMsgs
 * ��    �ܣ�TransmitApp��ZDO��Ϣ�����¼���������
 * ��ڲ�����inMsg  ָ�������ZDO��Ϣ��ָ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  /* ZDO��Ϣ�еĴ�ID */
  switch ( inMsg->clusterID )
  { /* ZDO�Ĵ�End_Device_Bind_rsp */
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
      {  /* ��ȡָ����Ӧ��Ϣ��ָ�� */
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        /* ��ָ����Ӧ��Ϣ��ָ�����pRsp��ֵ��Ϊ��(NULL) */
        if ( pRsp )
        { /* ָ����Ӧ��Ϣ��ָ�����pRsp��ֵ��Ч */
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            /* ����Ŀ�ĵ�ַ */
            TransmitApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            TransmitApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // ���õ�һ���˵㣬�û������޸Ĵ˴��������������ж˵�
            TransmitApp_DstAddr.endPoint = pRsp->epList[0];
            
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
 * �������ƣ�TransmitApp_HandleKeys
 * ��    �ܣ�TransmitApp�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ�����ĿΪ��
 *                  HAL_KEY_SW_1     SK-SmartRF05EB��UP��
 *                  HAL_KEY_SW_2     SK-SmartRF05EB��RIGHT��
 *                  HAL_KEY_SW_3     SK-SmartRF05EB��DOWN��
 *                  HAL_KEY_SW_4     SK-SmartRF05EB��LEFT��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;

  /* HAL_KEY_SW_1�����£��л���Ӧ��״̬������/�ȴ��� */
  /* HAL_KEY_SW_1��ӦSK-SmartRF05EB��UP�� */  
  if ( keys & HAL_KEY_SW_1 )
  {
    TransmitApp_ChangeState();
  }

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
                            TransmitApp_epDesc.endPoint,
                            TRANSMITAPP_PROFID,
                            TRANSMITAPP_MAX_CLUSTERS, (cId_t *)TransmitApp_ClusterList,
                            TRANSMITAPP_MAX_CLUSTERS, (cId_t *)TransmitApp_ClusterList,
                            FALSE );
  }

  /* HAL_KEY_SW_3�����£�������ʾ��Ϣ */
  /* HAL_KEY_SW_3��ӦSK-SmartRF05EB��DOWN�� */  
  if ( keys & HAL_KEY_SW_3 )
  {
    rxTotal = txTotal = 0;
    rxAccum = txAccum = 0;
    TransmitApp_DisplayResults();
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
                        TRANSMITAPP_PROFID,
                        TRANSMITAPP_MAX_CLUSTERS, (cId_t *)TransmitApp_ClusterList,
                        TRANSMITAPP_MAX_CLUSTERS, (cId_t *)TransmitApp_ClusterList,
                        FALSE );
  }
}


/*********************************************************************
 * �������ƣ�TransmitApp_MessageMSGCB
 * ��    �ܣ�TransmitApp��������Ϣ�¼����������������������������豸��
 *           �κ��������ݡ���ˣ��ɻ��ڴ�IDִ��Ԥ���Ķ�����
 * ��ڲ�����pkt  ָ��������Ϣ��ָ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  int16 i;
  uint8 error = FALSE;

  /* AF������Ϣ�еĴ� */
  switch ( pkt->clusterId )
  { /* ��TRANSMITAPP_CLUSTERID_TESTMSG */
    case TRANSMITAPP_CLUSTERID_TESTMSG:
#if !defined ( TRANSMITAPP_RANDOM_LEN )      
      /* ���������ݰ����Ȳ���ȷ */
      if (pkt->cmd.DataLength != TransmitApp_MaxDataLength)
      {
        error = TRUE;  // �ô����־ΪTRUE
      }
#endif
      
      /* �ж����ݰ��е������Ƿ���������� */
      for (i=4; i<pkt->cmd.DataLength; i++)
      {
        if (pkt->cmd.Data[i] != i%256)
          error = TRUE;
      }

      /* �������˴������ */
      if (error)
      {
        /* ����LED2������ָʾ�����˴������ */
        /* ����SK-SmartRF05EB���ԣ�LED2��LED_R����ɫ�� */
        HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
      }
      /* ��û�в���������� */
      else
      { /* ����������������ʾ�¼��ĳ�ʱ��ʱ��δ���� */
        if ( !timerOn )
        {
          /* 
             ��ָ����TRANSMITAPP_DISPLAY_TIMERʱ�䵽�ں󴥷�������ʾ
             �¼�TRANSMITAPP_RCVTIMER_EVT
           */          
          osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT,
                                                  TRANSMITAPP_DISPLAY_TIMER );
         
          clkShdw = osal_GetSystemClock();  // ��ȡ��ǰϵͳʱ�Ӽ���
              
          /* 
             ������������������ʾ�¼��ĳ�ʱ��ʱ��״̬����ΪTURE
             ����������������ʾ�¼��ĳ�ʱ��ʱ���ѱ���
           */
          timerOn = TRUE;
        }
        rxAccum += pkt->cmd.DataLength;  // �ۼƽ������ݵ�����
      }
      break;

    default:
      break;
  }
}


/*********************************************************************
 * �������ƣ�TransmitApp_SendTheMessage
 * ��    �ܣ�TransmitApp�����ݷ����¼�������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_SendTheMessage( void )
{
  uint16 len;
  uint8 tmp;

  /* ����һ�����ݷ��ͣ���������timesToSend�����ݰ� */
  /* ���������������ݰ�����δ�ﵽtimesToSend֮ǰ���ַ��ʹ���������ѭ�� */
  do
  {  
    /* ���������TransmitApp_TransID���뷢�����ݻ������ĵ�2��3�ֽ� */
    tmp = HI_UINT8( TransmitApp_TransID );
    tmp += (tmp <= 9) ? ('0') : ('A' - 0x0A);
    TransmitApp_Msg[2] = tmp;
    tmp = LO_UINT8( TransmitApp_TransID );
    tmp += (tmp <= 9) ? ('0') : ('A' - 0x0A);
    TransmitApp_Msg[3] = tmp;
    
    len = TransmitApp_MaxDataLength;  // ��ȡ���������ݰ�����󳤶�

  /* �����������ݰ��ĳ���ʹ��������� */
#if defined ( TRANSMITAPP_RANDOM_LEN )
    len = (uint8)(osal_rand() & 0x7F);  // ��ȡ���������ݰ����������
    /* ����ȡ���ı��������ݰ���������ȴ��ڱ��������ݰ�����󳤶Ȼ����0 */
    if( len > TransmitApp_MaxDataLength || len == 0 )
      // �ñ��������ݰ�����󳤶���Ϊ���������ݰ��ĳ���
      len = TransmitApp_MaxDataLength;  
    /* ����ȡ���ı��������ݰ����������С��4 */
    else if ( len < 4 )
      // ���������ݰ��ĳ�������Ϊ4
      len = 4;    
#endif

    /* �������ݰ���OTA���� */
    tmp = AF_DataRequest( &TransmitApp_DstAddr, 
                          (endPointDesc_t *)&TransmitApp_epDesc,
                           TRANSMITAPP_CLUSTERID_TESTMSG,
                           len, TransmitApp_Msg,
                          &TransmitApp_TransID,
                           TRANSMITAPP_TX_OPTIONS,
                           AF_DEFAULT_RADIUS );

/* �����������ݰ�ʹ��������� */    
#if defined ( TRANSMITAPP_RANDOM_LEN )
    /* �����ݰ����ɹ�������OTA���� */
    if ( tmp == afStatus_SUCCESS )
    {
      txAccum += len;  // �ۼƷ������ݵ�����
    }
#endif    
    
    /* �������ݰ����ʹ������� */
    if ( timesToSend )
    {
      timesToSend--;  
    }
  } 
  while ( (timesToSend != 0) && (afStatus_SUCCESS == tmp) );

  /* ��һ�����ݷ��ͳɹ� */
  if ( afStatus_SUCCESS == tmp )
  {
    pktCounter++;  // ���ݷ��ʹ����ۼ�
  }
  /* ��һ�����ݷ��Ͳ��ɹ� */
  else
  {
    /* ��ָ��ʱ��10ms��ʱ�󴥷����ʹ����¼�TRANSMITAPP_SEND_ERR_EVT */
    osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_SEND_ERR_EVT, 10 );
  }
}


/*********************************************************************
 * �������ƣ�TransmitApp_ChangeState
 * ��    �ܣ��л�����/�ȴ�״̬��־
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_ChangeState( void )
{
  /* ��Ϊ�ȴ�״̬ */
  if ( TransmitApp_State == TRANSMITAPP_STATE_WAITING )
  {
    /* �л��ȴ�״̬Ϊ����״̬ */
    TransmitApp_State = TRANSMITAPP_STATE_SENDING; // ����״̬��־Ϊ����
    TransmitApp_SetSendEvt();                      // �������ݷ����¼���־
    timesToSend = TRANSMITAPP_INITIAL_MSG_COUNT;   // ��ʼ�����ݰ����ʹ�������
  }
  /* ��Ϊ����״̬ */
  else
  {
    TransmitApp_State = TRANSMITAPP_STATE_WAITING; // ����״̬��ʾ��־Ϊ�ȴ�
  }
}


/*********************************************************************
 * �������ƣ�TransmitApp_SetSendEvt
 * ��    �ܣ��������ݷ����¼���־
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_SetSendEvt( void )
{
/* ����������ʱ���ͣ�ʹ���ն��豸��*/
#if defined( TRANSMITAPP_DELAY_SEND )
  /* 
     ��ָ����TRANSMITAPP_SEND_DELAYʱ�䵽�ں󴥷�
     ���ݷ����¼�TRANSMITAPP_SEND_MSG_EVT
  */
  osal_start_timerEx( TransmitApp_TaskID,
                      TRANSMITAPP_SEND_MSG_EVT, TRANSMITAPP_SEND_DELAY );

/* ��δ������ʱ���ͣ�ʹ��Э������·������*/
#else
  /* ���̴������ݷ����¼�TRANSMITAPP_SEND_MSG_EVT */
  osal_set_event( TransmitApp_TaskID, TRANSMITAPP_SEND_MSG_EVT );
#endif
}


/*********************************************************************
 * �������ƣ�TransmitApp_DisplayResults
 * ��    �ܣ�������ʾ���
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void TransmitApp_DisplayResults( void )
{
#ifdef LCD_SUPPORTED
  #define LCD_W  21
  uint32 rxShdw, txShdw, tmp;
  byte lcd_buf[LCD_W+1];
  byte idx;
#endif
  
  /* ����ʵ�ʵ�ͳ��ʱ�� */
  uint32 msecs = osal_GetSystemClock() - clkShdw;
  clkShdw = osal_GetSystemClock();

  rxTotal += rxAccum;  // �������������ۼ�
  txTotal += txAccum;  // �������������ۼ�

/* ��������LCD_SUPPORTED����ѡ�����LCD����ʾͳ����Ϣ */
#if defined ( LCD_SUPPORTED )
  /*
     ����ÿ���ӽ��յ���ƽ�������������ֽ�Ϊ��λ���������������
  */
  rxShdw = (rxAccum * 1000 + msecs/2) / msecs;
 
  /*
     ����ÿ���ӷ��ͳ�ȥ��ƽ�������������ֽ�Ϊ��λ���������������
  */    
  txShdw = (txAccum * 1000 + msecs/2) / msecs;

  /* ��ʾ��������ʼ��*/  
  osal_memset( lcd_buf, ' ', LCD_W );
  lcd_buf[LCD_W] = NULL;
  
  /* ��ʾÿ����յ����������Լ��ۼƽ��յ��������������ֽ�Ϊ��λ�� */
  idx = 4;
  tmp = (rxShdw >= 100000) ? 99999 : rxShdw;
  
  /* ÿ����յ��������� */
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  idx = LCD_W-1;
  tmp = rxTotal;
  
  /* �ۼƽ��յ��������� */
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );
  
  /* ��LCD����ʾ */
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_4 );
  osal_memset( lcd_buf, ' ', LCD_W ); 

 /* ��ʾÿ�뷢�ͳ�ȥ���������Լ��ۼƷ��ͳ�ȥ�������������ֽ�Ϊ��λ�� */ 
  idx = 4;
  tmp = (txShdw >= 100000) ? 99999 : txShdw;
  
  /* ÿ�뷢�ͳ�ȥ�������� */
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  idx = LCD_W-1;
  tmp = txTotal;
  
  /* �ۼƷ��ͳ�ȥ��������*/
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );
  /* ��LCD����ʾ */
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_5 );

/* ��δ������LCD_SUPPORTED����ѡ���Ӵ������ͳ����Ϣ */
#else
  /* ע�⣺��ʾ��Ŀ����LCD����ʾʱ������ */  
  DEBUG_INFO( COMPID_APP, SEVERITY_INFORMATION, 3, rxAccum,
                                              (uint16)msecs, (uint16)rxTotal );
#endif

  /* �����ϴ���ʾ/���µ����ڽ��պͷ������ݵ�������Ϊ0 */
  if ( (rxAccum == 0) && (txAccum == 0) )
  {
    /* ֹͣ������ʾ�¼��Ĵ��� */
    osal_stop_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT );
    timerOn = FALSE;  // ������������������ʾ�¼��ĳ�ʱ��ʱ��״̬����ΪFALSE
  }

  /* ������ϴ���ʾ/���µ����ڽ���/�������ݵ����� */
  rxAccum = txAccum = 0;
}


