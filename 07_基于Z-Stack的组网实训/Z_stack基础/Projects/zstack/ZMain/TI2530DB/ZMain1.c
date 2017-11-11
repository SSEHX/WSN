/*******************************************************************************
 * ���W�١GZMain.c
 * �\    ��Gmain��ƤJ�f�C
 * �@    �̡GPOWER
 * ��    ���G2010-03-08
 ******************************************************************************/


/* �]�t�Y��� */
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


/* �t�ιB��e�̤jVDD�˴����˦��� */
#define MAX_VDD_SAMPLES  3
#define ZMAIN_VDD_LIMIT  HAL_ADC_VDD_LIMIT_4


/* �~����� */
/********************************************************************/
extern bool HalAdcCheckVdd (uint8 limit);


/* ���a��� */
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
 * ��ƦW�١Gmain
 * �\    ��G�D��ơC
 * �J�f�ѼơG�L
 * �X�f�ѼơG�L
 * �� �^ �ȡG�L
 ********************************************************************/
int main( void )
{
  /* �������_ */
  osal_int_disable( INTS_ALL );

  /* ��l�ƨt�ή�����LED�� */
  HAL_BOARD_INIT();

  /* �˴��ѹq�q�� */ 
  zmain_vdd_check();

  /* ��l�ư�� */
  zmain_ram_init();

  /* ��l�ƥD�O�~��I/O */
  InitBoard( OB_COLD );

  /* ��l�Ƶw���H�h�X�� */
  HalDriverInit();

  /* ��l�ƨt��NV�D�����ʦs�x */
  osal_nv_init( NULL );

  /* ��l�ư�¦NV�� */
  zgInit();

  /* ��l��MAC */ 
  ZMacInit();

  /* �T�w�X�i�a�} */
  zmain_ext_addr();

  /* ��l�����ήج[ */
#ifndef NONWK
  afInit(); // AF���ήج[���O�t�Ϊ����ȡA�]���եΥ�����l�Ƶ{��
#endif

  /* ��l�ƾާ@�t�� */
  osal_init_system();

  /* ���\���_ */
  osal_int_enable( INTS_ALL );

  /* �̲תO�Ū�l�� */
  InitBoard( OB_READY );

  /* ��ܸӳ]�ƫH�� */
  zmain_dev_info();

  /* �p�G�w�q�FLCD�A�h�bLCD�W��ܳ]�ƫH�� */
#ifdef LCD_SUPPORTED
  zmain_lcd_init();
#endif

#ifdef WDT_IN_PM1
  /* �p�G�ݪ����Q�ϥΡA���B�ϯ� */
  WatchDogEnable( WDTIMX );
#endif

  osal_start_system(); // �i�J�t�νիסA�L��^

  return ( 0 );
}


/*********************************************************************
 * ��ƦW�١Gzmain_vdd_check
 * �\    ��G�t�ιq���˴���ơC�ˬdVDD�O�_���`
 * �J�f�ѼơG�L
 * �X�f�ѼơG�p�GVDD�����`�A����{�{LED�C
 * �� �^ �ȡG�L
 ********************************************************************/
static void zmain_vdd_check( void )
{
  uint8 vdd_passed_count = 0;
  bool toggle = 0;

  /* �˴����`���ƬO�_�F��]�w�� */
  while ( vdd_passed_count < MAX_VDD_SAMPLES )
  {
    /* �p�GVDD�q���˴��q�L */
    if ( HalAdcCheckVdd (ZMAIN_VDD_LIMIT) )
    {
      vdd_passed_count++;    // ���`�q�����˦��ƥ[1
      MicroWait (10000);     // ���� 10�@��᭫�s����
    }
    /* �p�GVDD�q���˴����q�L */
    else
    {
      vdd_passed_count = 0;  // �M�s�p�ƭ�
      MicroWait (50000);     // ����50�@��
      MicroWait (50000);     // �~�򵥫�50�@��᭫�s����
    }

    /* �p�GVDD�q���˴����q�L�A�h����LED1 LED2�G�����A */
    if (vdd_passed_count == 0)
    {
      if ((toggle = !(toggle)))
        HAL_TOGGLE_LED1();
      else
        HAL_TOGGLE_LED2();
    }
  }

  /* ����LED1 LED2 */
  HAL_TURN_OFF_LED1();
  HAL_TURN_OFF_LED2();
}


/*********************************************************************
 * ��ƦW�١Gzmain_ext_addr
 * �\    ��G�T�w�~���a�}�C�����X�i�a�}�N�����u���ŷj���åB�N���G
 *           �g�J��t��NV�D�����ʦs�x�����C
 * �J�f�ѼơG�L
 * �X�f�ѼơG�L
 * �� �^ �ȡG�L
 ********************************************************************/
static void zmain_ext_addr(void)
{
  uint8 nullAddr[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 writeNV = TRUE;

  /* �����ˬd�t��NV���O�_�s�b�X�i�a�} */
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL))  ||
      (SUCCESS != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress)) ||
      (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN)))
  {
    /* Ū���X�i�a�} */
    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, aExtendedAddress, Z_EXTADDR_LEN);

    if (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN))
    {
      if (!osal_memcmp((uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), nullAddr, Z_EXTADDR_LEN))
      {
        osal_memcpy(aExtendedAddress, (uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), Z_EXTADDR_LEN);
      }
      else  // ����즳���X�i�a�}
      {
        uint8 idx;
        
#if !defined ( NV_RESTORE )
        writeNV = FALSE;  // �N�����{��IEEE�a�}�A���Q�g�JNV
#endif

        /* �����X�i�a�}�H���� */
        for (idx = 0; idx < (Z_EXTADDR_LEN - 2);)
        {
          uint16 randy = osal_rand();
          aExtendedAddress[idx++] = LO_UINT16(randy);
          aExtendedAddress[idx++] = HI_UINT16(randy);
        }
        
        /* ZigBee�]���������Ѳ� */
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

  /* �ھڤW�z�]�mMAC�X�i�a�} */
  (void)ZMacSetReq(MAC_EXTENDED_ADDRESS, aExtendedAddress);
}


/*********************************************************************
 * ��ƦW�١Gzmain_dev_info
 * �\    ��G�bLCD�W���IEEE�a�}�C
 * �J�f�ѼơG�L
 * �X�f�ѼơG�L
 * �� �^ �ȡG�L
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
  
  /* ����X�i�a�} */
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
 * ��ƦW�١Gzmain_ram_init
 * �\    ��G��l��RAM��̡C
 * �J�f�ѼơG�L
 * �X�f�ѼơG�L
 * �� �^ �ȡG�L
 ********************************************************************/
static void zmain_ram_init( void )
{
  uint8 *end;
  uint8 *ptr;

  /* ��l�ƽեΡ]�Ѽơ^��� */
  end = (uint8*)CSTK_BEG;
  ptr = (uint8*)(*( __idata uint16*)(CSTK_PTR));
  while ( --ptr > end )
    *ptr = STACK_INIT_VALUE;

  /* ��l�ƪ�^�]�a�}�^��� */
  ptr = (uint8*)RSTK_END - 1;
  while ( --ptr > (uint8*)SP )
    *(__idata uint8*)ptr = STACK_INIT_VALUE;
}


#ifdef LCD_SUPPORTED
/*********************************************************************
 * ��ƦW�١Gzmain_lcd_init
 * �\    ��G�b�ҰʹL�{����l��LCD�C
 * �J�f�ѼơG�L
 * �X�f�ѼơG�L
 * �� �^ �ȡG�L
 ********************************************************************/
static void zmain_lcd_init ( void )
{
#ifdef SERIAL_DEBUG_SUPPORTED
  {
    HalLcdWriteString( "SIKAIELECTRONICS", HAL_LCD_LINE_1 );

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
