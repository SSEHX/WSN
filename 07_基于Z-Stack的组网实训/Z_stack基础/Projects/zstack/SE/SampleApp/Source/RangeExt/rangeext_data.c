/*******************************************************************************
 * �ļ����ƣ�rangeext_data.c
 * ��    �ܣ������������ļ���
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZDConfig.h"
#include "se.h"
#include "rangeext.h"
#include "zcl_general.h"
#include "zcl_key_establish.h"
/********************************************************************/


/* �������� */
/********************************************************************/
#define RANGEEXT_DEVICE_VERSION      0
#define RANGEEXT_FLAGS               0

#define RANGEEXT_HWVERSION           1
#define RANGEEXT_ZCLVERSION          1

/*
  ������
 */
const uint8 rangeExtZCLVersion = RANGEEXT_ZCLVERSION;
const uint8 rangeExtHWVersion = RANGEEXT_HWVERSION;
const uint8 rangeExtManufacturerName[] = { 16, 'S','I','K','A','I',' ','E','L','E','C','T','R','O','N','I','C' };
const uint8 rangeExtModelId[] = { 16, 'S','K','0','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 rangeExtDateCode[] = { 16, '2','0','1','0','0','3','0','8',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 rangeExtPowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 rangeExtLocationDescription[] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 rangeExtPhysicalEnvironment = PHY_UNSPECIFIED_ENV;
uint8 rangeExtDeviceEnabled = DEVICE_ENABLED;

/*
  ʶ�������
 */
uint16 rangeExtIdentifyTime = 0;
uint32 rangeExtTime = 0;
uint8 rangeExtTimeStatus = 0x01;

/*
  ��Կ����
 */
uint8 zclRangeExt_KeyEstablishmentSuite = CERTIFICATE_BASED_KEY_ESTABLISHMENT;

/*
  �ض��� - �û���ID
 */
CONST zclAttrRec_t rangeExtAttrs[RANGEEXT_MAX_ATTRIBUTES] =
{
  /* ͨ�û��������� */
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // ��ID - �ڽ���ʱ������(���磺zcl.h)
    {  // ���Լ�¼
      ATTRID_BASIC_ZCL_VERSION,           // ����ID - �ڴؿ�ͷ�ļ���(���磺zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // �������� - �� zcl.h��
      ACCESS_CONTROL_READ,                // �洢���Ʊ��� - �� zcl.h��
      (void *)&rangeExtZCLVersion         // ָ�����Ա���ָ��
    }
  },
  {                                       // ��ͬ
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_HW_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&rangeExtHWVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)rangeExtManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)rangeExtModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)rangeExtDateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {   // ���Լ�¼
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&rangeExtPowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)rangeExtLocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&rangeExtPhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&rangeExtDeviceEnabled
    }
  },

  /* ʶ������� */
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // ���Լ�¼
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&rangeExtIdentifyTime
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
      (void *)&rangeExtTime
    }
  },
  // �� SE Ӧ����, ����ʱ�ӽ���ʹ�á���˱�Ǵ洢���ƽ��ܹ�����
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { // ���Լ�¼
      ATTRID_TIME_STATUS,
      ZCL_DATATYPE_BITMAP8,
      ACCESS_CONTROL_READ,
      (void *)&rangeExtTimeStatus
    }
  },

  // SE����
  {
    ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT,
    {  // ���Լ�¼
      ATTRID_KEY_ESTABLISH_SUITE,
      ZCL_DATATYPE_BITMAP16,
      ACCESS_CONTROL_READ,
      (void *)&zclRangeExt_KeyEstablishmentSuite
    }
  },
};

/*
  ��ѡ���
 */
zclOptionRec_t rangeExtOptions[RANGEEXT_MAX_OPTIONS] =
{
  /* ͨ�ô�ѡ�� */
  {
    ZCL_CLUSTER_ID_GEN_TIME,                 // ��ID - �ڻ������ж���(���磺zcl.h)
    ( AF_EN_SECURITY /*| AF_ACK_REQUEST*/ ), // ѡ�� - �� AFͷ�ļ��ж���(���磺AF.h)
  },
};

/* 
  ������������������
 */
#define RANGEEXT_MAX_INCLUSTERS       2
const cId_t rangeExtInClusterList[RANGEEXT_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY
};

/* 
  ������������������
 */
#define RANGEEXT_MAX_OUTCLUSTERS      2
const cId_t rangeExtOutClusterList[RANGEEXT_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY
};

SimpleDescriptionFormat_t rangeExtSimpleDesc =
{
  RANGEEXT_ENDPOINT,                    //  uint8 Endpoint;
  ZCL_SE_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_SE_DEVICEID_RANGE_EXTENDER,       //  uint16 AppDeviceId[2];
  RANGEEXT_DEVICE_VERSION,              //  int   AppDevVer:4;
  RANGEEXT_FLAGS,                       //  int   AppFlags:4;
  RANGEEXT_MAX_INCLUSTERS,              //  uint8  AppNumInClusters;
  (cId_t *)rangeExtInClusterList,       //  cId_t *pAppInClusterList;
  RANGEEXT_MAX_OUTCLUSTERS,             //  uint8  AppNumInClusters;
  (cId_t *)rangeExtOutClusterList       //  cId_t *pAppInClusterList;
};


