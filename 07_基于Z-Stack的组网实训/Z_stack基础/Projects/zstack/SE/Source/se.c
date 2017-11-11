/*******************************************************************************
 * �ļ����ƣ�se.c
 * ��    �ܣ�SE�ĳ�ʼ�������ļ���
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"
#include "se.h"
/********************************************************************/



/* ���غ��� */
/*********************************************************************
 * �������ƣ�zclSE_Init
 * ��    �ܣ�SE�ĳ�ʼ��������
 * ��ڲ�����*simpleDesc ʹ��SE Profileע������������ú���ͬʱҲע��
 *           Profile�Ĵ�ת����
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
void zclSE_Init( SimpleDescriptionFormat_t *simpleDesc )
{
  endPointDesc_t *epDesc;

  /* ע��Ӧ�ó���˵��������������ڴ治�ᱻ�ͷ� */
  epDesc = osal_mem_alloc( sizeof ( endPointDesc_t ) );
  if ( epDesc )
  {
    /* ���˵������� */
    epDesc->endPoint = simpleDesc->EndPoint;
    epDesc->task_id = &zcl_TaskID;   // ������Ϣ���ȷ���ZCL
    epDesc->simpleDesc = simpleDesc;
    epDesc->latencyReq = noLatencyReqs;

    /* ʹ��AFע��˵������� */
    afRegister( epDesc );
  }
}
