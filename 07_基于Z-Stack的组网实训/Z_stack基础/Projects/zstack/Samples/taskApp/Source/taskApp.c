/*******************************************************************************
 * �ļ����ƣ�TASKApp.c
 * ��    �ܣ�TASKAppʵ�顣
 * ʵ�����ݣ���2��Ϊ�����򴮿�"Hello ZIGBEE"��Ϣ��
*           LEB_G(��ɫ D5)�����򴮿ڷ���"Hello ZIGBEE"��Ϣʱ��LEDȡ��
*           ������� ��ADDTSAK": (��ɫ D4)��(��ɫ D3)�Ѳ�ͬƵ����˸��
 
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "taskApp.h"
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

/*
   ��Ӧ�õ�����ID��������SampleApp_Init()����������ʱ��
   �ñ������Ի������IDֵ��
*/
byte GenericApp_TaskID; 

#if defined (ADDTASK)
byte AddTask_ID;    //����ID
#define  AddTask_ev1      0x0001    //�¼�1
#define  AddTask_ev2      0x0002    //�¼�2
#endif

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
void GenericApp_SendTheMessage( void );
/********************************************************************/

#if defined (ADDTASK)
/*********************************************************************
 * �������ƣ�Addtask_Init
 * ��    �ܣ�Addtask_Init�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void AddTask_Init( byte task_id )
{
  AddTask_ID = task_id;   // ����
  /**����������**/
  osal_set_event( AddTask_ID, AddTask_ev1 );  //����������¼���־1
  osal_set_event( AddTask_ID, AddTask_ev2 );  //����������¼���־2
}
/*********************************************************************
 * �������ƣ�AddTask_Event
 * ��    �ܣ�AddTask_Event�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 AddTask_Event( byte task_id, UINT16 events )
{
     
  (void)task_id;  // ��ֹ����������
  
  if ( events & AddTask_ev1 )       //�¼�1
  {
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_TOGGLE ); //ȡ��led1
      //����ϵͳ��ʱ��1���������������¼���־1
      osal_start_timerEx(task_id, AddTask_ev1,1000); 
      return (events ^ AddTask_ev1);   // �������־
  }
  if ( events & AddTask_ev2 )       //�¼�1
  {
      HalLedSet ( HAL_LED_2, HAL_LED_MODE_TOGGLE ); //ȡ��led2
      //����ϵͳ��ʱ��2���������������¼���־2
      osal_start_timerEx(task_id, AddTask_ev2,2000); 
      return (events ^ AddTask_ev2);   // �������־
  }
  /* ����δ֪�¼� */
  return 0;
 }
#endif
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
 *           events   ׼��������¼����ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
       
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

        
        /* AF������Ϣ�¼� */ 
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
            /* ��������"Hello World"��Ϣ���¼�GENERICAPP_SEND_MSG_EVT */
            osal_start_timerEx( GenericApp_TaskID,
                                GENERICAPP_SEND_MSG_EVT,
                                100 );
            HalUARTWrite(HAL_UART_PORT_0,"  \r\n",4);
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );  // �ͷŴ洢��

      // ��ȡ��һ��ϵͳ��Ϣ�¼�
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    }

    return (events ^ SYS_EVENT_MSG);  // ����δ������¼�
  }

  /* ����"Hello World"��Ϣ���¼�GENERICAPP_SEND_MSG_EVT */
  /* ���¼���ǰ���"case ZDO_STATE_CHANGE:"���벿�ֿ��ܱ����� */
  if ( events & GENERICAPP_SEND_MSG_EVT )
  {
      // ���ʹ�����Ϣ�¼� 
      HAL_TOGGLE_LED4();  //�̵�ȡ��
      //�������ݵ�����
      HalUARTWrite(HAL_UART_PORT_0,"HELLO ZIGBEE!\r\n",15);
      /* �ٴδ�������"Hello World"��Ϣ���¼�GENERICAPP_SEND_MSG_EVT */
      osal_start_timerEx( GenericApp_TaskID,
                          GENERICAPP_SEND_MSG_EVT,
                          GENERICAPP_SEND_MSG_TIMEOUT );

    return (events ^ GENERICAPP_SEND_MSG_EVT);  // ����δ������¼�
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
 *           keys   �����¼������롣
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_HandleKeys( byte shift, byte keys )
{
   (void)shift;
  
  /* HAL_KEY_SW_1�����£� */
  
  if ( keys & HAL_KEY_SW_6)
  {
#if defined (KEY_TO_UART)
      HalUARTWrite(HAL_UART_PORT_0,"KEY1 PRESS!\r\n",13);
#endif          
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_TOGGLE );
        //ȡ��led 
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
   /* ��UART����ʾ���յ�����Ϣ */    
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
 * ��    �ܣ�����"Hello World"��Ϣ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void GenericApp_SendTheMessage( void )
{
  /* Ҫ���͵�Ӧ������"Hello World"*/
  char theMessageData[] = "Hello Zigbee!\r\n";

  /* ����"Hello World"��Ϣ */
  if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, 
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // ��������ִ�гɹ����û�������Լ���Ӧ�Ĵ������
    
    /* Ϩ��LED2 */
    
    HalLedSet ( HAL_LED_2, HAL_LED_MODE_TOGGLE );      
   
   // HalLedBlink( HAL_LED_2, 2, 50, 250 );   
  }
  else
  {
        HalLedBlink( HAL_LED_2, 0, 50, 250 );
      // ��������ִ��ʧ�ܣ��û�������Լ���Ӧ�Ĵ������
  }
}

