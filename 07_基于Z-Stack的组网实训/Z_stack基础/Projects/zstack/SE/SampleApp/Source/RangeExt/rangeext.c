/*******************************************************************************
 * �ļ����ƣ�rangeext.c
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
#include "rangeext.h"
#include "zcl_general.h"
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
static uint8 rangeExtTaskID;              // ����������ϵͳ����ID
static devStates_t rangeExtNwkState;      // ����״̬����
static uint8 rangeExtTransID;             // ����id
static afAddrType_t ESPAddr;              // ESPĿ�ĵ�ַ
static uint8 linkKeyStatus;               // �ӻ�ȡ������Կ�������ص�״̬����


/* ���غ��� */
/********************************************************************/
static void rangeext_HandleKeys( uint8 shift, uint8 keys );

#if defined ( ZCL_KEY_ESTABLISH )
static uint8 rangeext_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif 

#if defined ( ZCL_ALARMS )
static void rangeext_ProcessAlarmCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                      uint16 clusterID, zclFrameHdr_t *hdr, 
                                      uint8 len, uint8 *data );
#endif 

static void rangeext_ProcessIdentifyTimeChange( void );
/********************************************************************/


/* ���غ��� - Ӧ�ûص����� */
/********************************************************************/
/*
   �����ص�����
*/
static uint8 rangeext_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

/*
   ͨ�ôػص�����
*/
static void rangeext_BasicResetCB( void );
static void rangeext_IdentifyCB( zclIdentify_t *pCmd );
static void rangeext_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void rangeext_AlarmCB( zclAlarm_t *pAlarm );

/* ����ZCL�������� - ��������/��Ӧ��Ϣ */
/********************************************************************/
static void rangeext_ProcessZCLMsg( zclIncomingMsg_t *msg );

#if defined ( ZCL_READ )
static uint8 rangeext_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif

#if defined ( ZCL_WRITE )
static uint8 rangeext_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif 

static uint8 rangeext_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

#if defined ( ZCL_DISCOVER )
static uint8 rangeext_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif 
/********************************************************************/

/*
   ZCLͨ�ôػص���
*/
static zclGeneral_AppCallbacks_t rangeext_GenCmdCallbacks =
{
  rangeext_BasicResetCB,         // �����ظ�λ����
  rangeext_IdentifyCB,           // ʶ������
  rangeext_IdentifyQueryRspCB,   // ʶ���ѯ��Ӧ����
  NULL,                          // ���ش�����
  NULL,                          // ��������뿪����
  NULL,                          // ��������ƶ�����
  NULL,                          // ������Ƶ�������
  NULL,                          // �������ֹͣ����
  NULL,                          // ����Ӧ����
  NULL,                          // ����������������
  NULL,                          // �����ص�����������
  NULL,                          // ������Ӧ����
  rangeext_AlarmCB,              // ����(��Ӧ)����
  NULL,                          // RSSI��λ����
  NULL,                          // RSSI��λ��Ӧ����
};


/*********************************************************************
 * �������ƣ�rangeext_Init
 * ��    �ܣ��������ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void rangeext_Init( uint8 task_id )
{
  rangeExtTaskID = task_id;

  // ����ESPĿ�ĵ�ַ
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = RANGEEXT_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  /* ע��һ��SE�˵� */
  zclSE_Init( &rangeExtSimpleDesc );

  /* ע��ZCLͨ�ôؿ�ص����� */
  zclGeneral_RegisterCmdCallbacks( RANGEEXT_ENDPOINT, &rangeext_GenCmdCallbacks );

  /* ע��Ӧ�ó��������б� */
  zcl_registerAttrList( RANGEEXT_ENDPOINT, RANGEEXT_MAX_ATTRIBUTES, rangeExtAttrs );

  /* ע��Ӧ�ó����ѡ���б� */
  zcl_registerClusterOptionList( RANGEEXT_ENDPOINT, RANGEEXT_MAX_OPTIONS, rangeExtOptions );

  /* ע��Ӧ�ó�������������֤�ص����� */
  zcl_registerValidateAttrData( rangeext_ValidateAttrDataCB );

  /* ע��Ӧ�ó���������δ�����������/��Ӧ��Ϣ */
  zcl_registerForMsg( rangeExtTaskID );

  /* ע�����а����¼� - ���������а����¼� */
  RegisterForKeys( rangeExtTaskID );
}


/*********************************************************************
 * �������ƣ�rangeext_event_loop
 * ��    �ܣ��������������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
uint16 rangeext_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( rangeExtTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          /* ����ZCL��������/��Ӧ��Ϣ */
          rangeext_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          /* �������� */
          rangeext_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          rangeExtNwkState = (devStates_t)(MSGpkt->hdr.status);

          if (ZG_SECURE_ENABLED)
          {
            if ( rangeExtNwkState == DEV_ROUTER )
            {
              /* ���������Կ�Ƿ��Ѿ������� */
              linkKeyStatus = rangeext_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

              if (linkKeyStatus != ZSuccess)
              {
                /* ������Կ�������� */
                osal_set_event( rangeExtTaskID, RANGEEXT_KEY_ESTABLISHMENT_REQUEST_EVT);
              }
            }
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt ); // �ͷŴ洢��
    }

    return (events ^ SYS_EVENT_MSG); // ����δ������¼�
  }

  /* ��ʼ����Կ���������¼� */
  if ( events & RANGEEXT_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(rangeExtTaskID, &ESPAddr, rangeExtTransID);

    return ( events ^ RANGEEXT_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  /* ��һ��ʶ���������ʶ��ʱ�¼����� */
  if ( events & RANGEEXT_IDENTIFY_TIMEOUT_EVT )
  {
    if ( rangeExtIdentifyTime > 0 )
    {
      rangeExtIdentifyTime--;
    }
    rangeext_ProcessIdentifyTimeChange();

    return ( events ^ RANGEEXT_IDENTIFY_TIMEOUT_EVT );
  }

  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�rangeext_ProcessIdentifyTimeChange
 * ��    �ܣ�������˸LED��ָ��ʶ��ʱ������ֵ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_ProcessIdentifyTimeChange( void )
{
  if ( rangeExtIdentifyTime > 0 )
  {
    osal_start_timerEx( rangeExtTaskID, RANGEEXT_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( rangeExtTaskID, RANGEEXT_IDENTIFY_TIMEOUT_EVT );
  }
}

#if defined ( ZCL_KEY_ESTABLISH )
/*********************************************************************
 * �������ƣ�rangeext_KeyEstablish_ReturnLinkKey
 * ��    �ܣ���ȡ���������Կ������
 * ��ڲ�����shortAddr �̵�ַ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 rangeext_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
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
 * �������ƣ�rangeext_HandleKeys
 * ��    �ܣ��������İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_2     SK-SmartRF10BB��K2��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_2 )
  {
    ZDOInitDevice(0); // ��������
  }
}


/*********************************************************************
 * �������ƣ�rangeext_ValidateAttrDataCB
 * ��    �ܣ����Ϊ�����ṩ������ֵ�Ƿ��ڸ�����ָ���ķ�Χ�ڡ�
 * ��ڲ�����pAttr     ָ�����Ե�ָ��
 *           pAttrInfo ָ��������Ϣ��ָ��            
 * ���ڲ�������
 * �� �� ֵ��TRUE      ��Ч����
 *           FALSE     ��Ч����
 ********************************************************************/
static uint8 rangeext_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
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
 * �������ƣ�rangeext_BasicResetCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص��������������дص���������Ϊ����Ĭ�����á�
 * ��ڲ�������          
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_BasicResetCB( void )
{
  // �ڴ���Ӵ���
}


/*********************************************************************
 * �������ƣ�rangeext_IdentifyCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ������󱻵��á�
 * ��ڲ�����pCmd      ָ��ʶ������ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_IdentifyCB( zclIdentify_t *pCmd )
{
  rangeExtIdentifyTime = pCmd->identifyTime;
  rangeext_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * �������ƣ�rangeext_IdentifyQueryRspCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó���ʶ���ѯ����
 *           �󱻵��á�
 * ��ڲ�����pRsp    ָ��ʶ���ѯ��Ӧ�ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // ����������û�����
}


/*********************************************************************
 * �������ƣ�rangeext_AlarmCB
 * ��    �ܣ�ZCLͨ�ôؿ�ص������������յ�һ��Ӧ�ó��򱨾���������
 *           �󱻵��á�
 * ��ڲ�����pAlarm   ָ�򱨾�����ṹ��ָ��       
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_AlarmCB( zclAlarm_t *pAlarm )
{
  // ����������û�����
}


/* 
  ����ZCL������������/��Ӧ��Ϣ
 */
/*********************************************************************
 * �������ƣ�rangeext_ProcessZCLMsg
 * ��    �ܣ�����ZCL����������Ϣ����
 * ��ڲ�����inMsg           ���������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void rangeext_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      rangeext_ProcessInReadRspCmd( pInMsg );
      break;
#endif 

#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      rangeext_ProcessInWriteRspCmd( pInMsg );
      break;
#endif 

    case ZCL_CMD_DEFAULT_RSP:
      rangeext_ProcessInDefaultRspCmd( pInMsg );
      break;

#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      rangeext_ProcessInDiscRspCmd( pInMsg );
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
 * �������ƣ�rangeext_ProcessInReadRspCmd
 * ��    �ܣ����������ļ�����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 rangeext_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�rangeext_ProcessInWriteRspCmd
 * ��    �ܣ����������ļ�д��Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static uint8 rangeext_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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
 * �������ƣ�rangeext_ProcessInDefaultRspCmd
 * ��    �ܣ����������ļ�Ĭ����Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 rangeext_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  return TRUE;
}


#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * �������ƣ�rangeext_ProcessInDiscRspCmd
 * ��    �ܣ����������ļ�������Ӧ�����
 * ��ڲ�����inMsg           �������������Ϣ
 * ���ڲ�������
 * �� �� ֵ��TRUE
 ********************************************************************/
static uint8 rangeext_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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
