/*******************************************************************************
 * �ļ����ƣ�SerialApp.h
 * ��    �ܣ�SerialApp.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef SERIALAPP_H
#define SERIALAPP_H

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
#define SERIALAPP_ENDPOINT           11

  
/*
   Profile ID
 */
#define SERIALAPP_PROFID             0x0F05
  

/*
   �豸ID
 */ 
#define SERIALAPP_DEVICEID           0x0001
  

/*
   �豸�汾
 */   
#define SERIALAPP_DEVICE_VERSION     0
  

/*
   ��ʶ
 */ 
#define SERIALAPP_FLAGS              0

  
/*
   �ص�����
 */ 
#define SERIALAPP_MAX_CLUSTERS       2
  

/*
   �������ݴ�
 */  
#define SERIALAPP_CLUSTERID1         1
  
  
/*
   ��Ӧ��Ϣ��
 */  
#define SERIALAPP_CLUSTERID2         2

  
/*
   ���������ش��¼� 
 */
#define SERIALAPP_MSG_RTRY_EVT       0x0001
  

/*
   ��Ӧ��Ϣ�ش��¼� 
 */
#define SERIALAPP_RSP_RTRY_EVT       0x0002
  
  
/*
   �������ݷ����¼� 
 */
#define SERIALAPP_MSG_SEND_EVT       0x0004


/*
   ����ش����� 
 */
#define SERIALAPP_MAX_RETRIES        10


/*
   �ش���ʱʱ��
   ����Ӧ�����豸ʹ���ն��豸��������ɫ����ô�����ն��豸���ص㣬
   �ش���ʱʱ��Ӧ����������Ϊ�ն��豸��ѯ����POLL_RATE����ֵ��f8wConfig.cfg
   �ļ��ж��壬Ĭ��ֵΪ1000���룩��2����
   ����Ӧ�����豸ֻʹ��·������Э���������������ɫ�����ش���ʱʱ
   ��������õø�СһЩ������100ms��
    
 */  
#define SERIALAPP_MSG_RTRY_TIMEOUT   POLL_RATE*2
#define SERIALAPP_RSP_RTRY_TIMEOUT   POLL_RATE*2


/*
   OTA��Ϣ����ʱ�� 
 */
/* �Ӵ���ɷ�������ֵʵ��û��ʹ������ */  
#define SERIALAPP_ACK_DELAY          1  
/* ������æ����Ӧ֪ͨ�Է�Ӧ����ʱ��ֵ��ô����ʱ������ط� */
#define SERIALAPP_NAK_DELAY          250  


/*
   OTA��Ϣ����״̬ 
 */
/* ���ճɹ�����ָ�����յ���OTA��Ϣ�ɹ�д�봮�ڣ� */
#define OTA_SUCCESS                  ZSuccess     
/* �ظ�����ָ���յ���OTA��Ϣ���ظ���Ϣ�� */
#define OTA_DUP_MSG                 (ZSuccess+1)  
/* ����æ����ָ�����յ���OTA��Ϣд�봮��ʧ�ܣ���Ϊ��ʱ����æ�� */
#define OTA_SER_BUSY                (ZSuccess+2)   
/********************************************************************/


  
/* ȫ�ֱ��� */
/********************************************************************/
/*
   ��Ӧ�õ�����ID��������SerialApp_Init()����������ʱ��   �ñ�������
   �������IDֵ��
*/
extern byte SerialApp_TaskID;
/********************************************************************/



/*********************************************************************
 * �������ƣ�SerialApp_Init
 * ��    �ܣ�SerialApp�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void SerialApp_Init( byte task_id );


/*********************************************************************
 * �������ƣ�SerialApp_ProcessEvent
 * ��    �ܣ�SerialApp�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern UINT16 SerialApp_ProcessEvent( byte task_id, UINT16 events );




#ifdef __cplusplus
}
#endif

#endif /* SERIALAPP_H */