/*******************************************************************************
 * �ļ����ƣ�ZMain.c
 * ��    �ܣ�main������ڡ�
 ******************************************************************************/


/* ����ͷ�ļ� */
/********************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"
#include "ZMAC.h"

#ifndef NONWK
  #include "AF.h"
#endif

/* Hal */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_assert.h"
#include "hal_flash.h"
/********************************************************************/


/* ϵͳ����ǰ���VDD���������� */
#define MAX_VDD_SAMPLES  3
#define ZMAIN_VDD_LIMIT  HAL_ADC_VDD_LIMIT_4


/* �ⲿ���� */
/********************************************************************/
extern bool HalAdcCheckVdd (uint8 limit);


/* ���غ��� */
/********************************************************************/
static void zmain_dev_info( void );
static void zmain_ext_addr( void );
static void zmain_ram_init( void );
static void zmain_vdd_check( void );

#ifdef LCD_SUPPORTED
static void zmain_lcd_init( void );
#endif
/********************************************************************/



/*********************************************************************
 * �������ƣ�main 
 * ��    �ܣ���������
 * ��ڲ�������   
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
int main( void )
{
  /* �ر��ж� */
  osal_int_disable( INTS_ALL );

  /* ��ʼ��ϵͳʱ�Ӽ�LED�� */
  HAL_BOARD_INIT();

  /* ��⹩���ѹ */ 
 // zmain_vdd_check();

  /* ��ʼ����ջ */
  zmain_ram_init();

  /* ��ʼ��������ΧI/O */
  InitBoard( OB_COLD );

  /* ��ʼ��Ӳ����������� */
  HalDriverInit();

  /* ��ʼ��ϵͳNV����ʧ�Դ洢 */
  osal_nv_init( NULL );

  /* ��ʼ������NV�� */
  zgInit();

  /* ��ʼ��MAC */ 
  ZMacInit();

  /* ȷ����չ��ַ */
  zmain_ext_addr();

  /* ��ʼ��Ӧ�ÿ�� */
#ifndef NONWK
  afInit(); // AFӦ�ÿ�ܲ���ϵͳ��������˵������ĳ�ʼ������
#endif

  /* ��ʼ������ϵͳ */
  osal_init_system();

  /* �����ж� */
  osal_int_enable( INTS_ALL );

  /* ���հ弶��ʼ�� */
  InitBoard( OB_READY );

  /* ��ʾ���豸��Ϣ */
  zmain_dev_info();

  /* ���������LCD������LCD����ʾ�豸��Ϣ */
#ifdef LCD_SUPPORTED
  zmain_lcd_init();
#endif

#ifdef WDT_IN_PM1
  /* ������Ź���ʹ�ã��˴�ʹ�� */
  WatchDogEnable( WDTIMX );
#endif

  osal_start_system(); // ����ϵͳ���ȣ��޷���

  return ( 0 );
}


/*********************************************************************
 * �������ƣ�zmain_vdd_check
 * ��    �ܣ�ϵͳ��ѹ��⺯�������VDD�Ƿ�����
 * ��ڲ�������
 * ���ڲ��������VDD����������ô��˸LED��
 * �� �� ֵ����
 ********************************************************************/
#if 1
static void zmain_vdd_check( void )
{
  uint8 vdd_passed_count = 0;
  bool toggle = 0;

  /* ������������Ƿ�ﵽ�趨ֵ */
  while ( vdd_passed_count < MAX_VDD_SAMPLES )
  {
    /* ���VDD��ѹ���ͨ�� */
    if ( HalAdcCheckVdd (ZMAIN_VDD_LIMIT) )
    {
      vdd_passed_count++;    // ������ѹ����������1
      MicroWait (10000);     // �ȴ� 10��������²���
    }
    /* ���VDD��ѹ���δͨ�� */
    else
    {
      vdd_passed_count = 0;  // �������ֵ
      MicroWait (50000);     // �ȴ�50����
      MicroWait (50000);     // �����ȴ�50��������²���
    }

    /* ���VDD��ѹ���δͨ�������л�LED1 LED2����״̬ */
    if (vdd_passed_count == 0)
    {
      if ((toggle = !(toggle)))
        HAL_TOGGLE_LED1();
      else
        HAL_TOGGLE_LED2();
    }
  }

  /* Ϩ��LED1 LED2 */
  HAL_TURN_OFF_LED1();
  HAL_TURN_OFF_LED2();
}
#endif

/*********************************************************************
 * �������ƣ�zmain_ext_addr
 * ��    �ܣ�ȷ���ⲿ��ַ����Ч��չ��ַ��ִ�����ȼ��������ҽ����
 *           д�뵽ϵͳNV����ʧ�Դ洢���С�
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void zmain_ext_addr(void)
{
  uint8 nullAddr[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 writeNV = TRUE;

  /* ���ȼ��ϵͳNV���Ƿ������չ��ַ */
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL))  ||
      (SUCCESS != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress)) ||
      (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN)))
  {
    /* ��ȡ��չ��ַ */
    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, aExtendedAddress, Z_EXTADDR_LEN);

    if (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN))
    {
      if (!osal_memcmp((uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), nullAddr, Z_EXTADDR_LEN))
      {
        osal_memcpy(aExtendedAddress, (uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), Z_EXTADDR_LEN);
      }
      else  // δ�ҵ���Ч��չ��ַ
      {
        uint8 idx;
        
#if !defined ( NV_RESTORE )
        writeNV = FALSE;  // ��������ʱIEEE��ַ������д��NV
#endif

        /* ������չ��ַ����� */
        for (idx = 0; idx < (Z_EXTADDR_LEN - 2);)
        {
          uint16 randy = osal_rand();
          aExtendedAddress[idx++] = LO_UINT16(randy);
          aExtendedAddress[idx++] = HI_UINT16(randy);
        }
        
        /* ZigBee�豸���ͱ�ʶ�� */
#if defined ZDO_COORDINATOR
        aExtendedAddress[idx++] = 0x10;
#elif defined RTR_NWK
        aExtendedAddress[idx++] = 0x20;
#else
        aExtendedAddress[idx++] = 0x30;
#endif
        aExtendedAddress[idx] = 0xF8;
      }
    }

    if ( writeNV )
    {
      (void)osal_nv_write(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);
    }
  }

  /* ������������MAC��չ��ַ */
  (void)ZMacSetReq(MAC_EXTENDED_ADDRESS, aExtendedAddress);
}


/*********************************************************************
 * �������ƣ�zmain_dev_info
 * ��    �ܣ���LCD����ʾIEEE��ַ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void zmain_dev_info(void)
{
#ifdef LCD_SUPPORTED
  uint8 i;
  uint8 *xad;
  uint8 lcd_buf[Z_EXTADDR_LEN*2+1+5];
  uint8 ieeeAddr[]="IEEE:";
  
  for(i = 0; i < 5; i++)
  {
    lcd_buf[i] = ieeeAddr[i];
  }
  
  /* ��ʾ��չ��ַ */
  xad = aExtendedAddress + Z_EXTADDR_LEN - 1;

  for (i = 5; i < Z_EXTADDR_LEN*2 + 5; xad--)
  {
    uint8 ch;
    ch = (*xad >> 4) & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *xad & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
  lcd_buf[Z_EXTADDR_LEN*2+5] = '\0';

  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_3 );
#endif
}


/*********************************************************************
 * �������ƣ�zmain_ram_init
 * ��    �ܣ���ʼ��RAM��ջ��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void zmain_ram_init( void )
{
  uint8 *end;
  uint8 *ptr;

  /* ��ʼ�����ã���������ջ */
  end = (uint8*)CSTK_BEG;
  ptr = (uint8*)(*( __idata uint16*)(CSTK_PTR));
  while ( --ptr > end )
    *ptr = STACK_INIT_VALUE;

  /* ��ʼ�����أ���ַ����ջ */
  ptr = (uint8*)RSTK_END - 1;
  while ( --ptr > (uint8*)SP )
    *(__idata uint8*)ptr = STACK_INIT_VALUE;
}


#ifdef LCD_SUPPORTED
/*********************************************************************
 * �������ƣ�zmain_lcd_init
 * ��    �ܣ������������г�ʼ��LCD��
 * ��ڲ�������
 * ���ڲ�������
 * �� �� ֵ����
 ********************************************************************/
static void zmain_lcd_init ( void )
{
#ifdef SERIAL_DEBUG_SUPPORTED
  {
    HalLcdWriteString( "EMDOOR ELECTRONICS", HAL_LCD_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "MAC-MT Coord", HAL_LCD_LINE_5 );
#else
      HalLcdWriteString( "MAC-MT Device", HAL_LCD_LINE_5 );
#endif
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "NWK Coordinator", HAL_LCD_LINE_5 );
#else
      HalLcdWriteString( "NWK Device", HAL_LCD_LINE_5 );
#endif
#endif
  }
#endif 
}
#endif
