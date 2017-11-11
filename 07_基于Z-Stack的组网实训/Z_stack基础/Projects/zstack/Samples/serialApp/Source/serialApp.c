/*******************************************************************************
 * �ļ����ƣ�GenericApp.c
 * ��    �ܣ�GenericAppʵ�顣
 * ʵ�����ݣ�һ��ZigBee�����е�ĳ�����豸ͨ�����ն��豸�����󡱻�ƥ��������
 *           ���󡱽���Ӧ�ò���߼����Ӻ���5��Ϊ���ڻ���"HelloZIGBEE"��Ϣ��
 * ʵ���豸��
          һ��PCͨ����������һ��ʹ�ñ�Ӧ��ʵ����ZigBee�豸���շ����ݣ���һ��PCͨ
������������һ��ʹ�ñ�Ӧ��ʵ����ZigBee�豸���շ����ݣ� ʵ����pc�˵Ĵ���˫��ͨѶ����ʵ����ʾ����zigbee�豸��ʵ�����ߴ������ݴ���ķ�����

 *           LED_R(��ɫ D6)�����ɹ�������������������LED����
 *           LEB_G(��ɫ D5)��������һ�豸�ɹ�����Ӧ�ò���߼����Ӻ��LED����
*           LED_B(��ɫ D4): ������"Hello ZIGBEE"��Ϣʱ��LEDȡ��
 *          LED_B(��ɫ D3)���������յ�"Hello World"��Ϣʱ��LED��˸
 *           
  ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "serialApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
/********************************************************************/


/* ��ID�б� */
/* ���б�Ӧ���ñ�Ӧ��ר�õĴ�ID����� */
const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID
};


/* �豸�ļ������� */
const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,              //  �˵��
  GENERICAPP_PROFID,                //  Profile ID
  GENERICAPP_DEVICEID,              //  �豸ID
  GENERICAPP_DEVICE_VERSION,        //  �豸�汾
  GENERICAPP_FLAGS,                 //  ��ʶ
  GENERICAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)GenericApp_ClusterList,  //  ������б�
  GENERICAPP_MAX_CLUSTERS,          //  ����ص�����
  (cId_t *)GenericApp_ClusterList   //  ������б�
};


/* �豸�Ķ˵������� */
/* �����ڴ˴��������豸�Ķ˵�������������������GenericApp_Init()����
   �жԸ�������������䡣
   ����Ҳ�����ڴ˴���"const"�ؼ��������岢���һ���˵��������Ľṹ��
   ������
   �����ԣ����Ǵ˴����õ�һ�ַ�ʽ�����仰˵��������RAM�ﶨ���˶˵���
   ������
*/
endPointDesc_t GenericApp_epDesc;


/* ���ر��� */
/********************************************************************/
uint8   rbuf[256], sbuf[256];
/*
   ��Ӧ�õ�����ID��������SampleApp_Init()����������ʱ��
   �ñ������Ի������IDֵ��
*/
byte GenericApp_TaskID;   


/*
   �豸������״̬������Э����/·����/�ն��豸/�豸����������
*/
devStates_t GenericApp_NwkState;


/*
   ����ID��������������
*/
byte GenericApp_TransID;  


/*
   "Hello World"��Ϣ��Ŀ�ĵ�ַ�ṹ�����
*/
afAddrType_t GenericApp_DstAddr;
/********************************************************************/


/* ���غ��� */
/********************************************************************/
void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void GenericApp_HandleKeys( byte shift, byte keys );
void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void GenericApp_SendTheMessage(byte *buf, uint8 len);
uint16  uart_read(uint8 *buf, uint16 maxlen);
/********************************************************************/


/*********************************************************************
 * �������ƣ�GenericApp_Init
 * ��    �ܣ�GenericApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_Init( byte task_id )
{
  GenericApp_TaskID = task_id;
  GenericApp_NwkState = DEV_INIT;
  GenericApp_TransID = 0;

  /* 
     �����ڴ˴���Ӷ��豸Ӳ���ĳ�ʼ�����롣 
     ��Ӳ���Ǳ�Ӧ��ר�õģ����Խ��Ը�Ӳ���ĳ�ʼ����������ڴ˴��� 
     ��Ӳ�����Ǳ�Ӧ��ר�õģ����Խ���ʼ�����������Zmain.c�ļ��е�main()������
  */

  /* ��ʼ��"Hello World"��Ϣ��Ŀ�ĵ�ַ */
  GenericApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  GenericApp_DstAddr.endPoint = 0;
  GenericApp_DstAddr.addr.shortAddr = 0;

  /* ���˵������� */
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;

  /* ע��˵������� */
  afRegister( &GenericApp_epDesc );

  /* ע�ᰴ���¼��������а����¼����͸���Ӧ������GenericApp_TaskID */
  RegisterForKeys( GenericApp_TaskID );

  /* ��������LCD_SUPPORTED����ѡ�����LCD�Ͻ�����Ӧ����ʾ */
#if defined ( LCD_SUPPORTED )
#if defined ( ZIGBEEPRO )
    HalLcdWriteString( "GenericApp(ZigBeePRO)", HAL_LCD_LINE_2 );
#else
    HalLcdWriteString( "GenericApp(ZigBee07)", HAL_LCD_LINE_2 );
#endif
#endif
  
  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�End_Device_Bind_rsp�����յ���End_Device_Bind_rsp�¼�
     ���͸���Ӧ������GenericApp_TaskID
   */
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, End_Device_Bind_rsp );
  
  /* ZDO��Ϣע�� */
  /* ע��ZDO�Ĵ�Match_Desc_rsp�����յ���Match_Desc_rsp�¼����͸���Ӧ
     ������GenericApp_TaskID
   */  
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, Match_Desc_rsp );
}


/*********************************************************************
 * �������ƣ�GenericApp_ProcessEvent
 * ��    �ܣ�GenericApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;
  uint16 len;
  /* �������ڱ���AF����ȷ����Ϣ���ֶ�ֵ�ı��� */
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       
  (void)task_id;  // ��ֹ����������

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        /* ZDO��Ϣ�����¼� */
        case ZDO_CB_MSG:
          // ����ZDO��Ϣ�����¼�������
          GenericApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
        
        /* �����¼� */
        case KEY_CHANGE:
          GenericApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, 
                                 ((keyChange_t *)MSGpkt)->keys );
          break;

        /* AF����ȷ���¼� */
        case AF_DATA_CONFIRM_CMD:
          // ��ȡAF����ȷ����Ϣ������Ӧ����
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // ��AF����ȷ����Ϣ�����ղŷ��͵����ݲ��ɹ�������Ӧ����
          if ( sentStatus != ZSuccess )
          {
            // �û����ڴ˴�����Լ��Ĵ������
          }
          break;

        /* RF������Ϣ�¼� */ 
        /*���յ�ZIGBEE����*/  
        case AF_INCOMING_MSG_CMD:
          GenericApp_MessageMSGCB( MSGpkt );  // ����������Ϣ�¼�������
          break;

        /* ZDO״̬�ı��¼� */  
        case ZDO_STATE_CHANGE:
          GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);// ��ȡ�豸״̬
          /* ���豸��Э������·�������ն�  */
          if ( (GenericApp_NwkState == DEV_ZB_COORD)
              || (GenericApp_NwkState == DEV_ROUTER)
              || (GenericApp_NwkState == DEV_END_DEVICE) )
          {
            osal_start_timerEx( GenericApp_TaskID,GENERICAPP_SEND_MSG_EVT, 4000);
            HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
          }
      }
      osal_msg_deallocate( (uint8 *)MSGpkt );  // �ͷŴ洢��

      // ��ȡ��һ��ϵͳ��Ϣ�¼�
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    }

    return (events ^ SYS_EVENT_MSG);  // ����δ������¼�
  }

  /* �¼�GENERICAPP_SEND_MSG_EVT */
  /* ���¼���ǰ���"case ZDO_STATE_CHANGE:"���벿�ֿ��ܱ����� */
  if ( events & GENERICAPP_SEND_MSG_EVT )
  {
        /* ������GENERICAPP_READ_UART_EVT��Ϣ���¼� */
            osal_start_timerEx( GenericApp_TaskID,GENERICAPP_READ_UART_EVT, 100);
            GenericApp_HandleKeys( 0,HAL_KEY_SW_6 );
      return (events ^ GENERICAPP_SEND_MSG_EVT);  // ����δ������¼�
  }
   /* �¼�GENERICAPP_SEND_MSG_EVT */
  /* ���¼����մ������ݣ���ͨ��zigbee����*/
  if ( events & GENERICAPP_READ_UART_EVT )
  {
      /* �ý��մ�������*/
      len=uart_read(rbuf,256);
    if(len) 
    {   //�д�������
        osal_memcpy( sbuf, rbuf, len);
 #if defined UARTSEL        
        HalUARTWrite(HAL_UART_PORT_0, sbuf, len);
#endif
        //ͨ��zigbee����
        GenericApp_SendTheMessage(sbuf,  len);
    }
     /* �ٴδ���GENERICAPP_READ_UART_EVT */
    osal_start_timerEx( GenericApp_TaskID,GENERICAPP_READ_UART_EVT, 20);
  }
 
  /* ����δ֪�¼� */
  return 0;
}


/*********************************************************************
 * �������ƣ�GenericApp_ProcessZDOMsgs
 * ��    �ܣ�GenericApp��ZDO��Ϣ�����¼���������
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  /* ZDO��Ϣ�еĴ�ID */
  switch ( inMsg->clusterID )
  {
    /* ZDO�Ĵ�End_Device_Bind_rsp */
    case End_Device_Bind_rsp:
      /* ���󶨳ɹ� */
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
      
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
            /* ����"Hello World"��Ϣ��Ŀ�ĵ�ַ */
            GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            GenericApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            
            // ���õ�һ���˵㣬�û������޸Ĵ˴��������������ж˵�
            GenericApp_DstAddr.endPoint = pRsp->epList[0];

            /* ����LED4 */
            
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );  // �ͷ�ָ�����pRsp��ռ�õĴ洢��
        }
      }
      break;
  }
}


/*********************************************************************
 * �������ƣ�GenericApp_HandleKeys
 * ��    �ܣ�GenericApp�İ����¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ�����ĿΪ��
 *                  HAL_KEY_SW_2     SK-SmartRF05EB��RIGHT��
 *                  HAL_KEY_SW_4     SK-SmartRF05EB��LEFT��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;
  (void)shift;
  
  /* HAL_KEY_SW_1�����£����͡��ն��豸������ */
  
  if ( keys & HAL_KEY_SW_6)
  {
#if defined (KEY_TO_UART)
      HalUARTWrite(HAL_UART_PORT_0,"KEY1 PRESS!\r\n",13);
#endif          
      

    /* Ϊǿ�ƶ˵㷢��һ�����ն��豸�����󡱣�������*/
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
    dstAddr.addrMode = Addr16Bit;
    dstAddr.addr.shortAddr = 0x0000; // Ŀ�ĵ�ַΪ������Э������16λ�̵�ַ0x0000
    ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(), 
                            GenericApp_epDesc.endPoint,
                            GENERICAPP_PROFID,
                            GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                            GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                            FALSE );

  }

  /* HAL_KEY_SW_2�����£����͡�ƥ������������ */
   
  if ( keys &HAL_KEY_SW_7 )
  {
#if defined  (KEY_TO_UART)
      HalUARTWrite(HAL_UART_PORT_0,"KEY2 PRESS!\r\n",13);
#endif   
     
      
 
    /* ����һ����ƥ�����������󡱣��㲥��*/
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
    dstAddr.addrMode = AddrBroadcast;
    dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
    ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        GENERICAPP_PROFID,
                        GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                        GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                        FALSE );
   
  }
}


/*********************************************************************
 * �������ƣ�GenericApp_MessageMSGCB
 * ��    �ܣ�GenericApp��������Ϣ�¼����������������������������豸��
 *           �κ��������ݡ���ˣ��ɻ��ڴ�IDִ��Ԥ���Ķ�����
 * ��ڲ�����pkt  ָ��������Ϣ��ָ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  /* AF������Ϣ�еĴ� */
  switch ( pkt->clusterId )
  {
    /* ��GENERICAPP_CLUSTERID */
    case GENERICAPP_CLUSTERID:
   
      /* Ϩ��LED1 */
      /* ����SK-SmartRF05EB���ԣ�LED1��LED_G����ɫ�� */
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );      
      
      /* ��˸LED1 */
      HalLedBlink( HAL_LED_1, 2, 50, 250 ); 
   /* UART����ʾ���յ�����Ϣ */    
      HalUARTWrite(HAL_UART_PORT_0,pkt->cmd.Data, osal_strlen(pkt->cmd.Data));
    /* ��������LCD_SUPPORTED����ѡ�����LCD����ʾ���յ�����Ϣ */      
#if defined( LCD_SUPPORTED )
      HalLcdWriteString( (char*)pkt->cmd.Data, HAL_LCD_LINE_4 );
      HalLcdWriteString( "rcvd", HAL_LCD_LINE_5 );
/* ��������WIN32����ѡ�����IAR���Ի����´�ӡ�����յ�����Ϣ */
#elif defined( WIN32 )
      WPRINTSTR( pkt->cmd.Data );
#endif
    break;
  }
}


/*********************************************************************
 * �������ƣ�GenericApp_SendTheMessage
 * ��    �ܣ�������Ϣ��
 * ��ڲ�����buf, len 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_SendTheMessage(byte *buf, uint8 len)
{
  
  if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       len + 1,
                       (byte *)buf,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, 
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // ��������ִ�гɹ����û�������Լ���Ӧ�Ĵ������
    
       HalLedSet ( HAL_LED_2, HAL_LED_MODE_TOGGLE );      
         
  }
  else
  {
        HalLedBlink( HAL_LED_2, 0, 50, 250 );
      // ��������ִ��ʧ�ܣ��û�������Լ���Ӧ�Ĵ������
  }
}

/**********************************************************
func: ���մ������ݰ�
������
      1. *buf -- ���ڽ�����������ŵĵط�
      2. maxlen -- ���ڽ������ݵ���󳤶�
retuen: uint16 ���յ������ݳ���
***********************************************************/
uint16  uart_read(uint8 *buf, uint16 maxlen)
{
    uint16 slen=0; 
    uint8 dtime=0;
    while(dtime<20)
    {    
        while(HalUARTRead(HAL_UART_PORT_0,buf+slen, 1))
        {
            slen++;
            if(slen>=maxlen)
                return  slen;
            dtime=0;
        }
        MicroWait (30);     // Wait 30us; 
        dtime++;
    }    
   return  slen; 
}
    
