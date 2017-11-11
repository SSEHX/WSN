/*******************************************************************************
 * �ļ����ƣ�loadcontrol.h
 * ��    �ܣ�SE������Դʵ��loadcontrol.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef LOADCONTROL_H
#define LOADCONTROL_H

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
  ���ؿ������˵�ֵ
 */
#define LOADCONTROL_ENDPOINT                 0x09

/* 
  ���ؿ��������������
 */
#define LOADCONTROL_MAX_ATTRIBUTES           16

/* 
  ���ؿ��������ѡ����
 */
#define LOADCONTROL_MAX_OPTIONS              2

/* 
  ����ʱ������
 */
#define LOADCONTROL_UPDATE_TIME_PERIOD       1000       // ����ʱ���¼�����Ϊ��λ
#define LOADCONTROL_LOAD_CTRL_PERIOD         60000      // ���ؿ������¼�����1���ӣ��Ժ���Ϊ��λ
#define LOADCONTROL_EVENT_ID                 0x12345678 // ���ؿ������¼�ID
#define START_TIME_NOW                       0x00000000 // �����¼� - ��������ִ��
#define HVAC_DEVICE_CLASS                    0x000001   // HVAC  ѹ������¯ - λ0��λ
#define ONOFF_LOAD_DEVICE_CLASS              0x000080   // ��סլ���ظ��� - λ7��λ

/* 
  Ӧ�ó����¼�
 */
#define LOADCONTROL_IDENTIFY_TIMEOUT_EVT           0x0001
#define LOADCONTROL_UPDATE_TIME_EVT                0x0002
#define LOADCONTROL_KEY_ESTABLISHMENT_REQUEST_EVT  0x0004
#define LOADCONTROL_LOAD_CTRL_EVT                  0x0008

/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t loadControlSimpleDesc;
extern CONST zclAttrRec_t loadControlAttrs[];
extern zclOptionRec_t loadControlOptions[];
extern uint8 loadControlDeviceEnabled;
extern uint16 loadControlTransitionTime;
extern uint16 loadControlIdentifyTime;
extern uint32 loadControlTime;
extern uint8 loadControlSignature[];
/********************************************************************/



/*********************************************************************
 * �������ƣ�loadcontrol_Init
 * ��    �ܣ�LCD�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void loadcontrol_Init( uint8 task_id );


/*********************************************************************
 * �������ƣ�loadcontrol_event_loop
 * ��    �ܣ�LCD�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 loadcontrol_event_loop( uint8 task_id, uint16 events );



#ifdef __cplusplus
}
#endif

#endif /* LOADCONTROL_H */
