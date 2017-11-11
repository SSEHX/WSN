/*******************************************************************************
 * �ļ����ƣ�sapi.h
 * ��    �ܣ�SAPI������ͷ�ļ�   
 ******************************************************************************/

#ifndef SAPI_H
#define SAPI_H


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "af.h"
/********************************************************************/


/* �궨�� */
/********************************************************************/
/*
  ����󶨶�ʱ�������¼�
 */
#define ZB_ALLOW_BIND_TIMER               0x4000  

/*
  �󶨶�ʱ�������¼�
 */
#define ZB_BIND_TIMER                     0x2000

/*
  ��Ӧ�ó�����������¼�
 */
#define ZB_ENTRY_EVENT                    0x1000

/*
  �û������¼�
 */
#define ZB_USER_EVENTS                    0x00FF


/*
  �����豸����
 */
#define ZB_IEEE_SEARCH                    1

/*
  �豸��Ϣ����
 */
#define ZB_INFO_DEV_STATE                 0
#define ZB_INFO_IEEE_ADDR                 1
#define ZB_INFO_SHORT_ADDR                2
#define ZB_INFO_PARENT_SHORT_ADDR         3
#define ZB_INFO_PARENT_IEEE_ADDR          4
#define ZB_INFO_CHANNEL                   5
#define ZB_INFO_PAN_ID                    6
#define ZB_INFO_EXT_PAN_ID                7

/*
  SAPI������������Ŀ�ĵ�ַ
 */
#define ZB_BINDING_ADDR                   INVALID_NODE_ADDR
#define ZB_BROADCAST_ADDR                 0xffff

/*
  SAPI״ֵ̬
 */
#define ZB_SUCCESS                        ZSuccess
#define ZB_FAILURE                        ZFailure
#define ZB_INVALID_PARAMETER              ZInvalidParameter
#define ZB_ALREADY_IN_PROGRESS            ZSapiInProgress
#define ZB_TIMEOUT                        ZSapiTimeout
#define ZB_INIT                           ZSapiInit
#define ZB_AF_FAILURE                     afStatus_FAILED
#define ZB_AF_MEM_FAIL                    afStatus_MEM_FAIL
#define ZB_AF_INVALID_PARAMETER           afStatus_INVALID_PARAMETER

/*
  SAPI ɨ������ֵ
 */
#define ZB_SCAN_DURATION_0                0   //  19.2 ms
#define ZB_SCAN_DURATION_1                1   //  38.4 ms
#define ZB_SCAN_DURATION_2                2   //  76.8 ms
#define ZB_SCAN_DURATION_3                3   //  153.6 ms
#define ZB_SCAN_DURATION_4                4   //  307.2 ms
#define ZB_SCAN_DURATION_5                5   //  614.4 ms
#define ZB_SCAN_DURATION_6                6   //  1.23 sec
#define ZB_SCAN_DURATION_7                7   //  2.46 sec
#define ZB_SCAN_DURATION_8                8   //  4.92 sec
#define ZB_SCAN_DURATION_9                9   //  9.83 sec
#define ZB_SCAN_DURATION_10               10  //  19.66 sec
#define ZB_SCAN_DURATION_11               11  //  39.32 sec
#define ZB_SCAN_DURATION_12               12  //  78.64 sec
#define ZB_SCAN_DURATION_13               13  //  157.28 sec
#define ZB_SCAN_DURATION_14               14  //  314.57 sec

/*
  �豸���Ͷ���
 */
#define ZG_DEVICETYPE_COORDINATOR      0x00
#define ZG_DEVICETYPE_ROUTER           0x01
#define ZG_DEVICETYPE_ENDDEVICE        0x02

/*
  �����б���
 */
typedef struct
{
  uint16 panID;
  uint8 channel;
} zb_NetworkList_t;

/*
  �ص��¼�����
 */
typedef struct
{
  osal_event_hdr_t hdr;
  uint16 data;
} sapi_CbackEvent_t;


/* �ⲿ���� */
/********************************************************************/
extern uint8 sapi_TaskID;
extern endPointDesc_t sapi_epDesc;

#ifdef __cplusplus
extern "C"
{
#endif

extern const SimpleDescriptionFormat_t zb_SimpleDesc;
/********************************************************************/



/*********************************************************************
 * �������ƣ�zb_SystemReset
 * ��    �ܣ�ϵͳ��λ
 * ��ڲ�������
 * ���ڲ������� 
 * �� �� ֵ����
 ********************************************************************/
extern void zb_SystemReset ( void );


/*********************************************************************
 * �������ƣ�zb_StartRequest
 * ��    �ܣ���ʼZigBeeЭ��ջ����Э��ջ���������ȴ�NV�ж�ȡ���ò�����
 *           Ȼ���豸��������硣
 * ��ڲ�������
 * ���ڲ������� 
 * �� �� ֵ����
 ********************************************************************/
extern void zb_StartRequest ( void );


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
extern uint8 zb_PermitJoiningRequest ( uint16 destination, uint8 timeout );


/*********************************************************************
 * �������ƣ�zb_BindDevice
 * ��    �ܣ�����2���豸�佨�����߽����
 * ��ڲ�����create       TRUE��ʾ�����󶨣�FLASH ��ʾ�����
 *           commandId    ����ID���󶨱�ʶ��������ĳ����İ�
 *           pDestination �豸64λIEEE��ַ
 * ���ڲ������󶨲�����״̬   
 * �� �� ֵ����
 ********************************************************************/
extern void zb_BindDevice ( uint8 create, uint16 commandId, uint8 *pDestination );


/*********************************************************************
 * �������ƣ�zb_AllowBind
 * ��    �ܣ�ʹ�豸�ڹ涨��ʱ���ڴ��������ģʽ��
 * ��ڲ�����timeout    ����󶨵Ĺ涨ʱ�䣨��λΪ�룩
 * ���ڲ�����ZB_SUCCESS �豸���������ģʽ�����򱨴�
 * �� �� ֵ����
 ********************************************************************/
extern void zb_AllowBind (  uint8 timeout );


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
extern void zb_SendDataRequest ( uint16 destination, uint16 commandId, uint8 len,
                          uint8 *pData, uint8 handle, uint8 ack, uint8 radius );


/*********************************************************************
 * �������ƣ�zb_ReadConfiguration
 * ��    �ܣ���NV��ȡ������Ϣ��
 * ��ڲ�����configId    ������Ϣ��ʶ���� 
 *           len         ������Ϣ���ݳ���
 *           pValue      �洢������Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern uint8 zb_ReadConfiguration( uint8 configId, uint8 len, void *pValue );


/*********************************************************************
 * �������ƣ�zb_WriteConfiguration
 * ��    �ܣ���NVд��������Ϣ��
 * ��ڲ�����configId    ������Ϣ��ʶ���� 
 *           len         ������Ϣ���ݳ���
 *           pValue      �洢������Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern uint8 zb_WriteConfiguration( uint8 configId, uint8 len, void *pValue );


/*********************************************************************
 * �������ƣ�zb_GetDeviceInfo
 * ��    �ܣ���ȡ�豸��Ϣ��
 * ��ڲ�����param    �豸��Ϣ��ʶ���� 
 *           pValue   �洢�豸��Ϣ������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_GetDeviceInfo ( uint8 param, void *pValue );


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
extern void zb_FindDeviceRequest( uint8 searchType, void *searchKey );


/*********************************************************************
 * �������ƣ�zb_HandleOsalEvent
 * ��    �ܣ�����ϵͳ�¼���������
 * ��ڲ�����event  ��OSAL������¼�event����λ��������˱����õ��¼���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_HandleOsalEvent( uint16 event );


/*********************************************************************
 * �������ƣ�zb_StartConfirm
 * ��    �ܣ�����ȷ�Ϻ������������ڿ���һ���豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����status  ����������״ֵ̬�����ֵΪZB_SUCCESS��ָʾ����
 *           �豸�����ɹ���ɡ�������ǣ�����ʱmyStartRetryDelay��
 *           �����ϵͳ����MY_START_EVT�¼���
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_StartConfirm( uint8 status );


/*********************************************************************
 * �������ƣ�zb_SendDataConfirm
 * ��    �ܣ���������ȷ�Ϻ������������ڷ������ݲ��������ZigBeeЭ��ջ
 *           �����á�
 * ��ڲ�����handle  ���ݴ���ʶ������
 *           status  ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_SendDataConfirm( uint8 handle, uint8 status );


/*********************************************************************
 * �������ƣ�zb_BindConfirm
 * ��    �ܣ���ȷ�Ϻ������������ڰ󶨲��������ZigBeeЭ��ջ�����á�
 * ��ڲ�����commandId  ��ȷ���������ID��
 *           status     ����״̬�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_BindConfirm( uint16 commandId, uint8 status );


/*********************************************************************
 * �������ƣ�zb_FindDeviceConfirm
 * ��    �ܣ������豸ȷ�Ϻ������������ڲ����豸���������ZigBee
 *           Э��ջ�����á�
 * ��ڲ�����searchType  �������͡� 
 *           searchKey   ����ֵ
 *           result      ���ҽ��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result );


/*********************************************************************
 * �������ƣ�zb_ReceiveDataIndication
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
extern void zb_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  );


/*********************************************************************
 * �������ƣ�zb_AllowBindConfirm
 * ��    �ܣ������ȷ�Ϻ�����ָʾ����һ���豸��ͼ�󶨵����豸��
 * ��ڲ�����source   ��Դ��ַ�� 
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_AllowBindConfirm( uint16 source );


/*********************************************************************
 * �������ƣ�zb_HandleKeys
 * ��    �ܣ������¼���������
 * ��ڲ�����shift  ��Ϊ�棬�����ð����ĵڶ����ܡ������������ĸò�����
 *           keys   �����¼������롣���ڱ�Ӧ�ã��Ϸ��İ���Ϊ��
 *                  HAL_KEY_SW_1     SK-SmartRF10GW��UP��
 *                  HAL_KEY_SW_2     SK-SmartRF10GW��RIGHT��
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void zb_HandleKeys( uint8 shift, uint8 keys );


/*********************************************************************
 * �������ƣ�SAPI_ProcessEvent
 * ��    �ܣ�SimpleApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern UINT16 SAPI_ProcessEvent( byte task_id, UINT16 events );


/*********************************************************************
 * �������ƣ�SAPI_Init
 * ��    �ܣ�SimpleApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void SAPI_Init( byte task_id );


//extern void osalAddTasks( void );

#ifdef __cplusplus
}
#endif

#endif
