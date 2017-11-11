/*******************************************************************************
 * �ļ����ƣ�simplemeter.c
 * ��    �ܣ�SE������Դʵ�顣
 * ʵ�����ݣ�ESP(��Դ�����Ż�)����һ��ZigBeePRO��·�������豸������������IPD(
 *           Ԥװ��ʾ�豸)��PCT(�ɱ��ͨ���¿���)�ȷ��ָ����磬���Ҽ��뵽�������С�
 *           ESP(��Դ�����Ż�)��ͨ�����߷�ʽ���������豸��
 * ʵ���豸��SK-SmartRF05BB��װ����SK-CC2530EM��
 *           RIGHT����            ��������
 *
 *           D1(��ɫ)��      �豸�ɹ�������1���������˳���1��Ϩ��
 *           D3(��ɫ)��      ���ɹ�������������������LED����
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "ZDApp.h"
#include "AddrMgr.h"

#include "se.h"
#include "simplemeter.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
/********************************************************************/


/* �궨�� */
/********************************************************************/

/*
   ��ǰ��ǿ�Ʊ��������б���û������
*/
#define zcl_MandatoryReportableAttribute( a ) ( a == NULL )

/* ���� */
/********************************************************************/
/* 
  �򵥼�������С�������� 
 */
#define SIMPLEMETER_MIN_REPORTING_INTERVAL       5

/* ���ر��� */
/********************************************************************/
static uint8 simpleMeterTaskID;                       // esp����ϵͳ����ID
static uint8 simpleMeterTransID;                      // ����id
static devStates_t simpleMeterNwkState;               // ����״̬����
static afAddrType_t ESPAddr;                          // ESPĿ�ĵ�ַ
static zclReportCmd_t *pReportCmd;                    // ��������ṹ
static uint8 numAttr = 1;                             // �����е�������
extern uint8 simpleMeterCurrentSummationDelivered[];  // ��simplemeter_data.c�ļ��б�����
static uint8 linkKeyStatus;                           // �ӻ�ȡ������Կ�������ص�״̬����


/* ���غ��� */
/********************************************************************/
static void simplemeter_HandleKeys( uint8 shift, uint8 keys );

#if defined (ZCL_KEY_ESTABLISH)
static uint8 simplemeter_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif 

#if defined ( ZCL_ALARMS )
static void simplemeter_ProcessAlarmCmd( uint8 srcEP, afAddrType_t *dstAddr,
                        uint16 clusterID, zclFrameHdr_t *hdr, uint8 len, uint8 *data );
#endif 

static void simplemeter_ProcessIdentifyTimeChange( void );
/********************************************************************/


/* ���غ��� - Ӧ�ûص����� */
/********************************************************************/
/*
   �����ص�����
*/
static uint8 simplemeter_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

/*
   ͨ�ôػص�����
*/
static void simplemeter_BasicResetCB( void );
static void simplemeter_IdentifyCB( zclIdentify_t *pCmd );
static void simplemeter_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void simplemeter_AlarmCB( zclAlarm_t *pAlarm );

/*
   SE�ص�����
*/
static void simplemeter_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum );
static void simplemeter_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum );
static void simplemeter_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void simplemeter_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum );
static void simplemeter_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void simplemeter_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum );

/* ����ZCL�������� - ��������/��Ӧ��Ϣ */
/********************************************************************/
static void simplemeter_ProcessZCLMsg( zclIncomingMsg_t *msg );

#if defined ( ZCL_READ )
static uint8 simplemeter_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif 

#if defined ( ZCL_WRITE )
static uint8 simplemeter_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif

#if defined ( ZCL_REPORT )
static uint8 simplemeter_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg );
static uint8 simplemeter_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 simplemeter_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg );
static uint8 simplemeter_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 simplemeter_ProcessInReportCmd( zclIncomingMsg_t *pInMsg );
#endif 

static uint8 simplemeter_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

#if defined ( ZCL_DISCOVER )
static uint8 simplemeter_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif 


/*
   ZCLͨ�ôػص���
*/
static zclGeneral_AppCallbacks_t simplemeter_GenCmdCallbacks =
{
  simplemeter_BasicResetCB,      // �����Ĵظ�λ����
  simplemeter_IdentifyCB,        // ʶ������
  simplemeter_IdentifyQueryRspCB,// ʶ���ѯ��Ӧ����
  NULL,                          // ���ش�����
  NULL,                          // ��������뿪����
  NULL,                          // ��������ƶ�����
  NULL,                          // ������Ƶ�������
  NULL,                          // �������ֹͣ����
  NULL,                          // ����Ӧ����
  NULL,                          // ����������������
  NULL,                          // �����ص�����������
  NULL,                          // ������Ӧ����
  simplemeter_AlarmCB,           // ����(��Ӧ)����
  NULL,                          // RSSI��λ����
  NULL,                          // RSSI��λ��Ӧ����
};

/*
   ZCL SE�ػص���
*/
static zclSE_AppCallbacks_t simplemeter_SECmdCallbacks =			
{
  simplemeter_GetProfileCmdCB,   // ��ȡ Profile ����
  simplemeter_GetProfileRspCB,   // ��ȡ Profile ��Ӧ
  simplemeter_ReqMirrorCmdCB,    // ����������
  simplemeter_ReqMirrorRspCB,    // ��������Ӧ
  simplemeter_MirrorRemCmdCB,    // �����Ƴ�����
  simplemeter_MirrorRemRspCB,    // �����Ƴ���Ӧ
  NULL,                          // ��ȡ��ǰ�۸�
  NULL,                          // ��ȡԤ���۸�
  NULL,                          // �����۸�
  NULL,                          // ��ʾ��Ϣ����
  NULL,                          // ȡ����Ϣ����
  NULL,                          // ��ȡ���һ����Ϣ����
  NULL,                          // ��Ϣȷ��
  NULL,                          // ���ؿ����¼�
  NULL,                          // �˳����ؿ����¼�
  NULL,                          // �˳����и��ؿ����¼�
  NULL,                          // �����¼�״̬
  NULL,                          // ��ȡԤ���¼�
};


/*********************************************************************
 * �������ƣ�rangeext_Init
 * ��    �ܣ��������ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void simplemeter_Init( uint8 task_id )
{
  simpleMeterTaskID = task_id;
  simpleMeterNwkState = DEV_INIT;
  simpleMeterTransID = 0;

  // ����ESPĿ�ĵ�ַ
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = SIMPLEMETER_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  /* ע��һ��SE�˵� */
  zclSE_Init( &simpleMeterSimpleDesc );

  /* ע��ZCLͨ�ôؿ�ص����� */
  zclGeneral_RegisterCmdCallbacks( SIMPLEMETER_ENDPOINT, &simplemeter_GenCmdCallbacks );

  // Register the ZCL SE Cluster Library callback functions
  zclSE_RegisterCmdCallbacks( SIMPLEMETER_ENDPOINT, &simplemeter_SECmdCallbacks );
  /* ע��Ӧ�ó��������б� */
  zcl_registerAttrList( SIMPLEMETER_ENDPOINT, SIMPLEMETER_MAX_ATTRIBUTES, simpleMeterAttrs );

  /* ע��Ӧ�ó����ѡ���б� */
  zcl_registerClusterOptionList( SIMPLEMETER_ENDPOINT, SIMPLEMETER_MAX_OPTIONS, simpleMeterOptions );

  /* ע��Ӧ�ó�������������֤�ص����� */
  zcl_registerValidateAttrData( simplemeter_ValidateAttrDataCB );

  /* ע��Ӧ�ó���������δ�����������/��Ӧ��Ϣ */
  zcl_registerForMsg( simpleMeterTaskID );

  /* ע�����а����¼� - ���������а����¼� */
  RegisterForKeys( simpleMeterTaskID );

  /* ������ʱ����ͬ���򵥼������Ͳ���ϵͳʱ�� */
  osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_UPDATE_TIME_EVT, 
                      SIMPLEMETER_UPDATE_TIME_PERIOD );

  /* Ϊ�򵥼�������������ID */
  pReportCmd = (zclReportCmd_t *)osal_mem_alloc( sizeof( zclReportCmd_t ) + 
                                      ( numAttr * sizeof( zclReport_t ) ) );
  if ( pReportCmd != NULL )
  {
    pReportCmd->numAttr = numAttr;

    /* ������������� */
    pReportCmd->attrList[0].attrID = ATTRID_SE_CURRENT_SUMMATION_DELIVERED;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT48;
    pReportCmd->attrList[0].attrData = simpleMeterCurrentSummationDelivered;

    /* ����ʣ������ */
  }
}


/*********************************************************************
 * �������ƣ�simplemeter_event_loop
 * ��    �ܣ��򵥼������������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
uint16 simplemeter_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( simpleMeterTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          /* ����ZCL��������/��Ӧ��Ϣ */
          simplemeter_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          /* �������� */
          simplemeter_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          simpleMeterNwkState = (devStates_t)(MSGpkt->hdr.status);

          if (ZG_SECURE_ENABLED)
          {
            if ( (simpleMeterNwkState == DEV_ROUTER) || (simpleMeterNwkState == DEV_END_DEVICE) )
            {
              /* ���������Կ�Ƿ��Ѿ������� */
              linkKeyStatus = simplemeter_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

              if (linkKeyStatus != ZSuccess)
              {
                /* ������Կ�������� */
                osal_set_event( simpleMeterTaskID, SIMPLEMETER_KEY_ESTABLISHMENT_REQUEST_EVT);

              }
              else
              {
                /* ������Կ�Ѿ��������������ͱ��� */
                osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_REPORT_ATTRIBUTE_EVT, 
                                    SIMPLEMETER_REPORT_PERIOD );
              }
            }
          }
          else
          {
            osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_REPORT_ATTRIBUTE_EVT, 
                                SIMPLEMETER_REPORT_PERIOD );
          }

          /*����SE�ն��豸��ѯ����*/
          NLME_SetPollRate ( SE_DEVICE_POLL_RATE ); 
          break;

#if defined( ZCL_KEY_ESTABLISH )
        case ZCL_KEY_ESTABLISH_IND:
          osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_REPORT_ATTRIBUTE_EVT, 
                              SIMPLEMETER_REPORT_PERIOD );
          break;
#endif

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt ); // �ͷŴ洢��
    }

    return (events ^ SYS_EVENT_MSG); // ����δ������¼�
  }

  /* ��ʼ����Կ���������¼� */
  if ( events & SIMPLEMETER_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(simpleMeterTaskID, 
                                          &ESPAddr, simpleMeterTransID);

    return ( events ^ SIMPLEMETER_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  /* ���ͱ��������¼� */ 
  if ( events & SIMPLEMETER_REPORT_ATTRIBUTE_EVT )
  {
    if ( pReportCmd != NULL )
    {
      zcl_SendReportCmd( SIMPLEMETER_ENDPOINT, &ESPAddr,
                         ZCL_CLUSTER_ID_SE_SIMPLE_METERING, pReportCmd,
                         ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 );

      osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_REPORT_ATTRIBUTE_EVT,
                          SIMPLEMETER_REPORT_PERIOD );
    }

    return ( events ^ SIMPLEMETER_REPORT_ATTRIBUTE_EVT );
  }

  /* ��һ��ʶ���������ʶ��ʱ�¼����� */
  if ( events & SIMPLEMETER_IDENTIFY_TIMEOUT_EVT )
  {
    if ( simpleMeterIdentifyTime > 0 )
    {
      simpleMeterIdentifyTime--;
    }
    simplemeter_ProcessIdentifyTimeChange();

    return ( events ^ SIMPLEMETER_IDENTIFY_TIMEOUT_EVT );
  }

  /* ��ȡ��ǰʱ���¼� */ 
  if ( events & SIMPLEMETER_UPDATE_TIME_EVT )
  {
    simpleMeterTime = osal_getClock();
    osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_UPDATE_TIME_EVT, 
                        SIMPLEMETER_UPDATE_TIME_PERIOD );

    return ( events ^ SIMPLEMETER_UPDATE_TIME_EVT );
  }

  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�simplemeter_ProcessIdentifyTimeChange
 * ��    �ܣ�������˸LED��ָ��ʶ��ʱ������ֵ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_ProcessIdentifyTimeChange( void )
{
  if ( simpleMeterIdentifyTime > 0 )
  {
    osal_start_timerEx( simpleMeterTaskID, SIMPLEMETER_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( simpleMeterTaskID, SIMPLEMETER_IDENTIFY_TIMEOUT_EVT );
  }
}


#if defined ( ZCL_KEY_ESTABLISH )
/*********************************************************************
 * �������ƣ�simplemeter_KeyEstablish_ReturnLinkKey
 * ��    �ܣ���ȡ���������Կ������
 * ��ڲ�����shortAddr �̵�ַ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
{
  APSME_LinkKeyData_t* keyData;
  uint8 status = ZFailure;
  AddrMgrEntry_t entry;

  /* �����豸����ַ */
  entry.user = ADDRMGR_USER_DEFAULT;
  entry.nwkAddr = shortAddr;

  if ( AddrMgrEntryLookupNwk( &entry ) )
  {
    /* ���APS������Կ���� */
    APSME_LinkKeyDataGet( entry.extAddr, &keyData );

    if ( (keyData != NULL) && (keyData->key != NULL) )
    {
      status = ZSuccess;
    }
  }
  else
  {
    status = ZInvalidParameter; // δ֪�豸
  }

  return status;
}
#endif


/*********************************************************************
 * �������ƣ�simplemeter_HandleKeys
 * ��    �ܣ��򵥼������İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_2     SK-SmartRF10BB��K2��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_2 )
  {
    ZDOInitDevice(0); // ��������
  }
}


/*********************************************************************
 * �������ƣ�simplemeter_ValidateAttrDataCB
 * ��    �ܣ����Ϊ�����ṩ������ֵ�Ƿ��ڸ�����ָ���ķ�Χ�ڡ�
 * ��ڲ�����pAttr     ָ�����Ե�ָ��
 *           pAttrInfo ָ��������Ϣ��ָ��            
 * ���ڲ�������
 * �� �� ֵ��TRUE      ��Ч����
 *           FALSE     ��Ч����
 ********************************************************************/
static uint8 simplemeter_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
{
  uint8 valid = TRUE;

  switch ( pAttrInfo->dataType )
  {
    case ZCL_DATATYPE_BOOLEAN:
      if ( ( *(pAttrInfo->attrData) != 0 ) && ( *(pAttrInfo->attrData) != 1 ) )
        valid = FALSE;
      break;

    default:
      break;
  }

  return ( valid );
}


/*********************************************************************
 * �������ƣ�simplemeter_BasicResetCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص��������������дص���������Ϊ����Ĭ�����á�
 * ��ڲ�������          
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_BasicResetCB( void )
{
  // �ڴ���Ӵ���
}


/*********************************************************************
 * �������ƣ�simplemeter_IdentifyCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ������󱻵��á�
 * ��ڲ�����pCmd      ָ��ʶ������ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_IdentifyCB( zclIdentify_t *pCmd )
{
  simpleMeterIdentifyTime = pCmd->identifyTime;
  simplemeter_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * �������ƣ�simplemeter_IdentifyQueryRspCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ���ѯ����
 *           �󱻵��á�
 * ��ڲ�����pRsp    ָ��ʶ���ѯ��Ӧ�ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_AlarmCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó��򱨾���������
 *           �󱻵��á�
 * ��ڲ�����pAlarm   ָ�򱨾�����ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_AlarmCB( zclAlarm_t *pAlarm )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_GetProfileCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ��ȡ�����ļ��󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ�����ļ��ṹ��ָ��     
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )

  /* һ��ִ�л�ȡ�����ļ��������װ��Ӧ���ͻ�ȡ�����ļ���Ӧ�ص� */

  /* ����ı�������ʼ��Ϊ������ */
  /* ��ʵ��Ӧ���У��û�Ӧ������ pCmd->endTime ָ�������ڵ����ݱ�����
     ���ҷ�����Ӧ������ */

  uint32 endTime;
  uint8  status = zclSE_SimpleMeter_GetProfileRsp_Status_Success;
  uint8  profileIntervalPeriod = PROFILE_INTERVAL_PERIOD_60MIN;
  uint8  numberOfPeriodDelivered = 5;
  uint24 intervals[] = {0xa00001, 0xa00002, 0xa00003, 0xa00004, 0xa00005};

  // endTime: 32λֵ����ʾ��ʱ����˳��������������ʱ�䡣
  // �ٸ�����: ������2ʱ������3:00�ռ��������ݽ���Ϊ����3:00���䣨����ʱ�䣩
  // ���ص�ʱ������Ӧ���������Ľ���ʱ����ͬ���߱������е�pCmd->endTimeʱ�����

  // �������ʱ��ֵΪ0xFFFFFFFF ��ʾ�������齫������
  // �������� - ����������Ŀ����ʱ��ͬ������ͬ��

  endTime = pCmd->endTime;

  // ���ͻ�ȡ������Ӧ����ص�
  zclSE_SimpleMetering_Send_GetProfileRsp( SIMPLEMETER_ENDPOINT, srcAddr, endTime,
                                           status,
                                           profileIntervalPeriod,
                                           numberOfPeriodDelivered, intervals,
                                           false, seqNum );
#endif 
}


/*********************************************************************
 * �������ƣ�simplemeter_GetProfileRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ��ȡ�����ļ���Ӧ�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ�����ļ���Ӧ�ṹ��ָ��     
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_ReqMirrorCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�����������󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_ReqMirrorRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���������Ӧ�󱻵��á�
 * ��ڲ�����pRsp     ָ����������Ӧ�ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pRsp, afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_MirrorRemCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ��������Ƴ�����󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�simplemeter_MirrorRemRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ��������Ƴ���Ӧ�󱻵��á�
 * ��ڲ�����pCmd     ָ�����Ƴ���Ӧ�ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd, 
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/* 
  ����ZCL������������/��Ӧ��Ϣ
 */
/*********************************************************************
 * �������ƣ�simplemeter_ProcessZCLMsg
 * ��    �ܣ�����ZCL����������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void simplemeter_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      simplemeter_ProcessInReadRspCmd( pInMsg );
      break;
#endif 

#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      simplemeter_ProcessInWriteRspCmd( pInMsg );
      break;
#endif 

#if defined ( ZCL_REPORT )
    case ZCL_CMD_CONFIG_REPORT:
      simplemeter_ProcessInConfigReportCmd( pInMsg );
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      simplemeter_ProcessInConfigReportRspCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      simplemeter_ProcessInReadReportCfgCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      simplemeter_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      simplemeter_ProcessInReportCmd( pInMsg );
      break;
#endif

    case ZCL_CMD_DEFAULT_RSP:
      simplemeter_ProcessInDefaultRspCmd( pInMsg );
      break;
#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      simplemeter_ProcessInDiscRspCmd( pInMsg );
      break;
#endif 

    default:
      break;
  }

  if ( pInMsg->attrCmd != NULL )
  {
    /* �ͷŴ洢������ */
    osal_mem_free( pInMsg->attrCmd );
    pInMsg->attrCmd = NULL;
  }
}


#if defined ( ZCL_READ )
/*********************************************************************
 * �������ƣ�simplemeter_ProcessInReadRspCmd
 * ��    �ܣ����������ļ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // �ڴ���Ӵ���
  }

  return TRUE;
}
#endif  


#if defined ( ZCL_WRITE )
/*********************************************************************
 * �������ƣ�simplemeter_ProcessInWriteRspCmd
 * ��    �ܣ����������ļ�д��Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // �ڴ���Ӵ���
  }

  return TRUE;
}
#endif


#if defined ( ZCL_REPORT )
/*********************************************************************
 * �������ƣ�simplemeter_ProcessInConfigReportCmd
 * ��    �ܣ����������ļ����ñ��������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TURE            �������б����ҵ�����
 *           FALSE           δ�ҵ�
 ********************************************************************/
static uint8 simplemeter_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportCmd_t *cfgReportCmd;
  zclCfgReportRec_t *reportRec;
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 status;
  uint8 i, j = 0;

  cfgReportCmd = (zclCfgReportCmd_t *)pInMsg->attrCmd;

  /* Ϊ��Ӧ�������ռ� */
  cfgReportRspCmd = (zclCfgReportRspCmd_t *)osal_mem_alloc( sizeof ( zclCfgReportRspCmd_t ) + \
                                        sizeof ( zclCfgReportStatus_t) * cfgReportCmd->numAttr );
  if ( cfgReportRspCmd == NULL )
    return FALSE; // EMBEDDED RETURN

  /* ����ÿ���������ü�¼���� */
  for ( i = 0; i < cfgReportCmd->numAttr; i++ )
  {
    reportRec = &(cfgReportCmd->attrList[i]);

    status = ZCL_STATUS_SUCCESS;

    if ( zclFindAttrRec( SIMPLEMETER_ENDPOINT, pInMsg->clusterId, reportRec->attrID, &attrRec ) )
    {
      if ( reportRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        if ( reportRec->dataType == attrRec.attr.dataType )
        {
          /* ������������� */ 
          if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
          {
            if ( reportRec->minReportInt < SIMPLEMETER_MIN_REPORTING_INTERVAL ||
                 ( reportRec->maxReportInt != 0 &&
                   reportRec->maxReportInt < reportRec->minReportInt ) )
            {
              /* ��Ч�ֶ� */
              status = ZCL_STATUS_INVALID_VALUE;
            }
            else
            {
              /* 
	        ������С����󱨸������Ҹı䱨��״̬Ϊ 
		zclSetAttrReportInterval( pAttr, cfgReportCmd )
	       */
              status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE; // for now
            }
          }
          else
          {
            /* ���Բ��ܱ����� */
            status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;
          }
        }
        else
        {
          /* �����������Ͳ���ȷ */
          status = ZCL_STATUS_INVALID_DATA_TYPE;
        }
      }
      else
      {
        /* Ԥ�ȱ�������ֵ */
        if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
        {
          /* ���ó�ʱ���� */
          /* status = zclSetAttrTimeoutPeriod( pAttr, cfgReportCmd ); */
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE; // for now
        }
        else
        {
          /* ���Ա��治�ܱ����� */
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
      }
    }
    else
    {
      /* ���Բ�֧�� */
      status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    /* ������ɹ�����¼״̬ */
    if ( status != ZCL_STATUS_SUCCESS )
    {
      cfgReportRspCmd->attrList[j].status = status;
      cfgReportRspCmd->attrList[j++].attrID = reportRec->attrID;
    }
  }  

  if ( j == 0 )
  {
    /* 
      һ���������Ա��ɹ����ã���������Ӧ�����е�״̬�ֶεĵ�������״̬��¼
      �������ó�SUCCESS���Ҹ�����ID�ֶα�ʡ��
     */
    cfgReportRspCmd->attrList[0].status = ZCL_STATUS_SUCCESS;
    cfgReportRspCmd->numAttr = 1;
  }
  else
  {
    cfgReportRspCmd->numAttr = j;
  }

  /* �ط���Ӧ */
  zcl_SendConfigReportRspCmd( SIMPLEMETER_ENDPOINT, &(pInMsg->srcAddr),
                              pInMsg->clusterId, cfgReportRspCmd, 
                              ZCL_FRAME_SERVER_CLIENT_DIR,
                              true, pInMsg->zclHdr.transSeqNum );

  osal_mem_free( cfgReportRspCmd );

  return TRUE ;
}


/*********************************************************************
 * �������ƣ�simplemeter_ProcessInConfigReportRspCmd
 * ��    �ܣ����������ļ����ñ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TURE            �ɹ�
 *           FALSE           δ�ɹ�
 ********************************************************************/
static uint8 simplemeter_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 i;

  cfgReportRspCmd = (zclCfgReportRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < cfgReportRspCmd->numAttr; i++)
  {
    if ( zclFindAttrRec( SIMPLEMETER_ENDPOINT, pInMsg->clusterId,
                         cfgReportRspCmd->attrList[i].attrID, &attrRec ) )
    {
      // �ڴ���Ӵ���
    }
  }

  return TRUE;
}


/*********************************************************************
 * �������ƣ�simplemeter_ProcessInReadReportCfgCmd
 * ��    �ܣ����������ļ���ȡ�������������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadReportCfgCmd_t *readReportCfgCmd;
  zclReadReportCfgRspCmd_t *readReportCfgRspCmd;
  zclReportCfgRspRec_t *reportRspRec;
  zclAttrRec_t attrRec;
  uint8 reportChangeLen;
  uint8 *dataPtr;
  uint8 hdrLen;
  uint8 dataLen = 0;
  uint8 status;
  uint8 i;

  readReportCfgCmd = (zclReadReportCfgCmd_t *)pInMsg->attrCmd;

  /* �õ���Ӧ����(�ı�ɱ����ֶ�Ϊ��Ч����) */
  for ( i = 0; i < readReportCfgCmd->numAttr; i++ )
  {
    /* ����֧������Ϊģ���������ͣ��õ��ɱ����ֶγ��� */
    if ( zclFindAttrRec( SIMPLEMETER_ENDPOINT, pInMsg->clusterId,
                         readReportCfgCmd->attrList[i].attrID, &attrRec ) )
    {
      if ( zclAnalogDataType( attrRec.attr.dataType ) )
      {
         reportChangeLen = zclGetDataTypeLength( attrRec.attr.dataType );

         /* �����Ҫ���������� */
         if ( PADDING_NEEDED( reportChangeLen ) )
           reportChangeLen++;

         dataLen += reportChangeLen;
      }
    }
  }

  hdrLen = sizeof( zclReadReportCfgRspCmd_t ) + 
                  ( readReportCfgCmd->numAttr * sizeof( zclReportCfgRspRec_t ) );

  /* Ϊ��Ӧ�������ռ� */
  readReportCfgRspCmd = (zclReadReportCfgRspCmd_t *)osal_mem_alloc( hdrLen + dataLen );
  if ( readReportCfgRspCmd == NULL )
    return FALSE;  

  dataPtr = (uint8 *)( (uint8 *)readReportCfgRspCmd + hdrLen );
  readReportCfgRspCmd->numAttr = readReportCfgCmd->numAttr;
  for (i = 0; i < readReportCfgCmd->numAttr; i++)
  {
    reportRspRec = &(readReportCfgRspCmd->attrList[i]);

    if ( zclFindAttrRec( SIMPLEMETER_ENDPOINT, pInMsg->clusterId,
                         readReportCfgCmd->attrList[i].attrID, &attrRec ) )
    {
      if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
      {
        /* ��ȡ�������� */
        /* status = zclReadReportCfg( readReportCfgCmd->attrID[i], reportRspRec ); */
        status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;  
        if ( status == ZCL_STATUS_SUCCESS && zclAnalogDataType( attrRec.attr.dataType ) )
        {
          reportChangeLen = zclGetDataTypeLength( attrRec.attr.dataType );
          reportRspRec->reportableChange = dataPtr;

          /* �����Ҫ���������� */
          if ( PADDING_NEEDED( reportChangeLen ) )
            reportChangeLen++;
          dataPtr += reportChangeLen;
        }
      }
      else
      {
        /* ��ǿ�ƿɱ��������б���û������ */
        status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;
      }
    }
    else
    {
      /* ����δ�ҵ� */
      status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    reportRspRec->status = status;
    reportRspRec->attrID = readReportCfgCmd->attrList[i].attrID;
  }

  /* �ط���Ӧ */
  zcl_SendReadReportCfgRspCmd( SIMPLEMETER_ENDPOINT, &(pInMsg->srcAddr),
                               pInMsg->clusterId, readReportCfgRspCmd, 
                               ZCL_FRAME_SERVER_CLIENT_DIR,
                               true, pInMsg->zclHdr.transSeqNum );

  osal_mem_free( readReportCfgRspCmd );

  return TRUE;
}


/*********************************************************************
 * �������ƣ�simplemeter_ProcessInReadReportCfgRspCmd
 * ��    �ܣ����������ļ���ȡ����������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadReportCfgRspCmd_t *readReportCfgRspCmd;
  zclReportCfgRspRec_t *reportRspRec;
  uint8 i;

  readReportCfgRspCmd = (zclReadReportCfgRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < readReportCfgRspCmd->numAttr; i++ )
  {
    reportRspRec = &(readReportCfgRspCmd->attrList[i]);

    /* ֪ͨԭʼ���������������豸 */
    if ( reportRspRec->status == ZCL_STATUS_SUCCESS )
    {
      if ( reportRspRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        // �û��ڴ���Ӵ���
      }
      else
      {
        // �û��ڴ���Ӵ���
      }
    }
  }

  return TRUE;
}

/*********************************************************************
 * �������ƣ�simplemeter_ProcessInReportCmd
 * ��    �ܣ����������ļ����������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 simplemeter_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclReportCmd_t *reportCmd;
  uint8 i;

  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // �û��ڴ���Ӵ���
  }

  return TRUE;
}
#endif


/*********************************************************************
 * �������ƣ�simplemeter_ProcessInDefaultRspCmd
 * ��    �ܣ����������ļ�Ĭ����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 simplemeter_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  return TRUE;
}


#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * �������ƣ�simplemeter_ProcessInDiscRspCmd
 * ��    �ܣ����������ļ�������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 simplemeter_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // �豸�����Է�����������֪ͨ
  }

  return TRUE;
}
#endif 
