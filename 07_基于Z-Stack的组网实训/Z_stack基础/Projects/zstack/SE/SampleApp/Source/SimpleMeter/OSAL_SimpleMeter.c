/*******************************************************************************
 * �ļ����ƣ�OSAL_SimpleMeter.c
 * ��    �ܣ��򵥼����������ʼ����
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "hal_drivers.h"
#include "OSAL_Tasks.h"

#if defined ( MT_TASK )
  #include "MT_TASK.h"
#endif

#include "ZDApp.h"
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  #include "ZDNwkMgr.h"
#endif
#if defined ( ZIGBEE_FRAGMENTATION )
  #include "aps_frag.h"
#endif

#if defined ( ZCL_KEY_ESTABLISH )
  #include "zcl_key_establish.h"
#endif

#include "simplemeter.h"
/********************************************************************/



/* ȫ�ֱ��� */
/********************************************************************/
/*
   �������飺���ÿ���������Ӧ�Ĵ�����ָ��
   ע�⣺�±��и����˳������������osalInitTasks()����
         �������ʼ�����õ�˳��һ��
*/
const pTaskEventHandlerFn tasksArr[] = {
  macEventLoop,                 // MAC����������
  nwk_event_loop,               // �������������
  Hal_ProcessEvent,             // Ӳ���������������
#if defined( MT_TASK )
  MT_ProcessEvent,              // ��ز�����������
#endif
  APS_event_loop,               // Ӧ��֧���Ӳ���������
#if defined ( ZIGBEE_FRAGMENTATION )
  APSF_ProcessEvent,            // Ӧ��֧���Ӳ���Ϣ�ָ���������
#endif
  ZDApp_event_loop,             // ZigBee�豸Ӧ����������
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  ZDNwkMgr_event_loop,          // ����������������
#endif
  zcl_event_loop,               // ZCL��������
#if defined ( ZCL_KEY_ESTABLISH )
  zclKeyEstablish_event_loop,   // ZCL��Կ������������
#endif
  simplemeter_event_loop        // �û�Ӧ�ò��������������û��Լ���д
};


/*
   ��������
*/
const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );

/*
   �����¼�
*/
uint16 *tasksEvents;
/********************************************************************/



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
#if defined ( ZIGBEE_FRAGMENTATION )
  APSF_Init( taskID++ );
#endif
  ZDApp_Init( taskID++ );
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  ZDNwkMgr_Init( taskID++ );
#endif
  zcl_Init( taskID++ );
#if defined ( ZCL_KEY_ESTABLISH )
  zclGeneral_KeyEstablish_Init( taskID++ );
#endif
  simplemeter_Init( taskID );
}
