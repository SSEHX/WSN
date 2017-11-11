/*******************************************************************************
 * �ļ����ƣ�ipd.c
 * ��    �ܣ�SE������Դʵ�顣
 * ʵ�����ݣ�ESP(��Դ�����Ż�)����һ��ZigBeePRO��·�������豸������������IPD(
 *           Ԥװ��ʾ�豸)��PCT(�ɱ��ͨ���¿���)�ȷ��ָ����磬���Ҽ��뵽�������С�
 *           ESP(��Դ�����Ż�)��ͨ�����߷�ʽ���������豸��
 * ʵ���豸��SK-SmartRF05EB��װ����SK-CC2530EM��
 *           RIGHT����            ��������
 *
 *           LED1(��ɫ)��      �豸�ɹ�������1���������˳���1��Ϩ��
 *           LED3(��ɫ)��      ���ɹ�������������������LED����
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "ZDApp.h"
#include "AddrMgr.h"

#include "se.h"
#include "ipd.h"
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
static uint8 ipdTaskID;            // ipd����ϵͳ����ID
static uint8 ipdTransID;           // ����id
static devStates_t ipdNwkState;    // ����״̬����
static afAddrType_t ESPAddr;       // ESPĿ�ĵ�ַ
static uint8 linkKeyStatus;        // �ӻ�ȡ������Կ�������ص�״̬����
static uint8 option;               // ����ѡ���ֶ�
/********************************************************************/

/* ���غ��� */
/********************************************************************/
static void ipd_HandleKeys( uint8 shift, uint8 keys );

#if defined (ZCL_KEY_ESTABLISH)
static uint8 ipd_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif 

#if defined ( ZCL_ALARMS )
static void ipd_ProcessAlarmCmd( uint8 srcEP, afAddrType_t *dstAddr,
       uint16 clusterID, zclFrameHdr_t *hdr, uint8 len, uint8 *data );
#endif 

static void ipd_ProcessIdentifyTimeChange( void );
/********************************************************************/

/* ���غ��� - Ӧ�ûص����� */
/********************************************************************/

/*
   �����ص�����
*/
static uint8 ipd_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

/*
   ͨ�ôػص�����
*/
static void ipd_BasicResetCB( void );
static void ipd_IdentifyCB( zclIdentify_t *pCmd );
static void ipd_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void ipd_AlarmCB( zclAlarm_t *pAlarm );

/*
   SE�ص�����
*/
static void ipd_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );


/* ����ZCL�������� - ��������/��Ӧ��Ϣ */
/********************************************************************/
static void ipd_ProcessZCLMsg( zclIncomingMsg_t *msg );

#if defined ( ZCL_READ )
static uint8 ipd_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif

#if defined ( ZCL_WRITE )
static uint8 ipd_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif 

static uint8 ipd_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

#if defined ( ZCL_DISCOVER )
static uint8 ipd_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif 
/********************************************************************/


/*
   ZCLͨ�ôػص���
*/
static zclGeneral_AppCallbacks_t ipd_GenCmdCallbacks =
{
  ipd_BasicResetCB,              // �����ظ�λ����
  ipd_IdentifyCB,                // ʶ������
  ipd_IdentifyQueryRspCB,        // ʶ���ѯ��Ӧ����
  NULL,                          // ���ش�����
  NULL,                          // ��������뿪����
  NULL,                          // ��������ƶ�����
  NULL,                          // ������Ƶ�������
  NULL,                          // �������ֹͣ����
  NULL,                          // ����Ӧ����
  NULL,                          // ����������������
  NULL,                          // �����ص�����������
  NULL,                          // ������Ӧ����
  ipd_AlarmCB,                   // ����(��Ӧ)����
  NULL,                          // RSSI��λ����
  NULL,                          // RSSI��λ��Ӧ����
};

/*
   ZCL SE�ػص���
*/
static zclSE_AppCallbacks_t ipd_SECmdCallbacks =			
{
  NULL,                             // ��ȡ Profile ����
  NULL,                             // ��ȡ Profile ��Ӧ
  NULL,                             // ����������
  NULL,                             // ��������Ӧ
  NULL,                             // �����Ƴ�����
  NULL,                             // �����Ƴ���Ӧ
  ipd_GetCurrentPriceCB,            // ��ȡ��ǰ�۸�
  ipd_GetScheduledPriceCB,          // ��ȡԤ���۸�
  ipd_PublishPriceCB,               // �����۸�
  ipd_DisplayMessageCB,             // ��ʾ��Ϣ����
  ipd_CancelMessageCB,              // ȡ����Ϣ����
  ipd_GetLastMessageCB,             // ��ȡ���һ����Ϣ����
  ipd_MessageConfirmationCB,        // ��Ϣȷ��
  NULL,                             // ���ؿ����¼�
  NULL,                             // �˳����ؿ����¼�
  NULL,                             // �˳����и��ؿ����¼�
  NULL,                             // �����¼�״̬
  NULL,                             // ��ȡԤ���¼�
};



/*********************************************************************
 * �������ƣ�ipd_Init
 * ��    �ܣ�IPD�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void ipd_Init( uint8 task_id )
{
  ipdTaskID = task_id;
  ipdNwkState = DEV_INIT;
  ipdTransID = 0;

  /* 
     �����ڴ˴���Ӷ��豸Ӳ���ĳ�ʼ�����롣 
     ��Ӳ���Ǳ�Ӧ��ר�õģ����Խ��Ը�Ӳ���ĳ�ʼ����������ڴ˴��� 
     ��Ӳ�����Ǳ�Ӧ��ר�õģ����Խ���ʼ�����������Zmain.c�ļ��е�main()������
  */

  // ����ESPĿ�ĵ�ַ
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = IPD_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  /* ע��һ��SE�˵� */
  zclSE_Init( &ipdSimpleDesc );

  /* ע��ZCLͨ�ôؿ�ص����� */
  zclGeneral_RegisterCmdCallbacks( IPD_ENDPOINT, &ipd_GenCmdCallbacks );

  /* ע��ZCL SE�ؿ�ص����� */
  zclSE_RegisterCmdCallbacks( IPD_ENDPOINT, &ipd_SECmdCallbacks );

  /* ע��Ӧ�ó��������б� */
  zcl_registerAttrList( IPD_ENDPOINT, IPD_MAX_ATTRIBUTES, ipdAttrs );

  /* ע��Ӧ�ó����ѡ���б� */
  zcl_registerClusterOptionList( IPD_ENDPOINT, IPD_MAX_OPTIONS, ipdOptions );

  /* ע��Ӧ�ó�������������֤�ص����� */
  zcl_registerValidateAttrData( ipd_ValidateAttrDataCB );

  /* ע��Ӧ�ó���������δ�����������/��Ӧ��Ϣ */
  zcl_registerForMsg( ipdTaskID );

  /* ע�����а����¼� - ���������а����¼� */
  RegisterForKeys( ipdTaskID );

  /* ������ʱ��ʹ�ò���ϵͳʱ����ͬ��IPD��ʱ�� */
  osal_start_timerEx( ipdTaskID, IPD_UPDATE_TIME_EVT, IPD_UPDATE_TIME_PERIOD );
}


/*********************************************************************
 * �������ƣ�ipd_event_loop
 * ��    �ܣ�IPD�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
uint16 ipd_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ipdTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          /* ����ZCL��������/��Ӧ��Ϣ */
          ipd_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          /* �������� */
          ipd_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          ipdNwkState = (devStates_t)(MSGpkt->hdr.status);

          if (ZG_SECURE_ENABLED)
          {
            if ( ipdNwkState == DEV_END_DEVICE )
            {
              /* ���������Կ�Ƿ��Ѿ������� */
              linkKeyStatus = ipd_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

              if (linkKeyStatus != ZSuccess)
              {
                /* ������Կ�������� */
                osal_set_event( ipdTaskID, IPD_KEY_ESTABLISHMENT_REQUEST_EVT);
              }
              else
              {
                /* ������Կ�Ѿ����� �������ͱ��� */
                osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
              }
            }
          }
          else
          {
            osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
          }

          /* ����SE�ն��豸��ѯ���� */
          NLME_SetPollRate ( SE_DEVICE_POLL_RATE ); 

          break;

#if defined( ZCL_KEY_ESTABLISH )
        case ZCL_KEY_ESTABLISH_IND:
          osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
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
  if ( events & IPD_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(ipdTaskID, &ESPAddr, ipdTransID);

    return ( events ^ IPD_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  /* ��ȡ��ǰ�۸��¼� */
  if ( events & IPD_GET_PRICING_INFO_EVT )
  {
    zclSE_Pricing_Send_GetCurrentPrice( IPD_ENDPOINT, &ESPAddr, option, TRUE, 0 );
    osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );

    return ( events ^ IPD_GET_PRICING_INFO_EVT );
  }

  /* ��һ��ʶ���������ʶ��ʱ�¼����� */
  if ( events & IPD_IDENTIFY_TIMEOUT_EVT )
  {
    if ( ipdIdentifyTime > 0 )
    {
      ipdIdentifyTime--;
    }
    ipd_ProcessIdentifyTimeChange();

    return ( events ^ IPD_IDENTIFY_TIMEOUT_EVT );
  }

  /* ��ȡ��ǰʱ���¼� */
  if ( events & IPD_UPDATE_TIME_EVT )
  {
    ipdTime = osal_getClock();
    osal_start_timerEx( ipdTaskID, IPD_UPDATE_TIME_EVT, IPD_UPDATE_TIME_PERIOD );

    return ( events ^ IPD_UPDATE_TIME_EVT );
  }

  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�ipd_ProcessIdentifyTimeChange
 * ��    �ܣ�������˸LED��ָ��ʶ��ʱ������ֵ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_ProcessIdentifyTimeChange( void )
{
  if ( ipdIdentifyTime > 0 )
  {
    osal_start_timerEx( ipdTaskID, IPD_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( ipdTaskID, IPD_IDENTIFY_TIMEOUT_EVT );
  }
}


#if defined (ZCL_KEY_ESTABLISH)
/*********************************************************************
 * �������ƣ�ipd_KeyEstablish_ReturnLinkKey
 * ��    �ܣ���ȡ���������Կ������
 * ��ڲ�����shortAddr �̵�ַ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 ipd_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
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
 * �������ƣ�ipd_HandleKeys
 * ��    �ܣ�IPD�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_2     SK-SmartRF10BB��K2��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_2 )
  {
    ZDOInitDevice(0); // ��������
  }
}


/*********************************************************************
 * �������ƣ�ipd_ValidateAttrDataCB
 * ��    �ܣ����Ϊ�����ṩ������ֵ�Ƿ��ڸ�����ָ���ķ�Χ�ڡ�
 * ��ڲ�����pAttr     ָ�����Ե�ָ��
 *           pAttrInfo ָ��������Ϣ��ָ��            
 * ���ڲ�������
 * �� �� ֵ��TRUE      ��Ч����
 *           FALSE     ��Ч����
 ********************************************************************/
static uint8 ipd_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
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
 * �������ƣ�ipd_BasicResetCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص��������������дص���������Ϊ����Ĭ�����á�
 * ��ڲ�������          
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_BasicResetCB( void )
{
  // �ڴ���Ӵ���
}


/*********************************************************************
 * �������ƣ�ipd_IdentifyCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ������󱻵��á�
 * ��ڲ�����pCmd      ָ��ʶ������ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_IdentifyCB( zclIdentify_t *pCmd )
{
  ipdIdentifyTime = pCmd->identifyTime;
  ipd_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * �������ƣ�ipd_IdentifyQueryRspCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ���ѯ����
 *           �󱻵��á�
 * ��ڲ�����pRsp    ָ��ʶ���ѯ��Ӧ�ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�ipd_AlarmCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó��򱨾���������
 *           �󱻵��á�
 * ��ڲ�����pAlarm   ָ�򱨾�����ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_AlarmCB( zclAlarm_t *pAlarm )
{
  // ����������û�����
}


/*********************************************************************
 * @fn      ipd_GetCurrentPriceCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Get Current Price for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Current Price command
 * @param   srcAddr - source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
/*********************************************************************
 * �������ƣ�ipd_GetCurrentPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡ��ǰ�۸�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡ��ǰ�۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PRICING )
  /* �ڻ�ȡ��ǰ�۸�������豸Ӧ�����ʹ��е�ǰʱ����Ϣ�Ĺ����۸����� */  
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.priceTier = 0xfe;

  zclSE_Pricing_Send_PublishPrice( IPD_ENDPOINT, srcAddr, &cmd, false, seqNum );
#endif 
}


/*********************************************************************
 * �������ƣ�ipd_GetScheduledPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡԤ���۸�󱻵��á�
 * ��ڲ�����pCmd     ָ���ȡԤ���۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd, 
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  /* �ڻ�ȡԤ���۸�������豸��Ϊ��ǰԤ���۸��¼����͹����۸����� */ 
  /* ���Ӵ������� */

#if defined ( ZCL_PRICING )
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.priceTier = 0xfe;

  zclSE_Pricing_Send_PublishPrice( IPD_ENDPOINT, srcAddr, &cmd, false, seqNum );

#endif  
}


/*********************************************************************
 * �������ƣ�ipd_PublishPriceCB
 * ��    �ܣ�ZCL SE �����ļ����۴ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���չ����۸�󱻵��á�
 * ��ڲ�����pCmd     ָ�򹫲��۸�����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                afAddrType_t *srcAddr, uint8 seqNum )
{
  if ( pCmd )
  {
    /* ��ʾ�ṩ���豸ID�ֶ� */ 
    HalLcdWriteString("Provider ID", HAL_LCD_LINE_5);
    HalLcdWriteValue(pCmd->providerId, 10, HAL_LCD_LINE_6);
  }
}


/*********************************************************************
 * �������ƣ�ipd_DisplayMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ʾ��Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ����ʾ��Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( (char*)pCmd->msgString.pStr, HAL_LCD_LINE_6);
#endif  
}


/*********************************************************************
 * �������ƣ�ipd_CancelMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�ȡ����Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ��ȡ����Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�ipd_GetLastMessageCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ���ȡ�����Ϣ����󱻵��á�
 * ��ڲ�����srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum )
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

  zclSE_Message_Send_DisplayMessage( IPD_ENDPOINT, srcAddr, &cmd,
                                     false, seqNum );
#endif  
}


/*********************************************************************
 * �������ƣ�ipd_MessageConfirmationCB
 * ��    �ܣ�ZCL SE �����ļ���Ϣ�ؿ�ص������������յ�һ��Ӧ�ó���
 *           ���յ�ȷ����Ϣ����󱻵��á�
 * ��ڲ�����pCmd     ָ��ȷ����Ϣ����ṹ��ָ��
 *           srcAddr  ָ��Դ��ַָ��
 *           seqNum   ����������к�
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
 // ����������û�����
}


/* 
  ����ZCL������������/��Ӧ��Ϣ
 */
/*********************************************************************
 * �������ƣ�ipd_ProcessZCLMsg
 * ��    �ܣ�����ZCL����������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void ipd_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      ipd_ProcessInReadRspCmd( pInMsg );
      break;
#endif 

#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      ipd_ProcessInWriteRspCmd( pInMsg );
      break;
#endif 

    case ZCL_CMD_DEFAULT_RSP:
      ipd_ProcessInDefaultRspCmd( pInMsg );
      break;

#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      ipd_ProcessInDiscRspCmd( pInMsg );
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
 * �������ƣ�ipd_ProcessInReadRspCmd
 * ��    �ܣ����������ļ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 ipd_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�ipd_ProcessInWriteRspCmd
 * ��    �ܣ����������ļ�д��Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 ipd_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�ipd_ProcessInDefaultRspCmd
 * ��    �ܣ����������ļ�Ĭ����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 ipd_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  return TRUE;
}


#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * �������ƣ�ipd_ProcessInDiscRspCmd
 * ��    �ܣ����������ļ�������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 ipd_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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
