/*******************************************************************************
 * �ļ����ƣ�sapi.c
 * ��    �ܣ�SAPI����     
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "hal_drivers.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"

#if defined ( MT_TASK )
  #include "MT.h"
  #include "MT_TASK.h"
#endif

#include "nwk.h"
#include "APS.h"
#include "ZDApp.h"

#include "osal_nv.h"
#include "NLMEDE.h"
#include "AF.h"
#include "OnBoard.h"
#include "nwk_util.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "hal_led.h"
#include "hal_key.h"
#include "sapi.h"
#include "MT_SAPI.h"
/********************************************************************/


/* �ⲿ���� */
/********************************************************************/
extern uint8 zgStartDelay;
extern uint8 zgSapiEndpoint;
/********************************************************************/


/* �궨�� */
/* ����ֵ��Χ��0xE0-0xEF */
/********************************************************************/
/*
  �û���Ϣ
 */
#define ZB_USER_MSG                       0xE0

/*
  ��������ȷ��
 */
#define SAPICB_DATA_CNF                   0xE0

/*
  ��ȷ��
 */
#define SAPICB_BIND_CNF                   0xE1

/*
  �豸����ȷ��
 */
#define SAPICB_START_CNF                  0xE2
/********************************************************************/


/* ȫ�ֱ��� */
/********************************************************************/
/*
   �������飺���ÿ���������Ӧ�Ĵ�����ָ��
   ע�⣺�±��и����˳������������osalInitTasks()����
         �������ʼ�����õ�˳��һ��
*/
const pTaskEventHandlerFn tasksArr[] = {
  macEventLoop,               // MAC����������
  nwk_event_loop,             // �������������
  Hal_ProcessEvent,           // Ӳ���������������
#if defined( MT_TASK )
  MT_ProcessEvent,            // ��ز�����������
#endif
  APS_event_loop,             // Ӧ��֧���Ӳ���������
  ZDApp_event_loop,           // ZigBee�豸Ӧ����������
  SAPI_ProcessEvent           // �û�Ӧ�ò��������������û��Լ���д
};


/* ���ر��� */
/********************************************************************/
const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;
endPointDesc_t sapi_epDesc;
uint8 sapi_TaskID;
static uint16 sapi_bindInProgress;
/********************************************************************/


/* ���غ��� */
/********************************************************************/
void SAPI_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void SAPI_SendCback( uint8 event, uint8 status, uint16 data );
static void SAPI_StartConfirm( uint8 status );
static void SAPI_SendDataConfirm( uint8 handle, uint8 status );
static void SAPI_BindConfirm( uint16 commandId, uint8 status );
static void SAPI_FindDeviceConfirm( uint8 searchType,
                                        uint8 *searchKey, uint8 *result );
static void SAPI_ReceiveDataIndication( uint16 source,
                              uint16 command, uint16 len, uint8 *pData  );
static void SAPI_AllowBindConfirm( uint16 source );
/********************************************************************/



/*********************************************************************
 * �������ƣ�zb_SystemReset
 * ��    �ܣ�ϵͳ��λ
 * ��ڲ�������
 * ���ڲ������� 
 * �� �� ֵ����
 ********************************************************************/
void zb_SystemReset ( void )
{
  SystemReset();
}


/*********************************************************************
 * �������ƣ�zb_StartRequest
 * ��    �ܣ���ʼZigBeeЭ��ջ����Э��ջ���������ȴ�NV�ж�ȡ���ò�����
 *           Ȼ���豸��������硣
 * ��ڲ�������
 * ���ڲ������� 
 * �� �� ֵ����
 ********************************************************************/
void zb_StartRequest()
{
  uint8 logicalType;

  /* �����豸 */ 
  if ( zgStartDelay < NWK_START_DELAY )
    zgStartDelay = 0;
  else
    zgStartDelay -= NWK_START_DELAY;

  /* ����豸��������붨����� */
  zb_ReadConfiguration( ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType );
  if (  ( logicalType > ZG_DEVICETYPE_ENDDEVICE )        ||
#if defined( RTR_NWK )
  #if defined( ZDO_COORDINATOR )
          // ��·������Э������Ч 
         ( logicalType == ZG_DEVICETYPE_ENDDEVICE )   ||
  #else
          // ��·������Ч
          ( logicalType != ZG_DEVICETYPE_ROUTER )     ||
  #endif
#else
  #if defined( ZDO_COORDINATOR )
          // ����
          ( 1 )                                     ||
  #else
          // ���ն��豸��Ч
          ( logicalType != ZG_DEVICETYPE_ENDDEVICE )  ||
  #endif
#endif
          ( 0 ) )
   {
     /* �������� */
     SAPI_SendCback( SAPICB_START_CNF, ZInvalidParameter, 0 );
   }
   else
   {
     ZDOInitDevice(zgStartDelay);
   }

  return;
}


/*********************************************************************
 * �������ƣ�zb_BindDevice
 * ��    �ܣ�����2���豸�佨�����߽����
 * ��ڲ�����create       TRUE��ʾ�����󶨣�FLASH ��ʾ����� 
 *           commandId    ����ID���󶨱�ʶ��������ĳ����İ�
 *           pDestination �豸64λIEEE��ַ
 * ���ڲ������󶨲�����״̬   
 * �� �� ֵ����
 ********************************************************************/
void zb_BindDevice ( uint8 create, uint16 commandId, uint8 *pDestination )
{
  zAddrType_t destination;
  uint8 ret = ZB_ALREADY_IN_PROGRESS;

  if ( create )
  {
    if (sapi_bindInProgress == 0xffff) 
    {
      if ( pDestination ) // ��֪��չ��ַ��
      {
        destination.addrMode = Addr64Bit; 
        osal_cpyExtAddr( destination.addr.extAddr, pDestination );
        
        /* ����APS�������� */
        ret = APSME_BindRequest( sapi_epDesc.endPoint, commandId,
                                            &destination, sapi_epDesc.endPoint );

        if ( ret == ZSuccess )
        {
          /* ���������ַ */
          ZDP_NwkAddrReq(pDestination, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
          osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
        }
      }
      else // δ֪��չ��ַ��
      {
        ret = ZB_INVALID_PARAMETER;
        destination.addrMode = Addr16Bit;
        destination.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
        if ( ZDO_AnyClusterMatches( 1, &commandId, sapi_epDesc.simpleDesc->AppNumOutClusters,
                                                sapi_epDesc.simpleDesc->pAppOutClusterList ) )
        {
          /* �������ģʽ�£�����ƥ���豸 */
          ret = ZDP_MatchDescReq( &destination, NWK_BROADCAST_SHORTADDR,
              sapi_epDesc.simpleDesc->AppProfId, 1, &commandId, 0, (cId_t *)NULL, 0 );
        }
        else if ( ZDO_AnyClusterMatches( 1, &commandId, sapi_epDesc.simpleDesc->AppNumInClusters,
                                                sapi_epDesc.simpleDesc->pAppInClusterList ) )
        {
          ret = ZDP_MatchDescReq( &destination, NWK_BROADCAST_SHORTADDR,
              sapi_epDesc.simpleDesc->AppProfId, 0, (cId_t *)NULL, 1, &commandId, 0 );
        }

        if ( ret == ZB_SUCCESS )
        {
          /* ���ö�ʱ��ȷ������� */
#if ( ZG_BUILD_RTR_TYPE )
          osal_start_timerEx(sapi_TaskID, ZB_BIND_TIMER, AIB_MaxBindingTime);
#else
          /* AIB_MaxBindingTime δ������ */
          osal_start_timerEx(sapi_TaskID, ZB_BIND_TIMER, zgApsDefaultMaxBindingTime);
#endif
          sapi_bindInProgress = commandId;
          return;
        }
      }
    }

    SAPI_SendCback( SAPICB_BIND_CNF, ret, commandId );
  }
  else
  {
    /* ������ذ� */
    BindingEntry_t *pBind;

    /* ����� */
    while ( pBind = bindFind( sapi_epDesc.simpleDesc->EndPoint, commandId, 0 ) )
    {
      bindRemoveEntry(pBind);
    }
    osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
  }
  return;
}


/*********************************************************************
 * �������ƣ�zb_PermitJoiningRequest
 * ��    �ܣ������豸����������磬���߲������������
 * ��ڲ�����destination  �豸�ĵ�ַ��ͨ��Ϊ�����豸��ַ���ʾ����
 *                        Э��������·����������㲥��ַ��0xFFFC��
 *                        ͨ�����ַ�ʽ���Կ��Ƶ����豸�����������硣
 *           timeout      �������Ĺ涨ʱ�䣨��λΪ�룩
 *                        �����0x00,��ʾ�������������
 *                        �����0xff,��ʾһֱ�����������
 * ���ڲ�������   
 * �� �� ֵ��ZB_SUCCESS   �����������
 *           ����������� ���������
 ********************************************************************/
uint8 zb_PermitJoiningRequest ( uint16 destination, uint8 timeout )
{
#if defined( ZDO_MGMT_PERMIT_JOIN_REQUEST )
  zAddrType_t dstAddr;

  dstAddr.addrMode = Addr16Bit;
  dstAddr.addr.shortAddr = destination;

  return( (uint8) ZDP_MgmtPermitJoinReq( &dstAddr, timeout, 0, 0 ) );
#else
  (void)destination;
  (void)timeout;
  return ZUnsupportedMode;
#endif
}


/*********************************************************************
 * �������ƣ�zb_AllowBind
 * ��    �ܣ�ʹ�豸�ڹ涨��ʱ���ڴ��������ģʽ��
 * ��ڲ�����timeout    ����󶨵Ĺ涨ʱ�䣨��λΪ�룩
 * ���ڲ�����ZB_SUCCESS �豸���������ģʽ�����򱨴�
 * �� �� ֵ����
 ********************************************************************/
void zb_AllowBind ( uint8 timeout )
{
  osal_stop_timerEx(sapi_TaskID, ZB_ALLOW_BIND_TIMER);

  if ( timeout == 0 )
  {
    afSetMatch(sapi_epDesc.simpleDesc->EndPoint, FALSE);
  }
  else
  {
    afSetMatch(sapi_epDesc.simpleDesc->EndPoint, TRUE);
    if ( timeout != 0xFF )
    {
      if ( timeout > 64 )
      {
        timeout = 64;
      }
      osal_start_timerEx(sapi_TaskID, ZB_ALLOW_BIND_TIMER, timeout*1000);
    }
  }
  return;
}


/*********************************************************************
 * �������ƣ�zb_SendDataRequest
 * ��    �ܣ���������������
 * ��ڲ�����destination    ���������ݵ�Ŀ�ĵ�ַ���õ�ַΪ��
 ����������                 - �豸16λ�̵�ַ��0-0xFFFD��
 *                          - ZB_BROADCAST_ADDR �������е������豸�������ݡ�
 *                          - ZB_BINDING_ADDR ����ǰ�󶨵��豸�������ݡ�
 *           commandId      �����ݷ��͵�����ID�����ʹ��ZB_BINDING_ADDR
 *                          ������ͬ��������ָʾ�󶨱�ʹ�á�
 *           len            pData���ݳ���
 *           pData          ���͵�����ָ��
 *��         handle         ָʾ������������
 ��          txOptions      ���ΪTRUE����Ҫ���Ŀ�ĵ�ַ����Ӧ��
 ��          radius        ���������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_SendDataRequest ( uint16 destination, uint16 commandId, uint8 len,
                          uint8 *pData, uint8 handle, uint8 txOptions, uint8 radius )
{
  afStatus_t status;
  afAddrType_t dstAddr;

  txOptions |= AF_DISCV_ROUTE;

  /* ����Ŀ�ĵ�ַ */
  if (destination == ZB_BINDING_ADDR)
  {
    // ��
    dstAddr.addrMode = afAddrNotPresent;
  }
  else
  {
    // ʹ�ö̵�ַ
    dstAddr.addr.shortAddr = destination;
    dstAddr.addrMode = afAddr16Bit;

    if ( ADDR_NOT_BCAST != NLME_IsAddressBroadcast( destination ) )
    {
      txOptions &= ~AF_ACK_REQUEST;
    }
  }

  /* ���ö˵� */
  dstAddr.endPoint = sapi_epDesc.simpleDesc->EndPoint;

  /* ������Ϣ */
  status = AF_DataRequest(&dstAddr, &sapi_epDesc, commandId, len,
                          pData, &handle, txOptions, radius);

  if (status != afStatus_SUCCESS)
  {
    SAPI_SendCback( SAPICB_DATA_CNF, status, handle );
  }
}


/*********************************************************************
 * �������ƣ�zb_ReadConfiguration
 * ��    �ܣ���NV��ȡ������Ϣ��
 * ��ڲ�����configId    ������Ϣ��ʶ���� 
 *           len         ������Ϣ���ݳ���
 *           pValue      �洢������Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
uint8 zb_ReadConfiguration( uint8 configId, uint8 len, void *pValue )
{
  uint8 size;

  size = (uint8)osal_nv_item_len( configId );
  if ( size > len )
  {
    return ZFailure;
  }
  else
  {
    return( osal_nv_read(configId, 0, size, pValue) );
  }
}


/*********************************************************************
 * �������ƣ�zb_WriteConfiguration
 * ��    �ܣ���NVд��������Ϣ��
 * ��ڲ�����configId    ������Ϣ��ʶ���� 
 *           len         ������Ϣ���ݳ���
 *           pValue      �洢������Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
uint8 zb_WriteConfiguration( uint8 configId, uint8 len, void *pValue )
{
  return( osal_nv_write(configId, 0, len, pValue) );
}


/*********************************************************************
 * �������ƣ�zb_GetDeviceInfo
 * ��    �ܣ���ȡ�豸��Ϣ��
 * ��ڲ�����param    �豸��Ϣ��ʶ���� 
 *           pValue   �洢�豸��Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_GetDeviceInfo ( uint8 param, void *pValue )
{
  switch(param)
  {
    case ZB_INFO_DEV_STATE:
      osal_memcpy(pValue, &devState, sizeof(uint8));
      break;
    case ZB_INFO_IEEE_ADDR:
      osal_memcpy(pValue, &aExtendedAddress, Z_EXTADDR_LEN);
      break;
    case ZB_INFO_SHORT_ADDR:
      osal_memcpy(pValue, &_NIB.nwkDevAddress, sizeof(uint16));
      break;
    case ZB_INFO_PARENT_SHORT_ADDR:
      osal_memcpy(pValue, &_NIB.nwkCoordAddress, sizeof(uint16));
      break;
    case ZB_INFO_PARENT_IEEE_ADDR:
      osal_memcpy(pValue, &_NIB.nwkCoordExtAddress, Z_EXTADDR_LEN);
      break;
    case ZB_INFO_CHANNEL:
      osal_memcpy(pValue, &_NIB.nwkLogicalChannel, sizeof(uint8));
      break;
    case ZB_INFO_PAN_ID:
      osal_memcpy(pValue, &_NIB.nwkPanId, sizeof(uint16));
      break;
    case ZB_INFO_EXT_PAN_ID:
      osal_memcpy(pValue, &_NIB.extendedPANID, Z_EXTADDR_LEN);
      break;
  }
}


/*********************************************************************
 * �������ƣ�zb_FindDeviceRequest
 * ��    �ܣ�ȷ���������豸�Ķ̵�ַ���豸ͨ�����ñ���������ʼ������
 *           ������ͬ�����еĳ�Ա����������������ɺ�
 *           zb_FindDeviceConfirm�ص������������á�
 * ��ڲ�����searchType  �������͡� 
 *           searchKey   ����ֵ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zb_FindDeviceRequest( uint8 searchType, void *searchKey )
{
  if (searchType == ZB_IEEE_SEARCH)
  {
    ZDP_NwkAddrReq((uint8*) searchKey, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_StartConfirm
 * ��    �ܣ�����ȷ�Ϻ������������ڿ���һ���豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����status  ����������״ֵ̬�����ֵΪZB_SUCCESS��ָʾ����
 *           �豸�����ɹ���ɡ�������ǣ�����ʱmyStartRetryDelay��
 *           �����ϵͳ����MY_START_EVT�¼���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_StartConfirm( uint8 status )
{
   /*�Ƿ����˴��ڼ������ */
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_START_CNF ) )
  {
    zb_MTCallbackStartConfirm( status );
  }
  else
#endif
  {
    zb_StartConfirm( status );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_SendDataConfirm
 * ��    �ܣ���������ȷ�Ϻ������������ڷ������ݲ��������ZigBeeЭ��ջ
 *           �����á�
 * ��ڲ�����handle  ���ݴ���ʶ������
 *           status  ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_SendDataConfirm( uint8 handle, uint8 status )
{
  /*�Ƿ����˴��ڼ������ */  
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_SEND_DATA_CNF ) )
  {
    zb_MTCallbackSendDataConfirm( handle, status );
  }
  else
#endif
  {
    zb_SendDataConfirm( handle, status );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_BindConfirm
 * ��    �ܣ���ȷ�Ϻ������������ڰ󶨲��������ZigBeeЭ��ջ�����á�
 * ��ڲ�����commandId  ��ȷ���������ID��
 *           status     ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_BindConfirm( uint16 commandId, uint8 status )
{
  /*�Ƿ����˴��ڼ������ */
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_BIND_CNF ) )
  {
    zb_MTCallbackBindConfirm( commandId, status );
  }
  else
#endif
  {
    zb_BindConfirm( commandId, status );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_AllowBindConfirm
 * ��    �ܣ������ȷ�Ϻ�����ָʾ����һ���豸��ͼ�󶨵����豸��
 * ��ڲ�����source   ��Դ��ַ�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_AllowBindConfirm( uint16 source )
{
   /*�Ƿ����˴��ڼ������ */
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_ALLOW_BIND_CNF ) )
  {
    zb_MTCallbackAllowBindConfirm( source );
  }
  else
#endif
  {
    zb_AllowBindConfirm( source );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_FindDeviceConfirm
 * ��    �ܣ������豸ȷ�Ϻ������������ڲ����豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����searchType  �������͡� 
 *           searchKey   ����ֵ
 *           result      ���ҽ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result )
{
  /*�Ƿ����˴��ڼ������ */
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_FIND_DEV_CNF ) )
  {
    zb_MTCallbackFindDeviceConfirm( searchType, searchKey, result );
  }
  else
#endif
  {
    zb_FindDeviceConfirm( searchType, searchKey, result );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_ReceiveDataIndication
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
void SAPI_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{
  /*�Ƿ����˴��ڼ������ */
#if defined ( MT_SAPI_CB_FUNC )
  if ( SAPICB_CHECK( SPI_CB_SAPI_RCV_DATA_IND ) )
  {
    zb_MTCallbackReceiveDataIndication( source, command, len, pData  );
  }
  else
#endif
  {
    zb_ReceiveDataIndication( source, command, len, pData  );
  }
}


/*********************************************************************
 * �������ƣ�SAPI_ProcessEvent
 * ��    �ܣ�SimpleApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 SAPI_ProcessEvent( byte task_id, UINT16 events )
{
  osal_event_hdr_t *pMsg;
  afIncomingMSGPacket_t *pMSGpkt;
  afDataConfirm_t *pDataConfirm;

  if ( events & SYS_EVENT_MSG )
  {
    pMsg = (osal_event_hdr_t *) osal_msg_receive( task_id );
    while ( pMsg )
    {
      switch ( pMsg->event )
      {
        /* ZDO��Ϣ�����¼� */
        case ZDO_CB_MSG:
          SAPI_ProcessZDOMsgs( (zdoIncomingMsg_t *)pMsg );
          break;

        /* AF����ȷ���¼� */
        case AF_DATA_CONFIRM_CMD:
          /* �����ݰ����ͺ���ô����Ϣ���ᱻ���յ�����״̬ΪZStatus_t���� */
          /* ����Ϣ������AF.h�ļ��� */
          pDataConfirm = (afDataConfirm_t *) pMsg;
          SAPI_SendDataConfirm( pDataConfirm->transID, pDataConfirm->hdr.status );
          break;
       
        /* AF������Ϣ�¼� */ 
        case AF_INCOMING_MSG_CMD:
          pMSGpkt = (afIncomingMSGPacket_t *) pMsg;
          SAPI_ReceiveDataIndication( pMSGpkt->srcAddr.addr.shortAddr, pMSGpkt->clusterId,
                                    pMSGpkt->cmd.DataLength, pMSGpkt->cmd.Data);
          break;

        /* ZDO״̬�ı��¼� */  
        case ZDO_STATE_CHANGE:
          /* ���豸��Э������·�������ն� */
          if (pMsg->status == DEV_END_DEVICE ||
              pMsg->status == DEV_ROUTER ||
              pMsg->status == DEV_ZB_COORD )
          {
            SAPI_StartConfirm( ZB_SUCCESS );
          }
          else  if (pMsg->status == DEV_HOLD ||
                  pMsg->status == DEV_INIT)
          {
            SAPI_StartConfirm( ZB_INIT );
          }
          break;

        /* ZDOƥ����������Ӧ�¼� */  
        case ZDO_MATCH_DESC_RSP_SENT:
          SAPI_AllowBindConfirm( ((ZDO_MatchDescRspSent_t *)pMsg)->nwkAddr );
          break;

        /* �����¼� */
        case KEY_CHANGE:
          zb_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
          break;
          
        /* SAPI��������ȷ�ϻص��¼� */
        case SAPICB_DATA_CNF:
          SAPI_SendDataConfirm( (uint8)((sapi_CbackEvent_t *)pMsg)->data,
                                    ((sapi_CbackEvent_t *)pMsg)->hdr.status );
          break;

        /* SAPI��ȷ�ϻص��¼� */
        case SAPICB_BIND_CNF:
          SAPI_BindConfirm( ((sapi_CbackEvent_t *)pMsg)->data,
                              ((sapi_CbackEvent_t *)pMsg)->hdr.status );
          break;

        /* SAPI�豸����ȷ�ϻص��¼� */
        case SAPICB_START_CNF:
          SAPI_StartConfirm( ((sapi_CbackEvent_t *)pMsg)->hdr.status );
          break;

        default:
          if ( pMsg->event >= ZB_USER_MSG )
          {
            // �û����ڴ˴�����Լ��Ĵ������
          }
          break;
      }

      /* �ͷ��ڴ� */
      osal_msg_deallocate( (uint8 *) pMsg );
      
      /* ��ȡ��һ��ϵͳ��Ϣ�¼� */
      pMsg = (osal_event_hdr_t *) osal_msg_receive( task_id );
    }

    /* ����δ������¼� */
    return (events ^ SYS_EVENT_MSG);
  }

  /* ����󶨶�ʱ���¼� */
  if ( events & ZB_ALLOW_BIND_TIMER )
  {
    afSetMatch(sapi_epDesc.simpleDesc->EndPoint, FALSE);
    return (events ^ ZB_ALLOW_BIND_TIMER);
  }

  /* �󶨶�ʱ���¼� */
  if ( events & ZB_BIND_TIMER )
  {
    /* ��Ӧ�ó����Ͱ�ȷ�ϻص����� */
    SAPI_BindConfirm( sapi_bindInProgress, ZB_TIMEOUT );
    sapi_bindInProgress = 0xffff;

    return (events ^ ZB_BIND_TIMER);
  }

  /* �û���������¼� */
  if ( events & ZB_ENTRY_EVENT )
  {
    uint8 startOptions;

    /* ִ�в���ϵͳ�¼������� */
    zb_HandleOsalEvent( ZB_ENTRY_EVENT );

    /* �ر�LED4 */
    //HalLedSet (HAL_LED_4, HAL_LED_MODE_OFF);

    zb_ReadConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions );
    if ( startOptions & ZCD_STARTOPT_AUTO_START )
    {
      zb_StartRequest(); // �����豸������/��������
    }
    else
    {
      /* ��˸LED2,�ȴ��ⲿ�������벢���������ȴ����������ã� */
      HalLedBlink(HAL_LED_2, 0, 50, 500);
      //HalLedBlink(HAL_LED_2, 0, 50, 200);  
    }

    return (events ^ ZB_ENTRY_EVENT );
  }

  /* �û��Զ����¼� */
  if ( events & ( ZB_USER_EVENTS ) )
  {
    /* ִ�в���ϵͳ�¼������� */
    zb_HandleOsalEvent( events );
  }

  return 0;
}


/*********************************************************************
 * �������ƣ�SAPI_ProcessZDOMsgs
 * ��    �ܣ�SimpleApp��ZDO��Ϣ�����¼���������
 * ��ڲ�����inMsg  ������Ϣ
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case NWK_addr_rsp:
      {
        /* ���������豸��Ӧ�ó��� */
        ZDO_NwkIEEEAddrResp_t *pNwkAddrRsp = ZDO_ParseAddrRsp( inMsg );
        SAPI_FindDeviceConfirm( ZB_IEEE_SEARCH, (uint8*)&pNwkAddrRsp->nwkAddr, pNwkAddrRsp->extAddr );
      }
      break;

    case Match_Desc_rsp:
      {
        zAddrType_t dstAddr;
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );

        if ( sapi_bindInProgress != 0xffff )
        {
          /* �����󶨱���� */
          dstAddr.addrMode = Addr16Bit;
          dstAddr.addr.shortAddr = pRsp->nwkAddr;

          if ( APSME_BindRequest( sapi_epDesc.simpleDesc->EndPoint,
                     sapi_bindInProgress, &dstAddr, pRsp->epList[0] ) == ZSuccess )
          {
            osal_stop_timerEx(sapi_TaskID,  ZB_BIND_TIMER);
            osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );

            /* ��ѯIEEE��ַ */
            ZDP_IEEEAddrReq( pRsp->nwkAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );

            /* ���Ͱ�ȷ�ϵ�Ӧ�ó��� */
            zb_BindConfirm( sapi_bindInProgress, ZB_SUCCESS );
            sapi_bindInProgress = 0xffff;
          }
        }
      }
      break;
  }
}


/*********************************************************************
 * �������ƣ�SAPI_Init
 * ��    �ܣ�SimpleApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_Init( byte task_id )
{
  uint8 startOptions;

  sapi_TaskID = task_id;

  sapi_bindInProgress = 0xffff;

  /* ���˵������� */ 
  sapi_epDesc.endPoint = zb_SimpleDesc.EndPoint;
  sapi_epDesc.task_id = &sapi_TaskID;
  sapi_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&zb_SimpleDesc;
  sapi_epDesc.latencyReq = noLatencyReqs;

  /* ע��˵������� */
  afRegister( &sapi_epDesc );

  /* Ĭ�Ϲر�ƥ��������Ӧ */
  afSetMatch(sapi_epDesc.simpleDesc->EndPoint, FALSE);

  /* ע��ص��¼� */
  ZDO_RegisterForZDOMsg( sapi_TaskID, NWK_addr_rsp );
  ZDO_RegisterForZDOMsg( sapi_TaskID, Match_Desc_rsp );

#if (defined HAL_KEY) && (HAL_KEY == TRUE)
  /* ע��Ӳ������㰴���¼� */
  RegisterForKeys( sapi_TaskID );

  /* �ж�SW5�Ƿ񱻰��� */
  /* SK-SmartRF05EB��SW5��Ӧ��CENTER�� */
  //if ( HalKeyRead () == HAL_KEY_SW_5)
  //{
    /* ��������������а�סSW5���ţ���ô��ǿ������豸״̬�����ú������豸 */
  //  startOptions = ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG;
  //  zb_WriteConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions );
 //   zb_SystemReset();
 // }
#endif

  /* �����¼�����Ӧ�ó��� */
  osal_set_event(task_id, ZB_ENTRY_EVENT);
}


/*********************************************************************
 * �������ƣ�SAPI_SendCback
 * ��    �ܣ�������Ϣ��SAPI�������������Ա�����ص���
 * ��ڲ�����event  ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 *           status ״̬��־λ
 *           data   ���� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void SAPI_SendCback( uint8 event, uint8 status, uint16 data )
{
  sapi_CbackEvent_t *pMsg;

  pMsg = (sapi_CbackEvent_t *)osal_msg_allocate( sizeof(sapi_CbackEvent_t) );
  if( pMsg )
  {
    pMsg->hdr.event = event;
    pMsg->hdr.status = status;
    pMsg->data = data;

    osal_msg_send( sapi_TaskID, (uint8 *)pMsg );
  }
}


/*********************************************************************
 * �������ƣ�osalInitTasks
 * ��    �ܣ���ʼ��ϵͳ�е�ÿһ������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void osalInitTasks( void )
{
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++ );
  nwk_init( taskID++ );
  Hal_Init( taskID++ );
#if defined( MT_TASK )
  MT_TaskInit( taskID++ );
#endif
  APS_Init( taskID++ );
  ZDApp_Init( taskID++ );
  SAPI_Init( taskID );
}


