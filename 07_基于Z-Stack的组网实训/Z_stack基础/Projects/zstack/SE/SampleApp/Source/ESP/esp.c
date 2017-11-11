/*******************************************************************************
 * �ļ����ƣ�esp.c
 * ��    �ܣ�SE������Դʵ�顣
 * ʵ�����ݣ�ESP(��Դ�����Ż�)����һ��ZigBeePRO��·�������豸������������IPD(
 *           Ԥװ��ʾ�豸)��PCT(�ɱ��ͨ���¿���)�ȷ��ָ����磬���Ҽ��뵽�������С�
 *           ESP(��Դ�����Ż�)��ͨ�����߷�ʽ���������豸��
 * ʵ���豸��SK-SmartRF05EB��װ����SK-CC2530EM��
 *           UP����            �������为�ؿ����¼���PCT(�ɱ��ͨ���¿���)
 *           RIGHT����         ���͸��ؿ����¼������ؿ�����
 *           DOWN����          ������Ϣ��IPD(Ԥװ��ʾ�豸)
 *           LEFT����          ���ͷ��ַ�����Ϣ����ȡPCT(�ɱ��ͨ���¿���)��
 *                             ���ؿ�������16λ�����ַ
 *           LED1(��ɫ)��      �豸�ɹ�������1���������˳���1��Ϩ��
 *           LED3(��ɫ)��      ���ɹ�������������������LED����
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "MT.h"
#include "MT_APP.h"
#include "ZDObject.h"
#include "AddrMgr.h"

#include "se.h"
#include "esp.h"
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

/*
   ESP��С���������
*/
#define ESP_MIN_REPORTING_INTERVAL       5


/* ���ر��� */
/********************************************************************/
static uint8 espTaskID;                              // esp����ϵͳ����ID
static afAddrType_t ipdAddr;                         // IPD��Ŀ�ĵ�ַ
static afAddrType_t pctAddr;                         // PCT��Ŀ�ĵ�ַ
static afAddrType_t loadControlAddr;                 // LCD���ؿ�������Ŀ�ĵ�ַ
static zAddrType_t simpleDescReqAddr[2];             // ����������Ŀ�ĵ�ַ
static uint8 matchRspCount = 0;                      // ���յ���ƥ����Ӧ����
static zAddrType_t dstMatchAddr;                     // ƥ�������������ͨ��Ŀ�ĵ�ַ
static zclCCLoadControlEvent_t loadControlCmd;       // ���ؿ������������ṹ

/*
   ƥ��������������б�
*/
static const cId_t matchClusterList[1] =
{
  ZCL_CLUSTER_ID_SE_LOAD_CONTROL
};


/* ���غ��� */
/********************************************************************/
static void esp_HandleKeys( uint8 shift, uint8 keys );
static void esp_ProcessAppMsg( uint8 *msg );

#if defined (ZCL_KEY_ESTABLISH)
static uint8 esp_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif

#if defined (ZCL_ALARMS)
static void esp_ProcessAlarmCmd( uint8 srcEP, afAddrType_t *dstAddr,
                        uint16 clusterID, zclFrameHdr_t *hdr, uint8 len, uint8 *data );
#endif

static void esp_ProcessIdentifyTimeChange( void );


/* ���غ��� - Ӧ�ûص����� */
/********************************************************************/

/*
   �����ص�����
*/
static uint8 esp_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

/*
   ͨ�ôػص�����
*/
static void esp_BasicResetCB( void );
static void esp_IdentifyCB( zclIdentify_t *pCmd );
static void esp_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void esp_AlarmCB( zclAlarm_t *pAlarm );

/*
   SE�ص�����
*/
static void esp_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                          afAddrType_t *srcAddr, uint8 status, uint8 seqNum);
static void esp_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum);
static void esp_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum);


/* ����ZCL�������� - ��������/��Ӧ��Ϣ */
/********************************************************************/
static void esp_ProcessZCLMsg( zclIncomingMsg_t *msg );

#if defined ( ZCL_READ )
static uint8 esp_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif

#if defined ( ZCL_WRITE )
static uint8 esp_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif 

#if defined ( ZCL_REPORT )
static uint8 esp_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReportCmd( zclIncomingMsg_t *pInMsg );
#endif 

static uint8 esp_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

#if defined ( ZCL_DISCOVER )
static uint8 esp_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif 


/*
   ZDO��Ϣ�������
*/
static void esp_ProcessZDOMsg( zdoIncomingMsg_t *inMsg );

/*
   ZCLͨ�ôػص���
*/
static zclGeneral_AppCallbacks_t esp_GenCmdCallbacks =
{
  esp_BasicResetCB,              // �����Ĵظ�λ����
  esp_IdentifyCB,                // ʶ������
  esp_IdentifyQueryRspCB,        // ʶ���ѯ��Ӧ����
  NULL,                          // ���ش�����
  NULL,                          // ��������뿪����
  NULL,                          // ��������ƶ�����
  NULL,                          // ������Ƶ�������
  NULL,                          // �������ֹͣ����
  NULL,                          // ����Ӧ����
  NULL,                          // ����������������
  NULL,                          // �����ص�����������
  NULL,                          // ������Ӧ����
  esp_AlarmCB,                   // ����(��Ӧ)����
  NULL,                          // RSSI��λ����
  NULL,                          // RSSI��λ��Ӧ����
};

/*
   ZCL SE�ػص���
*/
static zclSE_AppCallbacks_t esp_SECmdCallbacks =			
{
  esp_GetProfileCmdCB,                     // ��ȡ Profile ����
  esp_GetProfileRspCB,                     // ��ȡ Profile ��Ӧ
  esp_ReqMirrorCmdCB,                      // ����������
  esp_ReqMirrorRspCB,                      // ��������Ӧ
  esp_MirrorRemCmdCB,                      // �����Ƴ�����
  esp_MirrorRemRspCB,                      // �����Ƴ���Ӧ
  esp_GetCurrentPriceCB,                   // ��ȡ��ǰ�۸�
  esp_GetScheduledPriceCB,                 // ��ȡԤ���۸�
  esp_PublishPriceCB,                      // �����۸�
  esp_DisplayMessageCB,                    // ��ʾ��Ϣ����
  esp_CancelMessageCB,                     // ȡ����Ϣ����
  esp_GetLastMessageCB,                    // ��ȡ���һ����Ϣ����
  esp_MessageConfirmationCB,               // ��Ϣȷ��
  esp_LoadControlEventCB,                  // ���ؿ����¼�
  esp_CancelLoadControlEventCB,            // �˳����ؿ����¼�
  esp_CancelAllLoadControlEventsCB,        // �˳����и��ؿ����¼�
  esp_ReportEventStatusCB,                 // �����¼�״̬
  esp_GetScheduledEventCB,                 // ��ȡԤ���¼�
};


/*********************************************************************
 * �������ƣ�esp_Init
 * ��    �ܣ�ESP�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void esp_Init( uint8 task_id )
{
  espTaskID = task_id;

  /* ע��һ��SE�˵� */
  zclSE_Init( &espSimpleDesc );

  /* ע��ZCLͨ�ôؿ�ص����� */
  zclGeneral_RegisterCmdCallbacks( ESP_ENDPOINT, &esp_GenCmdCallbacks );

  /* ע��ZCL SE�ؿ�ص����� */
  zclSE_RegisterCmdCallbacks( ESP_ENDPOINT, &esp_SECmdCallbacks );

  /* ע��Ӧ�ó��������б� */
  zcl_registerAttrList( ESP_ENDPOINT, ESP_MAX_ATTRIBUTES, espAttrs );

  /* ע��Ӧ�ó����ѡ���б� */
  zcl_registerClusterOptionList( ESP_ENDPOINT, ESP_MAX_OPTIONS, espOptions );

  /* ע��Ӧ�ó�������������֤�ص����� */
  zcl_registerValidateAttrData( esp_ValidateAttrDataCB );

  /* ע��Ӧ�ó���������δ�����������/��Ӧ��Ϣ */
  zcl_registerForMsg( espTaskID );

  /* ע��ƥ���������ͼ�������Ӧ */
  ZDO_RegisterForZDOMsg( espTaskID, Match_Desc_rsp );
  ZDO_RegisterForZDOMsg( espTaskID, Simple_Desc_rsp );

  /* ע�����а����¼� - ���������а����¼� */
  RegisterForKeys( espTaskID );

  /* ������ʱ��ʹ�ò���ϵͳʱ����ͬ��ESP��ʱ�� */
  osal_start_timerEx( espTaskID, ESP_UPDATE_TIME_EVT, ESP_UPDATE_TIME_PERIOD );

  /* ΪPCT���õ�ַģʽ��Ŀ�Ķ˵��ֶ� */
  pctAddr.addrMode = (afAddrMode_t)Addr16Bit;
  pctAddr.endPoint = ESP_ENDPOINT;

  /* ΪLCD���ؿ������õ�ַģʽ��Ŀ�Ķ˵��ֶ� */
  loadControlAddr.addrMode = (afAddrMode_t)Addr16Bit;
  loadControlAddr.endPoint = ESP_ENDPOINT;

  /* ���ø��ؿ�������ṹ */
  loadControlCmd.issuerEvent = 0x12345678;            // ����id
  loadControlCmd.deviceGroupClass = 0x000000;         // �������ַ
  loadControlCmd.startTime = 0x00000000;              // ��ʼʱ�� - ����
  loadControlCmd.durationInMinutes = 0x0001;          // ����ʱ��1����
  loadControlCmd.criticalityLevel = 0x01;             // ��ɫˮƽ
  loadControlCmd.coolingTemperatureSetPoint = 0x076C; // 19���϶ȣ�66.2����
  loadControlCmd.eventControl = 0x00;                 // û��������������Ӧ��
  
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString("SE(ESP)", HAL_LCD_LINE_2 );
#endif
}


/*********************************************************************
 * �������ƣ�esp_event_loop
 * ��    �ܣ�ESP�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
uint16 esp_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( espTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case MT_SYS_APP_MSG:
          /* ��MT(����)���յ���Ϣ */
          esp_ProcessAppMsg( ((mtSysAppMsg_t *)MSGpkt)->appData );
          break;

        case ZCL_INCOMING_MSG:
          /* ����ZCL��������/��Ӧ��Ϣ */
          esp_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          /* �������� */
          esp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_CB_MSG:
          /* ZDO����ע����Ϣ */ 
          esp_ProcessZDOMsg( (zdoIncomingMsg_t *)MSGpkt );
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt ); // �ͷŴ洢��
    }

    return (events ^ SYS_EVENT_MSG); // ����δ������¼�
  }

  /* ������ʶ���������ʶ��ʱ�¼� */
  if ( events & ESP_IDENTIFY_TIMEOUT_EVT )
  {
    if ( espIdentifyTime > 0 )
    {
      espIdentifyTime--;
    }
    esp_ProcessIdentifyTimeChange();

    return ( events ^ ESP_IDENTIFY_TIMEOUT_EVT );
  }

  /* ��ȡ��ǰʱ���¼� */
  if ( events & ESP_UPDATE_TIME_EVT )
  {
    espTime = osal_getClock();
    osal_start_timerEx( espTaskID, ESP_UPDATE_TIME_EVT, ESP_UPDATE_TIME_PERIOD );

    return ( events ^ ESP_UPDATE_TIME_EVT );
  }

  /* ��ȡ��һ��ƥ����Ӧ�ļ��������¼� */
  if ( events & SIMPLE_DESC_QUERY_EVT_1 )
  {
    ZDP_SimpleDescReq( &simpleDescReqAddr[0], simpleDescReqAddr[0].addr.shortAddr,
                        ESP_ENDPOINT, 0);

    return ( events ^ SIMPLE_DESC_QUERY_EVT_1 );
  }

  /* ��ȡ�ڶ���ƥ����Ӧ�ļ��������¼� */
  if ( events & SIMPLE_DESC_QUERY_EVT_2 )
  {
    ZDP_SimpleDescReq( &simpleDescReqAddr[1], simpleDescReqAddr[1].addr.shortAddr,
                        ESP_ENDPOINT, 0);

    return ( events ^ SIMPLE_DESC_QUERY_EVT_2 );
  }

  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�esp_ProcessAppMsg
 * ��    �ܣ�����MT����ϵͳӦ�ó�����Ϣ���̵Ĺؼ�����
 * ��ڲ�����msg   ָ����Ϣ��ָ�롣
 *           0     Ŀ�ĵ�ַ��λ�ֽ�
 *           1     Ŀ�ĵ�ַ��λ�ֽ�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ProcessAppMsg( uint8 *msg )
{
  afAddrType_t dstAddr;

  dstAddr.addr.shortAddr = BUILD_UINT16( msg[0], msg[1] );
  esp_KeyEstablish_ReturnLinkKey( dstAddr.addr.shortAddr );
}


/*********************************************************************
 * �������ƣ�esp_ProcessIdentifyTimeChange
 * ��    �ܣ�������˸LED��ָ��ʶ��ʱ������ֵ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ProcessIdentifyTimeChange( void )
{
  if ( espIdentifyTime > 0 )
  {
    osal_start_timerEx( espTaskID, ESP_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( espTaskID, ESP_IDENTIFY_TIMEOUT_EVT );
  }
}


#if defined (ZCL_KEY_ESTABLISH)
/*********************************************************************
 * �������ƣ�esp_KeyEstablish_ReturnLinkKey
 * ��    �ܣ���ȡ���������Կ������
 * ��ڲ�����shortAddr �̵�ַ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
{
  APSME_LinkKeyData_t* keyData;
  uint8 status = ZFailure;
  uint8 buf[1+SEC_KEY_LEN];
  uint8 len = 1;
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
      len += SEC_KEY_LEN; // ״̬ + ��Կֵ
    }
  }
  else
  {
    status = ZInvalidParameter; // δ֪�豸
  }

  buf[0] = status;

  if( status == ZSuccess )
  {
    osal_memcpy( &(buf[1]), keyData->key, SEC_KEY_LEN );
  }

  MT_BuildAndSendZToolResponse( ((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_DBG), MT_DEBUG_MSG,
                                  len, buf );
  return status;
}
#endif


/*********************************************************************
 * �������ƣ�esp_HandleKeys
 * ��    �ܣ�ESP�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_1     SK-SmartRF10GW��UP��
 *                  HAL_KEY_SW_2     SK-SmartRF10GW��RIGHT��      
 *                  HAL_KEY_SW_3     SK-SmartRF10GW��DOWN��
 *                  HAL_KEY_SW_4     SK-SmartRF10GW��LEFT��             
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_HandleKeys( uint8 shift, uint8 keys )
{

    if ( keys & HAL_KEY_SW_1 )
    {
      /* �������为�ؿ����¼���PCT(�ɱ��ͨ���¿���) */
      loadControlCmd.deviceGroupClass = HVAC_DEVICE_CLASS; // HVAC ѹ������¯ - λ0������
      zclSE_LoadControl_Send_LoadControlEvent( ESP_ENDPOINT, &pctAddr, &loadControlCmd, TRUE, 0 );
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      /* ���͸��ؿ����¼������ؿ����� */
      loadControlCmd.deviceGroupClass = ONOFF_LOAD_DEVICE_CLASS; // ��סլ���ظ��� - λ7������
      zclSE_LoadControl_Send_LoadControlEvent( ESP_ENDPOINT, &loadControlAddr, &loadControlCmd, TRUE, 0 );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
      /* Ԥװ��ʾ�豸 */
      zclCCDisplayMessage_t displayCmd;             // ��Ҫ�����͵�IPD��Ϣ������ṹ
      uint8 msgString[]="Rcvd MESSAGE Cmd";         // IPD����ʾ����Ϣ

      /* ��IPD����ʾ���͵���Ϣ */
      displayCmd.msgString.strLen = sizeof(msgString);
      displayCmd.msgString.pStr = msgString;

      zclSE_Message_Send_DisplayMessage( ESP_ENDPOINT, &ipdAddr, &displayCmd, TRUE, 0 );
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      /* ִ��PCT�͸��ؿ������ķ��ַ���*/
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      /* ��ʼ��һ��ƥ����������(������) */
      dstMatchAddr.addrMode = AddrBroadcast;
      dstMatchAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstMatchAddr, NWK_BROADCAST_SHORTADDR,
                        ZCL_SE_PROFILE_ID,
                        1, (cId_t *)matchClusterList,
                        1, (cId_t *)matchClusterList,
                        FALSE );
    }
}


/*********************************************************************
 * �������ƣ�esp_ValidateAttrDataCB
 * ��    �ܣ����Ϊ�����ṩ������ֵ�Ƿ��ڸ�����ָ���ķ�Χ�ڡ�
 * ��ڲ�����pAttr     ָ�����Ե�ָ��
 *           pAttrInfo ָ��������Ϣ��ָ��            
 * ���ڲ�������
 * �� �� ֵ��TRUE      ��Ч����
 *           FALSE     ��Ч����
 ********************************************************************/
static uint8 esp_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
{
  uint8 valid = TRUE;

  switch ( pAttrInfo->dataType )
  {
    case ZCL_DATATYPE_BOOLEAN:
      if ( ( *(pAttrInfo->attrData) != 0 ) && ( *(pAttrInfo->attrData) != 1 ) )
      {
        valid = FALSE;
      }
      break;

    default:
      break;
  }

  return ( valid );
}


/*********************************************************************
 * �������ƣ�esp_BasicResetCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص��������������дص���������Ϊ����Ĭ�����á�
 * ��ڲ�������          
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_BasicResetCB( void )
{
  // �û�Ӧ���ڴ˴���ص��������õ�����
}


/*********************************************************************
 * �������ƣ�esp_IdentifyCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ������󱻵��á�
 * ��ڲ�����pCmd      ָ��ʶ������ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_IdentifyCB( zclIdentify_t *pCmd )
{
  espIdentifyTime = pCmd->identifyTime;
  esp_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * �������ƣ�esp_IdentifyQueryRspCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ���ѯ����
 *           �󱻵��á�
 * ��ڲ�����pRsp    ָ��ʶ���ѯ��Ӧ�ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_AlarmCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó��򱨾���������
 *           �󱻵��á�
 * ��ڲ�����pAlarm   ָ�򱨾�����ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_AlarmCB( zclAlarm_t *pAlarm )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_GetProfileCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ��ȡ�����ļ��󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ�����ļ��ṹ��ָ��     
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
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
  zclSE_SimpleMetering_Send_GetProfileRsp( ESP_ENDPOINT, srcAddr, endTime,
                                           status,
                                           profileIntervalPeriod,
                                           numberOfPeriodDelivered, intervals,
                                           false, seqNum );
#endif 
}


/*********************************************************************
 * �������ƣ�esp_GetProfileRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ��ȡ�����ļ���Ӧ�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ�����ļ���Ӧ�ṹ��ָ��     
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_ReqMirrorCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�����������󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_ReqMirrorRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���������Ӧ�󱻵��á�
 * ��ڲ�����pRsp     ָ����������Ӧ�ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pRsp, afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_MirrorRemCmdCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ��������Ƴ�����󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_MirrorRemRspCB
 * ��    �ܣ�ZCL SE �����ļ��򵥼����ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ��������Ƴ���Ӧ�󱻵��á�
 * ��ڲ�����pCmd     ָ�����Ƴ���Ӧ�ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_GetCurrentPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡ��ǰ�۸�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ��ǰ�۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PRICING )
  /* �ڻ�ȡ��ǰ�۸�������豸Ӧ�����ʹ��е�ǰʱ����Ϣ�Ĺ����۸����� */  
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.priceTier = 0xfe;

  /* 
    ���Ƶ�ǰ������Ϣ������ʾ�豸��Դ��ַ��ʹESP���Է�����Ϣ����ʹ��
    IPDAddr��Ŀ�ĵ�ַ
   */  

  osal_memcpy( &ipdAddr, srcAddr, sizeof ( afAddrType_t ) );

  zclSE_Pricing_Send_PublishPrice( ESP_ENDPOINT, srcAddr, &cmd, false, seqNum );
#endif
}


/*********************************************************************
 * �������ƣ�esp_GetScheduledPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡԤ���۸�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡԤ���۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd, 
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  /* �ڻ�ȡԤ���۸�������豸��Ϊ��ǰԤ���۸��¼����͹����۸����� */ 
  /* ���Ӵ������� */

#if defined ( ZCL_PRICING )
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.priceTier = 0xfe;

  zclSE_Pricing_Send_PublishPrice( ESP_ENDPOINT, srcAddr, &cmd, false, seqNum );

#endif
}


/*********************************************************************
 * �������ƣ�esp_PublishPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���չ����۸�󱻵��á�
 * ��ڲ�����pCmd     ָ�򹫲��۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_DisplayMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ʾ��Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ����ʾ��Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( (char*)pCmd->msgString.pStr, HAL_LCD_LINE_6 );
#endif  
}


/*********************************************************************
 * �������ƣ�esp_CancelMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�ȡ����Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ��ȡ����Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                 afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_GetLastMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡ�����Ϣ����󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  /* �ڻ�ȡ�����Ϣ������豸Ӧ���ط���ʾ��Ϣ�����������Ϣ�� */  

#if defined ( ZCL_MESSAGE )
  zclCCDisplayMessage_t cmd;
  uint8 msg[10] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29 };

  /* Ϊ�����Ϣʹ��������Ϣ��������� */
  cmd.messageId = 0xaabbccdd;
  cmd.messageCtrl.transmissionMode = 0;
  cmd.messageCtrl.importance = 1;
  cmd.messageCtrl.confirmationRequired = 1;
  cmd.durationInMinutes = 60;

  cmd.msgString.strLen = 10;
  cmd.msgString.pStr = msg;

  zclSE_Message_Send_DisplayMessage( ESP_ENDPOINT, srcAddr, &cmd,
                                     false, seqNum );
#endif
}


/*********************************************************************
 * �������ƣ�esp_MessageConfirmationCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�ȷ����Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ��ȷ����Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


#if defined (ZCL_LOAD_CONTROL)
/*********************************************************************
 * �������ƣ�esp_SendReportEventStatus
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
static void esp_SendReportEventStatus( afAddrType_t *srcAddr, uint8 seqNum,
                                       uint32 eventID, uint32 startTime,
                                       uint8 eventStatus, uint8 criticalityLevel,
                                       uint8 eventControl )
{
  zclCCReportEventStatus_t *pRsp;

  pRsp = (zclCCReportEventStatus_t *)osal_mem_alloc( sizeof( zclCCReportEventStatus_t ) );

  if ( pRsp != NULL)
  {
    /* ǿ�����ֶ� - ʹ���������� */
    pRsp->issuerEventID = eventID;
    pRsp->eventStartTime = startTime;
    pRsp->criticalityLevelApplied = criticalityLevel;
    pRsp->eventControl = eventControl;
    pRsp->eventStatus = eventStatus;
    pRsp->signatureType = SE_PROFILE_SIGNATURE_TYPE_ECDSA;

    /* esp_Signature Ϊ��̬���飬����esp_data.c�ļ��и��� */
    osal_memcpy( pRsp->signature, espSignature, 16 );

    /* ��ѡ���ֶ� - Ĭ��ʹ��δʹ��ֵ����� */
    pRsp->coolingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
    pRsp->heatingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
    pRsp->averageLoadAdjustment = SE_OPTIONAL_FIELD_INT8;
    pRsp->dutyCycleApplied = SE_OPTIONAL_FIELD_UINT8;

    /* 
       ����Ӧ����Ӧ
       DisableDefaultResponse ��־λ������Ϊ�� - ���ǽ����Ĭ����Ӧ��
       ��Ϊ�����¼�״̬������û��������Ӧ
     */
    zclSE_LoadControl_Send_ReportEventStatus( ESP_ENDPOINT, srcAddr,
                                            pRsp, false, seqNum );
    osal_mem_free( pRsp );
  }
}
#endif  


/*********************************************************************
 * �������ƣ�esp_LoadControlEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ�����ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ���ؿ����¼�����
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 *           Status           �¼�״̬
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                                    afAddrType_t *srcAddr, uint8 status,
                                    uint8 seqNum )
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
  esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                             pCmd->startTime, eventStatus,
                             pCmd->criticalityLevel, pCmd->eventControl);

  if ( status != ZCL_STATUS_INVALID_FIELD )
  {
    // �ڴ�����û����ؿ����¼��������
  }
#endif  
}


/*********************************************************************
 * �������ƣ�esp_CancelLoadControlEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ��ȡ�����ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ��ȡ�����ؿ����¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                          afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_LOAD_CONTROL )
  if ( 0 )  // ���Ҫȡ�����¼�����ʹ���¼����ڱ�־���� "0"
  {
    /* �û����ڴ����ȡ���¼����� */ 

    // ���������Ӵ��룬�����ط���Ӧ
    /*
    esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                               // startTime
                               EVENT_STATUS_LOAD_CONTROL_EVENT_CANCELLED, // eventStatus
                               // Criticality level
                               // eventControl };
    */

  }
  else
  {
	/* ����¼������ڣ�Ӧ��״̬���������� */ 
    esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                               SE_OPTIONAL_FIELD_UINT32,                  // ����ʱ��
                               EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED,  // �¼�״̬
                               SE_OPTIONAL_FIELD_UINT8,                   // �ٽ�״̬�ȼ�
                               SE_OPTIONAL_FIELD_UINT8 );                 // �¼�����
  }
#endif  
}


/*********************************************************************
 * �������ƣ�esp_CancelAllLoadControlEventsCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ��ȡ�����и��ؿ����¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ��ȡ�����и��ؿ����¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                                 afAddrType_t *srcAddr, uint8 seqNum )
{
  /* 
    �ڽ��յ�ȡ�����и��ؿ����¼�����󣬽��յ�������豸Ӧ��ѯ����
    �¼��б����Ҹ�ÿ���¼��ֱ�����Ӧ 
   */
}


/*********************************************************************
 * �������ƣ�esp_ReportEventStatusCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ�������¼�״̬�¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ�򱨸��¼�״̬����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum)
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�esp_GetScheduledEventCB
 * ��    �ܣ�ZCL SE �����ļ����ؿ��ƴؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�һ����ȡԤ���¼�����󱻵��á�
 * ��ڲ�����pCmd             ָ���ȡԤ���¼�����ṹָ��
 *           srcAddr          ָ��Դ��ַָ��
 *           seqNum           ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/* 
  ����ZDO������Ϣ����
*/
/*********************************************************************
 * �������ƣ�esp_ProcessZDOMsg
 * ��    �ܣ�����ZDO������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ProcessZDOMsg( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case Match_Desc_rsp:
    {
      ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        
      if ( pRsp )
      {
        matchRspCount++; // �鵽��ƥ�䣬������Ӧ������
        if ( pRsp->status == ZSuccess && pRsp->cnt )
        {
          if ( matchRspCount == 1 )
          {
            simpleDescReqAddr[0].addrMode = (afAddrMode_t)Addr16Bit;
            simpleDescReqAddr[0].addr.shortAddr = pRsp->nwkAddr;

            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON ); //����LED4

            /* ���ü���������ѯ�¼� */
            osal_set_event( espTaskID, SIMPLE_DESC_QUERY_EVT_1 );
          }
          else if ( matchRspCount == 2 )
          {
            matchRspCount = 0; // ��λ��Ӧ������
            simpleDescReqAddr[1].addrMode = (afAddrMode_t)Addr16Bit;
            simpleDescReqAddr[1].addr.shortAddr = pRsp->nwkAddr;

            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON ); //����LED4

            /* ���ü���������ѯ�¼� */
            osal_set_event( espTaskID, SIMPLE_DESC_QUERY_EVT_2 );
          }
        }

        osal_mem_free( pRsp );
      }
    }
    break;

    case Simple_Desc_rsp:
    {
      ZDO_SimpleDescRsp_t *pSimpleDescRsp;   // ָ����ռ���������Ӧ
      pSimpleDescRsp = (ZDO_SimpleDescRsp_t *)osal_mem_alloc( sizeof( ZDO_SimpleDescRsp_t ) );

      if(pSimpleDescRsp)
      {
        ZDO_ParseSimpleDescRsp( inMsg, pSimpleDescRsp );
		
	/* PCT�豸 */
        if( pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_SE_DEVICEID_PCT )
        {
          pctAddr.addr.shortAddr = pSimpleDescRsp->nwkAddr;
        }
	/* ���ؿ����豸 */
        else if ( pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_SE_DEVICEID_LOAD_CTRL_EXTENSION )
        {
          loadControlAddr.addr.shortAddr = pSimpleDescRsp->nwkAddr;
        }
        osal_mem_free( pSimpleDescRsp );
      }
    }
    break;
  }
}


/* 
  ����ZCL������������/��Ӧ��Ϣ
 */
/*********************************************************************
 * �������ƣ�esp_ProcessZCLMsg
 * ��    �ܣ�����ZCL����������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void esp_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      esp_ProcessInReadRspCmd( pInMsg );
      break;
#endif 
	  
#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      esp_ProcessInWriteRspCmd( pInMsg );
      break;
#endif

#if defined ( ZCL_REPORT )
    case ZCL_CMD_CONFIG_REPORT:
      esp_ProcessInConfigReportCmd( pInMsg );
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      esp_ProcessInConfigReportRspCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      esp_ProcessInReadReportCfgCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      esp_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      esp_ProcessInReportCmd( pInMsg );
      break;
#endif

    case ZCL_CMD_DEFAULT_RSP:
      esp_ProcessInDefaultRspCmd( pInMsg );
      break;
#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      esp_ProcessInDiscRspCmd( pInMsg );
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
 * �������ƣ�esp_ProcessInReadRspCmd
 * ��    �ܣ����������ļ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�esp_ProcessInWriteRspCmd
 * ��    �ܣ����������ļ�д��Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�esp_ProcessInConfigReportCmd
 * ��    �ܣ����������ļ����ñ��������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TURE            �������б����ҵ�����
 *           FALSE           δ�ҵ�
 ********************************************************************/
static uint8 esp_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportCmd_t *cfgReportCmd;
  zclCfgReportRec_t *reportRec;
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 status;
  uint8 i, j = 0;

  cfgReportCmd = (zclCfgReportCmd_t *)pInMsg->attrCmd;

  /* Ϊ��Ӧ�������ռ� */
  cfgReportRspCmd = (zclCfgReportRspCmd_t *)osal_mem_alloc( 
	  sizeof ( zclCfgReportRspCmd_t ) + sizeof ( zclCfgReportStatus_t) 
	                                         * cfgReportCmd->numAttr );
  if ( cfgReportRspCmd == NULL )
    return FALSE;

  /* ����ÿ���������ü�¼���� */
  for ( i = 0; i < cfgReportCmd->numAttr; i++ )
  {
    reportRec = &(cfgReportCmd->attrList[i]);

    status = ZCL_STATUS_SUCCESS;

    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId, reportRec->attrID, &attrRec ) )
    {
      if ( reportRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        if ( reportRec->dataType == attrRec.attr.dataType )
        {
          /* ������������� */ 
          if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
          {
            if ( reportRec->minReportInt < ESP_MIN_REPORTING_INTERVAL ||
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
              status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;  
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
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
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
  zcl_SendConfigReportRspCmd( ESP_ENDPOINT, &(pInMsg->srcAddr),
                              pInMsg->clusterId, cfgReportRspCmd, 
                              ZCL_FRAME_SERVER_CLIENT_DIR,
                              true, pInMsg->zclHdr.transSeqNum );
  
  osal_mem_free( cfgReportRspCmd );

  return TRUE ;
}


/*********************************************************************
 * �������ƣ�esp_ProcessInConfigReportRspCmd
 * ��    �ܣ����������ļ����ñ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TURE            �ɹ�
 *           FALSE           δ�ɹ�
 ********************************************************************/
static uint8 esp_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 i;

  cfgReportRspCmd = (zclCfgReportRspCmd_t *)pInMsg->attrCmd;

  for (i = 0; i < cfgReportRspCmd->numAttr; i++)
  {
    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
                         cfgReportRspCmd->attrList[i].attrID, &attrRec ) )
    {
      // �ڴ���Ӵ���
    }
  }

  return TRUE;
}


/*********************************************************************
 * �������ƣ�esp_ProcessInReadReportCfgCmd
 * ��    �ܣ����������ļ���ȡ�������������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg )
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
    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
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

    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
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
  zcl_SendReadReportCfgRspCmd( ESP_ENDPOINT, &(pInMsg->srcAddr),
                               pInMsg->clusterId, readReportCfgRspCmd, 
                               ZCL_FRAME_SERVER_CLIENT_DIR,
                               true, pInMsg->zclHdr.transSeqNum );
  
  osal_mem_free( readReportCfgRspCmd );

  return TRUE;
}


/*********************************************************************
 * �������ƣ�esp_ProcessInReadReportCfgRspCmd
 * ��    �ܣ����������ļ���ȡ����������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�esp_ProcessInReportCmd
 * ��    �ܣ����������ļ����������
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 esp_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclReportCmd_t *reportCmd;
  zclReport_t *reportRec;
  uint8 i;
  uint8 *meterData;
  char lcdBuf[13];

  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    /* �豸�����������������ֵ���豸��֪ͨ */
    reportRec = &(reportCmd->attrList[i]);

    if ( reportRec->attrID == ATTRID_SE_CURRENT_SUMMATION_DELIVERED )
    {
      /* ����ǰ�ַ��ܺ����Լ򵥼��� */
      meterData = reportRec->attrData;

      /* ʮ������תASCIIת������ */
      for(i=0; i<6; i++)
      {
        if(meterData[5-i] == 0)
        {
          lcdBuf[i*2] = '0';
          lcdBuf[i*2+1] = '0';
        }
        else if(meterData[5-i] <= 0x0A)
        {
          lcdBuf[i*2] = '0';
          _ltoa(meterData[5-i],(uint8*)&lcdBuf[i*2+1],16);
        }
        else
        {
          _ltoa(meterData[5-i],(uint8*)&lcdBuf[i*2],16);
        }
      }

      /* ��16���Ʒ�ʽ��ӡ����ǰ�ַ��ܺ� */
      HalLcdWriteString("Zigbee Coord esp", HAL_LCD_LINE_4 );
      HalLcdWriteString("Curr Summ Dlvd", HAL_LCD_LINE_5 );
      HalLcdWriteString(lcdBuf, HAL_LCD_LINE_6 );
    }
  }
  return TRUE;
}
#endif


/*********************************************************************
 * �������ƣ�esp_ProcessInDefaultRspCmd
 * ��    �ܣ����������ļ�Ĭ����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 esp_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  return TRUE;
}


#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * �������ƣ�esp_ProcessInDiscRspCmd
 * ��    �ܣ����������ļ�������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 esp_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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
