/*******************************************************************************
 * �ļ����ƣ�pct_data.c
 * ��    �ܣ�PCT�ɱ��ͨ���¿��������ļ���
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "ZDConfig.h"

#include "se.h"
#include "pct.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"
/********************************************************************/


/* �������� */
/********************************************************************/
#define PCT_DEVICE_VERSION      0
#define PCT_FLAGS               0

#define PCT_HWVERSION           1
#define PCT_ZCLVERSION          1

/*
  ������
 */
const uint8 pctZCLVersion = PCT_ZCLVERSION;
const uint8 pctHWVersion = PCT_HWVERSION;
const uint8 pctManufacturerName[] = { 16, 'S','I','K','A','I',' ','E','L','E','C','T','R','O','N','I','C' };
const uint8 pctModelId[] = { 16, 'S','K','0','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 pctDateCode[] = { 16, '2','0','1','0','0','3','0','8',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 pctPowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 pctLocationDescription[] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 pctPhysicalEnvironment = PHY_UNSPECIFIED_ENV;
uint8 pctDeviceEnabled = DEVICE_ENABLED;

/*
  ʶ�������
 */
uint16 pctIdentifyTime = 0;
uint32 pctTime = 0;
uint8 pctTimeStatus = 0x01;

/*
  ��Ӧ���󼰸��ؿ���
 */
uint8 pctUtilityDefinedGroup = 0x00;
uint8 pctStartRandomizeMinutes = 0x00;
uint8 pctStopRandomizeMinutes = 0x00;
uint8 pctSignature[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

/*
  ��Կ����
 */
uint16 pctKeyEstablishmentSuite = CERTIFICATE_BASED_KEY_ESTABLISHMENT;

/*
  �ض��� - �û���ID
 */
CONST zclAttrRec_t pctAttrs[PCT_MAX_ATTRIBUTES] =
{
  /* ͨ�û��������� */
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // ��ID - �ڽ���ʱ������(���磺zcl.h)
    {  // ���Լ�¼
      ATTRID_BASIC_ZCL_VERSION,           // ����ID - �ڴؿ�ͷ�ļ���(���磺zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // �������� - �� zcl.h��
      ACCESS_CONTROL_READ,                // �洢���Ʊ��� - �� zcl.h��
      (void *)&pctZCLVersion              // ָ�����Ա���ָ��
    }
  },
  {                                       // ��ͬ
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_HW_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&pctHWVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)pctManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)pctModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)pctDateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {   // ���Լ�¼
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&pctPowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)pctLocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&pctPhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&pctDeviceEnabled
    }
  },

  /* ʶ������� */
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // ���Լ�¼
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&pctIdentifyTime
    }
  },

  /* ʱ������� */
  // �� SE Ӧ����, ����ʱ�ӽ���ʹ�á���˱�Ǵ洢���ƽ��ܹ�����
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { // ���Լ�¼
      ATTRID_TIME_TIME,
      ZCL_DATATYPE_UTC,
      ACCESS_CONTROL_READ,
      (void *)&pctTime
    }
  },
  // �� SE Ӧ����, ����ʱ�ӽ���ʹ�á���˱�Ǵ洢���ƽ��ܹ�����
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { // ���Լ�¼
      ATTRID_TIME_STATUS,
      ZCL_DATATYPE_BITMAP8,
      ACCESS_CONTROL_READ,
      (void *)&pctTimeStatus
    }
  },


  // SE����
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    { // ���Լ�¼
      ATTRID_SE_UTILITY_DEFINED_GROUP,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&pctUtilityDefinedGroup
    }
  },
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    { // ���Լ�¼
      ATTRID_SE_START_RANDOMIZE_MINUTES,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&pctStartRandomizeMinutes
    }
  },
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    {  // ���Լ�¼
      ATTRID_SE_STOP_RANDOMIZE_MINUTES,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&pctStopRandomizeMinutes
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT,
    {  // ���Լ�¼
      ATTRID_KEY_ESTABLISH_SUITE,
      ZCL_DATATYPE_BITMAP16,
      ACCESS_CONTROL_READ,
      (void *)&pctKeyEstablishmentSuite
    }
  },
};


/*
  ��ѡ���
 */
zclOptionRec_t pctOptions[PCT_MAX_OPTIONS] =
{
  /* ͨ�ô�ѡ�� */
  {
    ZCL_CLUSTER_ID_GEN_TIME,                 // ��ID - �ڻ������ж���(���磺zcl.h)
    ( AF_EN_SECURITY /*| AF_ACK_REQUEST*/ ), // ѡ�� - �� AFͷ�ļ��ж���(���磺AF.h)
  },

  /* ������Դ��ѡ�� */
  // *** Smart Energy Cluster Options ***
  {
    ZCL_CLUSTER_ID_SE_PRICING,
    ( AF_EN_SECURITY ),
  },
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    ( AF_EN_SECURITY ),
  },
};


/* 
  PCT�ɱ���¿���������������
 */
#define PCT_MAX_INCLUSTERS       3
const cId_t pctInClusterList[PCT_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_SE_LOAD_CONTROL
};

/* 
  PCT�ɱ���¿���������������
 */
#define PCT_MAX_OUTCLUSTERS       3
const cId_t pctOutClusterList[PCT_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_SE_LOAD_CONTROL
};

SimpleDescriptionFormat_t pctSimpleDesc =
{
  PCT_ENDPOINT,                   //  uint8 Endpoint;
  ZCL_SE_PROFILE_ID,              //  uint16 AppProfId[2];
  ZCL_SE_DEVICEID_PCT,            //  uint16 AppDeviceId[2];
  PCT_DEVICE_VERSION,             //  int   AppDevVer:4;
  PCT_FLAGS,                      //  int   AppFlags:4;
  PCT_MAX_INCLUSTERS,             //  uint8  AppNumInClusters;
  (cId_t *)pctInClusterList,      //  cId_t *pAppInClusterList;
  PCT_MAX_OUTCLUSTERS,            //  uint8  AppNumInClusters;
  (cId_t *)pctOutClusterList      //  cId_t *pAppInClusterList;
};

