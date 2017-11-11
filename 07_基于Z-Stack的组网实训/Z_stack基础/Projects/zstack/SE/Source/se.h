/*******************************************************************************
 * �ļ����ƣ�se.h
 * ��    �ܣ�SE�ĳ�ʼ������ͷ�ļ���
 ******************************************************************************/


#ifndef SE_H
#define SE_H


#ifdef __cplusplus
extern "C"
{
#endif

/*
  Zigbee SE Profileʶ���
 */
#define ZCL_SE_PROFILE_ID                                      0x0109

/*
  ͨ���豸ID
 */
#define ZCL_SE_DEVICEID_RANGE_EXTENDER                         0x0008

/*
  SE�豸ID
 */
#define ZCL_SE_DEVICEID_ESP                                    0x0500
#define ZCL_SE_DEVICEID_METER                                  0x0501
#define ZCL_SE_DEVICEID_IN_PREMISE_DISPLAY                     0x0502
#define ZCL_SE_DEVICEID_PCT                                    0x0503
#define ZCL_SE_DEVICEID_LOAD_CTRL_EXTENSION                    0x0504
#define ZCL_SE_DEVICEID_SMART_APPLIANCE                        0x0505
#define ZCL_SE_DEVICEID_PREPAY_TERMINAL                        0x0506


/*
  ZCL SE Profile ��ʼ������
 */
extern void zclSE_Init( SimpleDescriptionFormat_t *simpleDesc );


#ifdef __cplusplus
}
#endif

#endif /* ZCL_SE_H */
  

