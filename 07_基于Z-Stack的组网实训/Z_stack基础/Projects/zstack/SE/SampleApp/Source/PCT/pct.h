/*******************************************************************************
 * �ļ����ƣ�pct.h
 * ��    �ܣ�SE������Դʵ��pct.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef PCT_H
#define PCT_H

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
  �ɱ��ͨ���¿����˵�ֵ
 */
#define PCT_ENDPOINT                 0x09

/* 
  �ɱ��ͨ���¿������������
 */
#define PCT_MAX_ATTRIBUTES           16

/* 
  �ɱ��ͨ���¿������ѡ����
 */
#define PCT_MAX_OPTIONS              3

/* 
  ����ʱ������
 */
#define PCT_UPDATE_TIME_PERIOD       1000       // ����ʱ���¼�����Ϊ��λ
#define PCT_LOAD_CTRL_PERIOD         60000      // �ɱ��ͨ���¿����¼�����1���ӣ��Ժ���Ϊ��λ
#define LOADCONTROL_EVENT_ID         0x12345678 // ���ؿ������¼�ID
#define START_TIME_NOW               0x00000000 // �����¼� - ��������ִ��
#define HVAC_DEVICE_CLASS            0x000001   // HVAC  ѹ������¯ - λ0��λ
#define ONOFF_LOAD_DEVICE_CLASS      0x000080   // ��סլ���ظ��� - λ7��λ
#define SE_DEVICE_POLL_RATE          8000       // SE�ն��豸��ѯ����

/* 
  Ӧ�ó����¼�
 */
#define PCT_IDENTIFY_TIMEOUT_EVT           0x0001
#define PCT_UPDATE_TIME_EVT                0x0002
#define PCT_KEY_ESTABLISHMENT_REQUEST_EVT  0x0004
#define PCT_LOAD_CTRL_EVT                  0x0008

/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t pctSimpleDesc;
extern CONST zclAttrRec_t pctAttrs[];
extern zclOptionRec_t pctOptions[];
extern uint8 pctDeviceEnabled;
extern uint16 pctTransitionTime;
extern uint16 pctIdentifyTime;
extern uint32 pctTime;
extern uint8 pctSignature[];
/********************************************************************/



/*********************************************************************
 * �������ƣ�pct_Init
 * ��    �ܣ�PCT�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void pct_Init( uint8 task_id );


/*********************************************************************
 * �������ƣ�pct_event_loop
 * ��    �ܣ�PCT�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 pct_event_loop( uint8 task_id, uint16 events );


#ifdef __cplusplus
}
#endif

#endif /* PCT_H */
