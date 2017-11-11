/*******************************************************************************
 * �ļ����ƣ�ipd.h
 * ��    �ܣ�SE������Դʵ��ipd.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef IPD_H
#define IPD_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ����ͷ�ļ� */
/********************************************************************/
#include "zcl.h"
/********************************************************************/


/* �궨�� */
/********************************************************************/
/* 
  IPD�˵�ֵ
 */
#define IPD_ENDPOINT                       0x09

/* 
  IPD���������
 */
#define IPD_MAX_ATTRIBUTES                 13

/* 
  IPD���ѡ����
 */
#define IPD_MAX_OPTIONS                    3

/* 
  ����ʱ������
 */
#define IPD_UPDATE_TIME_PERIOD             1000   // ����ʱ���¼���λΪ��
#define IPD_GET_PRICING_INFO_PERIOD        5000   // ��ȡ�۸���Ϣ������
#define SE_DEVICE_POLL_RATE                8000   // SE�ն��豸��ѯ����

/* 
  Ӧ�ó����¼�
 */
#define IPD_IDENTIFY_TIMEOUT_EVT           0x0001
#define IPD_UPDATE_TIME_EVT                0x0002
#define IPD_KEY_ESTABLISHMENT_REQUEST_EVT  0x0004
#define IPD_GET_PRICING_INFO_EVT           0x0008


/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t ipdSimpleDesc;
extern CONST zclAttrRec_t ipdAttrs[];
extern zclOptionRec_t ipdOptions[];
extern uint8 ipdDeviceEnabled;
extern uint16 ipdTransitionTime;
extern uint16 ipdIdentifyTime;
extern uint32 ipdTime;
/********************************************************************/



/*********************************************************************
 * �������ƣ�esp_Init
 * ��    �ܣ�ESP�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void ipd_Init( uint8 task_id );


/*********************************************************************
 * �������ƣ�esp_event_loop
 * ��    �ܣ�ESP�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 ipd_event_loop( uint8 task_id, uint16 events );


#ifdef __cplusplus
}
#endif

#endif /* IPD_H */
