/*******************************************************************************
 * �ļ����ƣ�pct.c
 * ��    �ܣ�SE������Դʵ�顣
 * ʵ�����ݣ�ESP(��Դ�����Ż�)����һ��ZigBeePRO��·�������豸������������IPD(
 *           Ԥװ��ʾ�豸)��PCT(�ɱ��ͨ���¿���)�ȷ��ָ����磬���Ҽ��뵽�������С�
 *           ESP(��Դ�����Ż�)��ͨ�����߷�ʽ���������豸��
 * ʵ���豸��SK-SmartRF05EB��װ����SK-CC2530EM��
 *           UP����          ��������
 *
 *           LED1(��ɫ)��    ���͸��ؿ����¼�ʱ��˸ 
 *           LED3(��ɫ)��    ���ɹ�������������������LED����
 *           LED4(��ɫ)��    δ����������˸�����������Ϩ��
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "ZDApp.h"
#include "AddrMgr.h"

#include "se.h"
#include "pct.h"
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


/* ���ر��� */
/********************************************************************/
static uint8 pctTaskID;                // PCT�ɱ��ͨ���¿�������ϵͳ����ID
static devStates_t pctNwkState;        // ����״̬����
static uint8 pctTransID;               // ����id
static afAddrType_t ESPAddr;           // ESPĿ�ĵ�ַ
static uint8 linkKeyStatus;            // �ӻ�ȡ������Կ�������ص�״̬����
static zclCCReportEventStatus_t rsp;   // ����ѡ���ֶ�


/* ���غ��� */
/********************************************************************/
static void pct_HandleKeys( uint8 shift, uint8 keys );

#if defined (ZCL_KEY_ESTABLISH)
static uint8 pct_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif 

#if defined ( ZCL_ALARMS )
static void pct_ProcessAlarmCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                 uint16 clusterID, zclFrameHdr_t *hdr,
                                 uint8 len, uint8 *data );
#endif 

static void pct_ProcessIdentifyTimeChange( void );
/********************************************************************/


/* ���غ��� - Ӧ�ûص����� */
/********************************************************************/
/*
   �����ص�����
*/
static uint8 pct_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

/*
   ͨ�ôػص�����
*/
static void pct_BasicResetCB( void );
static void pct_IdentifyCB( zclIdentify_t *pCmd );
static void pct_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void pct_AlarmCB( zclAlarm_t *pAlarm );

/*
   SE�ص�����
*/
static void pct_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 status, uint8 seqNum);
static void pct_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum );
static void pct_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum);
static void pct_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum );
static void pct_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum);


/* ����ZCL�������� - ��������/��Ӧ��Ϣ */
/********************************************************************/
static void pct_ProcessZCLMsg( zclIncomingMsg_t *msg );

#if defined ( ZCL_READ )
static uint8 pct_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif

#if defined ( ZCL_WRITE )
static uint8 pct_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif 

static uint8 pct_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

#if defined ( ZCL_DISCOVER )
static uint8 pct_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif 
/********************************************************************/


/*
   ZCLͨ�ôػص���
*/
static zclGeneral_AppCallbacks_t pct_GenCmdCallbacks =
{
  pct_BasicResetCB,              // �����ظ�λ����
  pct_IdentifyCB,                // ʶ������
  pct_IdentifyQueryRspCB,        // ʶ���ѯ��Ӧ����
  NULL,                          // ���ش�����
  NULL,                          // ��������뿪����
  NULL,                          // ��������ƶ�����
  NULL,                          // ������Ƶ�������
  NULL,                          // �������ֹͣ����
  NULL,                          // ����Ӧ����
  NULL,                          // ����������������
  NULL,                          // �����ص�����������
  NULL,                          // ������Ӧ����
  pct_AlarmCB,                   // ����(��Ӧ)����
  NULL,                          // RSSI��λ����
  NULL,                          // RSSI��λ��Ӧ����
};

/*
   ZCL SE�ػص���
*/
static zclSE_AppCallbacks_t pct_SECmdCallbacks =			
{
  NULL,                              // ��ȡ Profile ����
  NULL,                              // ��ȡ Profile ��Ӧ
  NULL,                              // ����������
  NULL,                              // ��������Ӧ
  NULL,                              // �����Ƴ�����
  NULL,                              // �����Ƴ���Ӧ
  NULL,                              // ��ȡ��ǰ�۸�
  NULL,                              // ��ȡԤ���۸�
  NULL,                              // �����۸�
  NULL,                              // ��ʾ��Ϣ����
  NULL,                              // ȡ����Ϣ����
  NULL,                              // ��ȡ���һ����Ϣ����
  NULL,                              // ��Ϣȷ��
  pct_LoadControlEventCB,            // ���ؿ����¼�
  pct_CancelLoadControlEventCB,      // �˳����ؿ����¼�
  pct_CancelAllLoadControlEventsCB,  // �˳����и��ؿ����¼�
  pct_ReportEventStatusCB,           // �����¼�״̬
  pct_GetScheduledEventCB,           // ��ȡԤ���¼�
};


/*********************************************************************
 * �������ƣ�pct_Init
 * ��    �ܣ�PCT�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void pct_Init( uint8 task_id )
{
  pctTaskID = task_id;

  // ����ESPĿ�ĵ�ַ
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = PCT_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  /* ע��һ��SE�˵� */
  zclSE_Init( &pctSimpleDesc );

  /* ע��ZCLͨ�ôؿ�ص����� */
  zclGeneral_RegisterCmdCallbacks( PCT_ENDPOINT, &pct_GenCmdCallbacks );

  /* ע��ZCL SE�ؿ�ص����� */
  zclSE_RegisterCmdCallbacks( PCT_ENDPOINT, &pct_SECmdCallbacks );

  /* ע��Ӧ�ó��������б� */
  zcl_registerAttrList( PCT_ENDPOINT, PCT_MAX_ATTRIBUTES, pctAttrs );

  /* ע��Ӧ�ó����ѡ���б� */
  zcl_registerClusterOptionList( PCT_ENDPOINT, PCT_MAX_OPTIONS, pctOptions );

  /* ע��Ӧ�ó�������������֤�ص����� */
  zcl_registerValidateAttrData( pct_ValidateAttrDataCB );

  /* ע��Ӧ�ó���������δ�����������/��Ӧ��Ϣ */
  zcl_registerForMsg( pctTaskID );

  /* ע�����а����¼� - ���������а����¼� */
  RegisterForKeys( pctTaskID );

  /* ������ʱ��ʹ�ò���ϵͳʱ����ͬ��LCD��ʱ�� */
  osal_start_timerEx( pctTaskID, PCT_UPDATE_TIME_EVT, PCT_UPDATE_TIME_PERIOD );
}


/*********************************************************************
 * �������ƣ�pct_event_loop
 * ��    �ܣ�PCT�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
uint16 pct_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( pctTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          /* ����ZCL��������/��Ӧ��Ϣ */
          pct_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          /* �������� */
          pct_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          pctNwkState = (devStates_t)(MSGpkt->hdr.status);

          if (ZG_SECURE_ENABLED)
          {
            if ( pctNwkState == DEV_END_DEVICE )
            {
              /* ���������Կ�Ƿ��Ѿ������� */
              linkKeyStatus = pct_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

              if (linkKeyStatus != ZSuccess)
              {
                /* ������Կ�������� */
                osal_set_event( pctTaskID, PCT_KEY_ESTABLISHMENT_REQUEST_EVT);
              }
            }
          }

          /* ����SE�ն��豸��ѯ���� */
          NLME_SetPollRate ( SE_DEVICE_POLL_RATE ); 
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt ); // �ͷŴ洢��
    }

    return (events ^ SYS_EVENT_MSG); // ����δ������¼�
  }

  /* ��ʼ����Կ���������¼� */
  if ( events & PCT_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(pctTaskID, &ESPAddr, pctTransID);

    return ( events ^ PCT_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  /* ��һ��ʶ���������ʶ��ʱ�¼����� */
  if ( events & PCT_IDENTIFY_TIMEOUT_EVT )
  {
    if ( pctIdentifyTime > 0 )
    {
      pctIdentifyTime--;
    }
    pct_ProcessIdentifyTimeChange();

    return ( events ^ PCT_IDENTIFY_TIMEOUT_EVT );
  }

  /* ��ȡ��ǰʱ���¼� */
  if ( events & PCT_UPDATE_TIME_EVT )
  {
    pctTime = osal_getClock();
    osal_start_timerEx( pctTaskID, PCT_UPDATE_TIME_EVT, PCT_UPDATE_TIME_PERIOD );

    return ( events ^ PCT_UPDATE_TIME_EVT );
  }

  /* ����PCT���ؿ���������¼� */ 
  if ( events & PCT_LOAD_CTRL_EVT )
  {
    /* 
      ����Ӧ����Ӧ
      DisableDefaultResponse ��־λ������Ϊ�� - ���ǽ����Ĭ����Ӧ��
      ��Ϊ�����¼�״̬������û��������Ӧ
     */

    rsp.eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_COMPLETED;
    zclSE_LoadControl_Send_ReportEventStatus( PCT_ENDPOINT, &ESPAddr,
                                            &rsp, false, pctTransID );
    /* LCD������ʾ��ɸ��¼� */ 
    HalLcdWriteString("PCT Evt Complete", HAL_LCD_LINE_6);
    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);

    return ( events ^ PCT_LOAD_CTRL_EVT );
  }

  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�pct_ProcessIdentifyTimeChange
 * ��    �ܣ�������˸LED��ָ��ʶ��ʱ������ֵ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_ProcessIdentifyTimeChange( void )
{
  if ( pctIdentifyTime > 0 )
  {
    osal_start_timerEx( pctTaskID, PCT_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( pctTaskID, PCT_IDENTIFY_TIMEOUT_EVT );
  }
}

#if defined (ZCL_KEY_ESTABLISH)
/*********************************************************************
 * �������ƣ�pct_KeyEstablish_ReturnLinkKey
 * ��    �ܣ���ȡ���������Կ������
 * ��ڲ�����shortAddr �̵�ַ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 pct_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
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
 * �������ƣ�pct_HandleKeys
 * ��    �ܣ�ESP�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_1     SK-SmartRF10BB��up��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_1 )
  {
    ZDOInitDevice(0); // ��������
  }
}


/*********************************************************************
 * �������ƣ�pct_ValidateAttrDataCB
 * ��    �ܣ����Ϊ�����ṩ������ֵ�Ƿ��ڸ�����ָ���ķ�Χ�ڡ�
 * ��ڲ�����pAttr     ָ�����Ե�ָ��
 *           pAttrInfo ָ��������Ϣ��ָ��            
 * ���ڲ�������
 * �� �� ֵ��TRUE      ��Ч����
 *           FALSE     ��Ч����
 ********************************************************************/
static uint8 pct_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
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
 * �������ƣ�pct_BasicResetCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص��������������дص���������Ϊ����Ĭ�����á�
 * ��ڲ�������          
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_BasicResetCB( void )
{
  // �ڴ���Ӵ���
}


/*********************************************************************
 * �������ƣ�pct_IdentifyCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ������󱻵��á�
 * ��ڲ�����pCmd      ָ��ʶ������ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_IdentifyCB( zclIdentify_t *pCmd )
{
  pctIdentifyTime = pCmd->identifyTime;
  pct_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * �������ƣ�pct_IdentifyQueryRspCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ���ѯ����
 *           �󱻵��á�
 * ��ڲ�����pRsp    ָ��ʶ���ѯ��Ӧ�ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�pct_AlarmCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó��򱨾���������
 *           �󱻵��á�
 * ��ڲ�����pAlarm   ָ�򱨾�����ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_AlarmCB( zclAlarm_t *pAlarm )
{
  // ����������û�����
}


#if defined (ZCL_LOAD_CONTROL)
/*********************************************************************
 * �������ƣ�pct_SendReportEventStatus
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ�������¼�����󱻵��á�
 * ��ڲ�����srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 *           eventID          ���¼���ID
 *           startTime        ���¼�����ʱ��
 *           eventStatus      �¼�״̬
 *           criticalityLevel ʱ���ٽ缶��
 *��������   eventControl �������¼����¼�����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_SendReportEventStatus( afAddrType_t *srcAddr, uint8 seqNum,
                                       uint32 eventID, uint32 startTime,
                                       uint8 eventStatus, uint8 criticalityLevel,
                                       uint8 eventControl )
{
  /* ǿ�����ֶ� - ʹ���������� */
  rsp.issuerEventID = eventID;
  rsp.eventStartTime = startTime;
  rsp.criticalityLevelApplied = criticalityLevel;
  rsp.eventControl = eventControl;
  rsp.eventStatus = eventStatus;
  rsp.signatureType = SE_PROFILE_SIGNATURE_TYPE_ECDSA;

  /* loadcontrol_Signature Ϊ��̬���飬����loadcontrol_data.c�ļ��и��� */
  osal_memcpy( rsp.signature, pctSignature, SE_PROFILE_SIGNATURE_LENGTH );

  /* ��ѡ���ֶ� - Ĭ��ʹ��δʹ��ֵ����� */
  rsp.coolingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
  rsp.heatingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
  rsp.averageLoadAdjustment = SE_OPTIONAL_FIELD_INT8;
  rsp.dutyCycleApplied = SE_OPTIONAL_FIELD_UINT8;

  /* 
    ����Ӧ����Ӧ
    DisableDefaultResponse ��־λ������Ϊ�� - ���ǽ����Ĭ����Ӧ��
    ��Ϊ�����¼�״̬������û��������Ӧ
   */
  zclSE_LoadControl_Send_ReportEventStatus( PCT_ENDPOINT, srcAddr,
                                            &rsp, false, seqNum );
}
#endif


/*********************************************************************
 * �������ƣ�pct_LoadControlEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ�����ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ���ؿ����¼�����
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 *           Status           �¼�״̬
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                                    afAddrType_t *srcAddr, uint8 status,
                                    uint8 seqNum)
{
#if defined ( ZCL_LOAD_CONTROL )
  /* 
    ����������Դ˵�����ڻ�ȡ�����ؿ����¼�������豸Ӧ���ط�
    �����¼�״̬����
   */  
  uint8 eventStatus;

  if ( status == ZCL_STATUS_INVALID_FIELD )
  {
    /* ���������ϢΪ��Ч�ֶΣ���ô���ط�����״̬ */
    eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_REJECTED;
  }
  else
  {
    /* ���������ϢΪ��Ч�ֶΣ���ô���ط�����״̬ */
    eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED;
  }

  /* �ط���Ӧ */
  pct_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                             pCmd->startTime, eventStatus,
                             pCmd->criticalityLevel, pCmd->eventControl);

  if ( status != ZCL_STATUS_INVALID_FIELD )
  {
    /* �������ؿ����¼� */
    if ( pCmd->issuerEvent == LOADCONTROL_EVENT_ID )
    {
      if ( pCmd->startTime == START_TIME_NOW ) // start time = NOW
      {
        /* �ط�״̬�¼� = ���ؿ����¼���ʼ */
        eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_STARTED;
        pct_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                                   pCmd->startTime, eventStatus,
                                   pCmd->criticalityLevel, pCmd->eventControl);
        /* �Ƿ���סլ���ظ��� */
        if ( pCmd->deviceGroupClass == ONOFF_LOAD_DEVICE_CLASS ) 
        {
          HalLcdWriteString("Load Evt Started", HAL_LCD_LINE_6);
        }
        /* �Ƿ���HVACѹ����/¯���ظ��� */
        else if ( pCmd->deviceGroupClass == HVAC_DEVICE_CLASS )
        {
          HalLcdWriteString("PCT Evt Started", HAL_LCD_LINE_6);
        }

        HalLedBlink ( HAL_LED_1, 0, 50, 500 );

        osal_start_timerEx( pctTaskID, PCT_LOAD_CTRL_EVT,
                           (PCT_LOAD_CTRL_PERIOD * (pCmd->durationInMinutes)) );
      }
    }
  }
#endif
}


/*********************************************************************
 * �������ƣ�pct_CancelLoadControlEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ��ȡ�����ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ��ȡ�����ؿ����¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                          afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_LOAD_CONTROL )
  if ( 0 )  // ���Ҫȡ�����¼�����ʹ���¼����ڱ�־���� "0"
  {
    /* �û����ڴ����ȡ���¼����� */ 

    // ���������Ӵ��룬�����ط���Ӧ
    /*
    pct_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                               // startTime
                               EVENT_STATUS_LOAD_CONTROL_EVENT_CANCELLED, // eventStatus
                               // Criticality level
                               // eventControl };
    */
  }
  else
  {
    /* ����¼������ڣ�Ӧ��״̬���������� */ 
    pct_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                               SE_OPTIONAL_FIELD_UINT32,                  // ����ʱ��
                               EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED,  // �¼�״̬
                               SE_OPTIONAL_FIELD_UINT8,                   // �ٽ�״̬�ȼ�
                               SE_OPTIONAL_FIELD_UINT8 );                 // �¼�����
  }
#endif
}


/*********************************************************************
 * �������ƣ�pct_CancelAllLoadControlEventsCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ��ȡ�����и��ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ��ȡ�����и��ؿ����¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                              afAddrType_t *srcAddr, uint8 seqNum )
{
  /* 
    �ڽ��յ�ȡ�����и��ؿ����¼�����󣬽��յ�������豸Ӧ��ѯ����
    �¼��б����Ҹ�ÿ���¼��ֱ�����Ӧ 
   */
}


/*********************************************************************
 * �������ƣ�pct_ReportEventStatusCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ�������¼�״̬�¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ�򱨸��¼�״̬����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum)
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�pct_GetScheduledEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ����ȡԤ���¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ���ȡԤ���¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/* 
  ����ZCL������������/��Ӧ��Ϣ
 */
/*********************************************************************
 * �������ƣ�pct_ProcessZCLMsg
 * ��    �ܣ�����ZCL����������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void pct_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      pct_ProcessInReadRspCmd( pInMsg );
      break;
#endif 
#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      pct_ProcessInWriteRspCmd( pInMsg );
      break;
#endif 
    case ZCL_CMD_DEFAULT_RSP:
      pct_ProcessInDefaultRspCmd( pInMsg );
      break;
#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      pct_ProcessInDiscRspCmd( pInMsg );
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
 * �������ƣ�pct_ProcessInReadRspCmd
 * ��    �ܣ����������ļ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 pct_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�pct_ProcessInWriteRspCmd
 * ��    �ܣ����������ļ�д��Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 pct_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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


/*********************************************************************
 * �������ƣ�pct_ProcessInDefaultRspCmd
 * ��    �ܣ����������ļ�Ĭ����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 pct_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  return TRUE;
}

#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * �������ƣ�pct_ProcessInDiscRspCmd
 * ��    �ܣ����������ļ�������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 pct_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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
