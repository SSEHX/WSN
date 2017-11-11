/*******************************************************************************
 * �ļ����ƣ�simplemeter.h
 * ��    �ܣ�SE������Դʵ��simplemeter.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef SIMPLEMETER_H
#define SIMPLEMETER_H

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
  �򵥼������˵�ֵ
 */
#define SIMPLEMETER_ENDPOINT                       0x09

/* 
  �򵥼��������������
 */
#define SIMPLEMETER_MAX_ATTRIBUTES                 50

/* 
  �򵥼��������ѡ����
 */
#define SIMPLEMETER_MAX_OPTIONS                    2

/* 
  ����ʱ������
 */
#define SIMPLEMETER_UPDATE_TIME_PERIOD             1000   // ����ʱ������Ϊ��λ 
#define SIMPLEMETER_REPORT_PERIOD                  5000   // ����������
#define SE_DEVICE_POLL_RATE                        8000   // SE�ն��豸��ѯ���� 

/* 
  Ӧ�ó����¼�
 */
#define SIMPLEMETER_IDENTIFY_TIMEOUT_EVT           0x0001
#define SIMPLEMETER_UPDATE_TIME_EVT                0x0002
#define SIMPLEMETER_KEY_ESTABLISHMENT_REQUEST_EVT  0x0004
#define SIMPLEMETER_REPORT_ATTRIBUTE_EVT           0x0008


/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t simpleMeterSimpleDesc;
extern CONST zclAttrRec_t simpleMeterAttrs[];
extern zclOptionRec_t simpleMeterOptions[];
extern uint8 simpleMeterDeviceEnabled;
extern uint16 simpleMeterTransitionTime;
extern uint16 simpleMeterIdentifyTime;
extern uint32 simpleMeterTime;
/********************************************************************/



/*********************************************************************
 * �������ƣ�simplemeter_Init
 * ��    �ܣ��򵥼������ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void simplemeter_Init( uint8 task_id );


/*********************************************************************
 * �������ƣ�simplemeter_event_loop
 * ��    �ܣ��򵥼������������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 simplemeter_event_loop( uint8 task_id, uint16 events );


#ifdef __cplusplus
}
#endif

#endif /* SIMPLEMETER_H */
