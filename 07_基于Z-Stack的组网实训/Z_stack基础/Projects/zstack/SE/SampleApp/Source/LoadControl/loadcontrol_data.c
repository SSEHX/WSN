/*******************************************************************************
 * �ļ����ƣ�loadcontrol_data.c
 * ��    �ܣ����ؿ����������ļ���
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "OSAL.h"
#include "ZDConfig.h"

#include "se.h"
#include "loadcontrol.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"
/********************************************************************/


/* �������� */
/********************************************************************/
#define LOADCONTROL_DEVICE_VERSION      0
#define LOADCONTROL_FLAGS               0

#define LOADCONTROL_HWVERSION           1
#define LOADCONTROL_ZCLVERSION          1

/*
  ������
 */
const uint8 loadControlZCLVersion = LOADCONTROL_ZCLVERSION;
const uint8 loadControlHWVersion = LOADCONTROL_HWVERSION;
const uint8 loadControlManufacturerName[] = { 16, 'S','I','K','A','I',' ','E','L','E','C','T','R','O','N','I','C' };
const uint8 loadControlModelId[] = { 16, 'S','K','0','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 loadControlDateCode[] = { 16, '2','0','1','0','0','3','0','8',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 loadControlPowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 loadControlLocationDescription[] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 loadControlPhysicalEnvironment = PHY_UNSPECIFIED_ENV;
uint8 loadControlDeviceEnabled = DEVICE_ENABLED;

/*
  ʶ�������
 */
uint16 loadControlIdentifyTime = 0;
uint32 loadControlTime = 0;
uint8 loadControlTimeStatus = 0x01;

/*
  ��Ӧ���󼰸��ؿ��� 
*/
uint8 loadControlUtilityDefinedGroup = 0x00;
uint8 loadControlStartRandomizeMinutes = 0x00;
uint8 loadControlStopRandomizeMinutes = 0x00;
uint8 loadControlSignature[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

/*
  ��Կ����
 */
uint16 loadControlKeyEstablishmentSuite = CERTIFICATE_BASED_KEY_ESTABLISHMENT;

/*
  �ض��� - �û���ID
 */
CONST zclAttrRec_t loadControlAttrs[LOADCONTROL_MAX_ATTRIBUTES] =
{
  /* ͨ�û��������� */
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // ��ID - �ڽ���ʱ������(���磺zcl.h)
    {  // ���Լ�¼
      ATTRID_BASIC_ZCL_VERSION,           // ����ID - �ڴؿ�ͷ�ļ���(���磺zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // �������� - �� zcl.h��
      ACCESS_CONTROL_READ,                // �洢���Ʊ��� - �� zcl.h��
      (void *)&loadControlZCLVersion      // ָ�����Ա���
    }
  },
  {                                       // ��ͬ
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_HW_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&loadControlHWVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)loadControlManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)loadControlModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)loadControlDateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {   // ���Լ�¼
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&loadControlPowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)loadControlLocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&loadControlPhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // ���Լ�¼
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&loadControlDeviceEnabled
    }
  },

  /* ʶ������� */
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // ���Լ�¼
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&loadControlIdentifyTime
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
      (void *)&loadControlTime
    }
  },
  // �� SE Ӧ����, ����ʱ�ӽ���ʹ�á���˱�Ǵ洢���ƽ��ܹ�����
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { // ���Լ�¼
      ATTRID_TIME_STATUS,
      ZCL_DATATYPE_BITMAP8,
      ACCESS_CONTROL_READ,
      (void *)&loadControlTimeStatus
    }
  },


  /* SE ���� */
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    {  // ���Լ�¼
      ATTRID_SE_UTILITY_DEFINED_GROUP,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&loadControlUtilityDefinedGroup
    }
  },
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    {  // ���Լ�¼
      ATTRID_SE_START_RANDOMIZE_MINUTES,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&loadControlStartRandomizeMinutes
    }
  },
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    {  // ���Լ�¼
      ATTRID_SE_STOP_RANDOMIZE_MINUTES,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&loadControlStopRandomizeMinutes
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT,
    {  // ���Լ�¼
      ATTRID_KEY_ESTABLISH_SUITE,
      ZCL_DATATYPE_BITMAP16,
      ACCESS_CONTROL_READ,
      (void *)&loadControlKeyEstablishmentSuite
    }
  },
};


/*
  ��ѡ���
 */
zclOptionRec_t loadControlOptions[LOADCONTROL_MAX_OPTIONS] =
{
  /* ͨ�ô�ѡ�� */
  {
    ZCL_CLUSTER_ID_GEN_TIME,                 // ��ID - �ڻ������ж���(���磺zcl.h)
    ( AF_EN_SECURITY /*| AF_ACK_REQUEST*/ ), // ѡ�� - �� AFͷ�ļ��ж���(���磺AF.h)
  },

  /* ������Դ��ѡ�� */
  {
    ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
    ( AF_EN_SECURITY ),
  },
};


/* 
  ���ؿ�����������������
 */
#define LOADCONTROL_MAX_INCLUSTERS       3
const cId_t loadControlInClusterList[LOADCONTROL_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_SE_LOAD_CONTROL
};

/* 
  ���ؿ�����������������
 */
#define LOADCONTROL_MAX_OUTCLUSTERS       3
const cId_t loadControlOutClusterList[LOADCONTROL_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_SE_LOAD_CONTROL
};

SimpleDescriptionFormat_t loadControlSimpleDesc =
{
  LOADCONTROL_ENDPOINT,                   //  uint8 Endpoint;
  ZCL_SE_PROFILE_ID,                      //  uint16 AppProfId[2];
  ZCL_SE_DEVICEID_LOAD_CTRL_EXTENSION,    //  uint16 AppDeviceId[2];
  LOADCONTROL_DEVICE_VERSION,             //  int   AppDevVer:4;
  LOADCONTROL_FLAGS,                      //  int   AppFlags:4;
  LOADCONTROL_MAX_INCLUSTERS,             //  uint8  AppNumInClusters;
  (cId_t *)loadControlInClusterList,      //  cId_t *pAppInClusterList;
  LOADCONTROL_MAX_OUTCLUSTERS,            //  uint8  AppNumInClusters;
  (cId_t *)loadControlOutClusterList      //  cId_t *pAppInClusterList;
};

