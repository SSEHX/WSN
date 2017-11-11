/*******************************************************************************
 * �ļ����ƣ�rangeext.h
 * ��    �ܣ�SE������Դʵ��rangeext.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef RANGEEXT_H
#define RANGEEXT_H

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
  �������˵�ֵ
 */
#define RANGEEXT_ENDPOINT                       0x09

/* 
  ���������������
 */
#define RANGEEXT_MAX_ATTRIBUTES                 13

/* 
  ���������ѡ����
 */
#define RANGEEXT_MAX_OPTIONS                    1

/* 
  ����ʱ������
 */
#define RANGEEXT_UPDATE_TIME_PERIOD             1000  // ����ʱ���¼�����Ϊ��λ

/* 
  Ӧ�ó����¼�
 */
#define RANGEEXT_IDENTIFY_TIMEOUT_EVT           0x0001
#define RANGEEXT_KEY_ESTABLISHMENT_REQUEST_EVT  0x0002

/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t rangeExtSimpleDesc;
extern CONST zclAttrRec_t rangeExtAttrs[];
extern zclOptionRec_t rangeExtOptions[];
extern uint8 rangeExtDeviceEnabled;
extern uint16 rangeExtTransitionTime;
extern uint16 rangeExtIdentifyTime;
extern uint32 rangeExtTime;
/********************************************************************/



/*********************************************************************
 * �������ƣ�rangeext_Init
 * ��    �ܣ��������ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void rangeext_Init( uint8 task_id );

/*********************************************************************
 * �������ƣ�rangeext_event_loop
 * ��    �ܣ��������������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 rangeext_event_loop( uint8 task_id, uint16 events );


#ifdef __cplusplus
}
#endif

#endif /* RANGEEXT_H */
