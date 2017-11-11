/*******************************************************************************
 * �ļ����ƣ�serialApp.h
 * ��    �ܣ�GenericApp.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef GENERICAPP_H
#define GENERICAPP_H

#ifdef __cplusplus
extern "C"
{
#endif


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
/********************************************************************/
  
  
/* ������Щ����������Ϊ����ʹ�����ǿɸ����豸��Ҫ���޸ġ� */
/********************************************************************/
/*
   Ӧ�ö˵�
 */ 
#define GENERICAPP_ENDPOINT           10

  
/*
   Profile ID
 */
#define GENERICAPP_PROFID             0x0F04
  

/*
   �豸ID
 */ 
#define GENERICAPP_DEVICEID           0x0001
  

/*
   �豸�汾
 */   
#define GENERICAPP_DEVICE_VERSION     0
  

/*
   ��ʶ
 */  
#define GENERICAPP_FLAGS              0

  
/*
   �ص�����
 */   
#define GENERICAPP_MAX_CLUSTERS       1
  


/*
   "Hello World"��Ϣ��
 */   
#define GENERICAPP_CLUSTERID          1

  
/*
   
 */  
#define GENERICAPP_SEND_MSG_TIMEOUT   5000     // ÿ5��һ��


/*
   Ӧ���¼���OSAL��
 */  
#define GENERICAPP_SEND_MSG_EVT       0x0001
#define GENERICAPP_READ_UART_EVT       0x0002    
/********************************************************************/
  
  
/*********************************************************************
 * �������ƣ�GenericApp_Init
 * ��    �ܣ�GenericApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void GenericApp_Init( byte task_id );


/*********************************************************************
 * �������ƣ�GenericApp_ProcessEvent
 * ��    �ܣ�GenericApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events );


#ifdef __cplusplus
}
#endif

#endif /* GENERICAPP_H */
