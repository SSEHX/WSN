/*******************************************************************************
 * �ļ����ƣ�esp.h
 * ��    �ܣ�SE������Դʵ��esp.c�ļ���ͷ�ļ���
 ******************************************************************************/

#ifndef ESP_H
#define ESP_H

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
  ESP�˵�ֵ
 */
#define ESP_ENDPOINT                 0x09

/* 
  ESP���������
 */
#define ESP_MAX_ATTRIBUTES           53

/* 
  ESP���ѡ����
 */
#define ESP_MAX_OPTIONS              8

/* 
  ESP����ʱ������
 */
#define ESP_UPDATE_TIME_PERIOD       1000     // ��λ��

/* 
  HVAC�豸��
 */
#define HVAC_DEVICE_CLASS            0x000001 // HVAC ѹ������¯ - λ0��λ

/* 
  ���������豸��
 */
#define ONOFF_LOAD_DEVICE_CLASS      0x000080 // ��סլ���ظ��� - λ7��λ

/* 
  Ӧ�ó����¼�
 */
#define ESP_IDENTIFY_TIMEOUT_EVT     0x0001
#define ESP_UPDATE_TIME_EVT          0x0002
#define SIMPLE_DESC_QUERY_EVT_1      0x0004
#define SIMPLE_DESC_QUERY_EVT_2      0x0008
/********************************************************************/

/* 
  ����
 */
/********************************************************************/
extern SimpleDescriptionFormat_t espSimpleDesc;
extern CONST zclAttrRec_t espAttrs[];
extern zclOptionRec_t espOptions[];
extern uint8 espDeviceEnabled;
extern uint16 espTransitionTime;
extern uint16 espIdentifyTime;
extern uint32 espTime;
extern uint8 espSignature[];
/********************************************************************/



/*********************************************************************
 * �������ƣ�esp_Init
 * ��    �ܣ�ESP�ĳ�ʼ��������
 * ��ڲ�����task_id  ��OSAL���������ID����ID������������Ϣ���趨��ʱ
 *           ����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
extern void esp_Init( uint8 task_id );


/*********************************************************************
 * �������ƣ�esp_event_loop
 * ��    �ܣ�ESP�������¼���������
 * ��ڲ�����task_id  ��OSAL���������ID��
 *           events   ׼��������¼����ñ�����һ��λͼ���ɰ�������¼���
 * ���ڲ�������
 * �� �� ֵ����δ������¼���
 ********************************************************************/
extern uint16 esp_event_loop( uint8 task_id, uint16 events );


#ifdef __cplusplus
}
#endif

#endif /* esp_H */
