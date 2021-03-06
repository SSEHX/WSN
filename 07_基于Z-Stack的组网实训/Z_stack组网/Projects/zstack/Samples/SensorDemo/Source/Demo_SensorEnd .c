/**************************************************************************************************
  Filename:       DemoSensor.c

  Description:    Sensor application for the sensor demo utilizing the Simple API.

                  The sensor application binds to a gateway and will periodically 
                  read temperature and supply voltage from the ADC and send report   
                  towards the gateway node.  


  Copyright 2009 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/******************************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL.h"
#include "sapi.h"
#include "hal_key.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_mcu.h"
#include "hal_uart.h"
#include "sensor.h"
#include "UART_PRINT.h"
#include "DemoApp.h"

/******************************************************************************
 * CONSTANTS
 */
#define REPORT_FAILURE_LIMIT                4
#define ACK_REQ_INTERVAL                    5 // each 5th packet is sent with ACK request

// Application States
#define APP_INIT                            0    // Initial state
#define APP_START                           1    // Sensor has joined network
#define APP_BIND                            2    // Sensor is in process of binding
#define APP_REPORT                          4    // Sensor is in reporting state

// Application osal event identifiers
// Bit mask of events ( from 0x0000 to 0x00FF )
#define MY_START_EVT                        0x0001
#define MY_REPORT_EVT                       0x0002
#define MY_FIND_COLLECTOR_EVT               0x0004
#define MY_SEND_EVT               0x0010

// ADC definitions for CC2430/CC2530 from the hal_adc.c file
#if defined (HAL_MCU_CC2530)
#define HAL_ADC_REF_125V    0x00    /* Internal 1.25V Reference */
#define HAL_ADC_DEC_064     0x00    /* Decimate by 64 : 8-bit resolution */
#define HAL_ADC_DEC_128     0x10    /* Decimate by 128 : 10-bit resolution */
#define HAL_ADC_DEC_512     0x30    /* Decimate by 512 : 14-bit resolution */
#define HAL_ADC_CHN_VDD3    0x0f    /* Input channel: VDD/3 */
#define HAL_ADC_CHN_TEMP    0x0e    /* Temperature sensor */
#endif // HAL_MCU_CC2530

/******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * LOCAL VARIABLES
 */


static uint8 appState =           APP_INIT;
static uint8 reportState =        FALSE;

static uint8 reportFailureNr =    0;

static uint16 myReportPeriod =    2303;         // milliseconds
static uint16 myBindRetryDelay =  2200;         // milliseconds

static uint16 parentShortAddr;

/******************************************************************************
 * GLOBAL VARIABLES
 */

// Inputs and Outputs for Sensor device
#define NUM_OUT_CMD_SENSOR                1
#define NUM_IN_CMD_SENSOR                 0

// List of output and input commands for Sensor device
const cId_t zb_OutCmdList[NUM_OUT_CMD_SENSOR] =
{
  SENSOR_REPORT_CMD_ID
};

// Define SimpleDescriptor for Sensor device
const SimpleDescriptionFormat_t zb_SimpleDesc =
{
  MY_ENDPOINT_ID,             //  Endpoint
  MY_PROFILE_ID,              //  Profile ID
  DEV_ID_SENSOR,              //  Device ID
  DEVICE_VERSION_SENSOR,      //  Device Version
  0,                          //  Reserved
  NUM_IN_CMD_SENSOR,          //  Number of Input Commands
  (cId_t *) NULL,             //  Input Command List
  NUM_OUT_CMD_SENSOR,         //  Number of Output Commands
  (cId_t *) zb_OutCmdList     //  Output Command List
};


/******************************************************************************
 * LOCAL FUNCTIONS
 */

void uartRxCB( uint8 port, uint8 event );
static void sendDummyReport(void);
static int8 readTemp(void);
static uint8 readinVoltage(void);

/*****************************************************************************
 * @fn          zb_HandleOsalEvent
 *
 * @brief       The zb_HandleOsalEvent function is called by the operating
 *              system when a task event is set
 *
 * @param       event - Bitmask containing the events that have been set
 *
 * @return      none
 */
void zb_HandleOsalEvent( uint16 event )
{
  if(event & SYS_EVENT_MSG)
  {
    
  }
  
  if( event & ZB_ENTRY_EVENT )
  { 
    initUart(uartRxCB);
    // blind LED 1 to indicate joining a network
    HalLedBlink ( HAL_LED_1, 0, 50, 500 );
    HalLedBlink ( HAL_LED_2, 0, 50, 500 ); 
    // Start the device 
    zb_StartRequest();
  }
  
  if ( event & MY_REPORT_EVT )
  {
    //if ( appState == APP_REPORT ) 
    {
      appState = APP_REPORT;
       HAL_TOGGLE_LED1();
       HAL_TURN_OFF_LED2();
       sendDummyReport();
      osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT, myReportPeriod+(uint8)osal_rand() );
      //osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT, myReportPeriod );
    }
  }
  if ( event & MY_FIND_COLLECTOR_EVT )
  {
    // Delete previous binding
    if ( appState==APP_REPORT ) 
    {
        zb_AllowBind( 0x00 );
        zb_BindDevice( FALSE, SENSOR_REPORT_CMD_ID, (uint8 *)NULL );
    }
    
    appState = APP_BIND;
    // blind LED 2 to indicate discovery and binding
    //HalLedBlink ( HAL_LED_2, 0, 50, 500 );
    HalLedBlink ( HAL_LED_4, 0, 10, 2000);
    HalLedSet( HAL_LED_3, HAL_LED_MODE_OFF );
    HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
    HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
    // Find and bind to a collector device
    zb_BindDevice( TRUE, SENSOR_REPORT_CMD_ID, (uint8 *)NULL );
    osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT,3000 );
     // appState =APP_REPORT;
      reportState = TRUE;
   // osal_start_timerEx( sapi_TaskID, MY_SEND_EVT, 2000 );
    
  }
  if ( event & MY_SEND_EVT )
  {
      osal_set_event( sapi_TaskID, MY_REPORT_EVT );
      appState =APP_REPORT;
      reportState = TRUE; 
      
  }
}

/******************************************************************************
 * @fn      zb_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 EVAL_SW4
 *                 EVAL_SW3
 *                 EVAL_SW2
 *                 EVAL_SW1
 *
 * @return  none
 */
void zb_HandleKeys( uint8 shift, uint8 keys )
{
   
    
    keys=0;
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
      // Start reporting
      osal_set_event( sapi_TaskID, MY_REPORT_EVT );
      reportState = TRUE;
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
}

/******************************************************************************
 * @fn          zb_StartConfirm
 *
 * @brief       The zb_StartConfirm callback is called by the ZigBee stack
 *              after a start request operation completes
 *
 * @param       status - The status of the start operation.  Status of
 *                       ZB_SUCCESS indicates the start operation completed
 *                       successfully.  Else the status is an error code.
 *
 * @return      none
 */
void zb_StartConfirm( uint8 status )
{
  // If the device sucessfully started, change state to running
  if ( status == ZB_SUCCESS ) 
  {
    // Change application state
    appState = APP_START;
    
    // Set LED 1 to indicate that node is operational on the network
    //HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
    
    // Update the display
    #if defined ( LCD_SUPPORTED )
    HalLcdWriteString( "SensorDemo", HAL_LCD_LINE_1 );
    HalLcdWriteString( "Sensor", HAL_LCD_LINE_2 );
    #endif
    
    // Store parent short address
    zb_GetDeviceInfo(ZB_INFO_PARENT_SHORT_ADDR, &parentShortAddr);
    
    // Set event to bind to a collector
    osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );  
  }
}

/******************************************************************************
 * @fn          zb_SendDataConfirm
 *
 * @brief       The zb_SendDataConfirm callback function is called by the
 *              ZigBee after a send data operation completes
 *
 * @param       handle - The handle identifying the data transmission.
 *              status - The status of the operation.
 *
 * @return      none
 */
void zb_SendDataConfirm( uint8 handle, uint8 status )
{
  if(status != ZB_SUCCESS) 
  {
    if ( ++reportFailureNr >= REPORT_FAILURE_LIMIT ) 
    {
       // Stop reporting
       osal_stop_timerEx( sapi_TaskID, MY_REPORT_EVT );
       
       // After failure reporting start automatically when the device
       // is binded to a new gateway
       reportState=TRUE;
        
       // Try binding to a new gateway
       osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );
       reportFailureNr=0;
    }
  }
  // status == SUCCESS
  else 
  {
    // Reset failure counter
    reportFailureNr=0;
  }
}

/******************************************************************************
 * @fn          zb_BindConfirm
 *
 * @brief       The zb_BindConfirm callback is called by the ZigBee stack
 *              after a bind operation completes.
 *
 * @param       commandId - The command ID of the binding being confirmed.
 *              status - The status of the bind operation.
 *
 * @return      none
 */
void zb_BindConfirm( uint16 commandId, uint8 status )
{
  if( status == ZB_SUCCESS )
  {   
    appState = APP_REPORT;
    HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
    //zb_AllowBind( 0xff );  //*******************  
    // After failure reporting start automatically when the device
    // is binded to a new gateway
    if ( reportState ) 
    {
      // Start reporting
      osal_set_event( sapi_TaskID, MY_REPORT_EVT );
    }
  }
  else
  {
    osal_start_timerEx( sapi_TaskID, MY_FIND_COLLECTOR_EVT, myBindRetryDelay );
  }
}

/******************************************************************************
 * @fn          zb_AllowBindConfirm
 *
 * @brief       Indicates when another device attempted to bind to this device
 *
 * @param
 *
 * @return      none
 */
void zb_AllowBindConfirm( uint16 source )
{
}

/******************************************************************************
 * @fn          zb_FindDeviceConfirm
 *
 * @brief       The zb_FindDeviceConfirm callback function is called by the
 *              ZigBee stack when a find device operation completes.
 *
 * @param       searchType - The type of search that was performed.
 *              searchKey - Value that the search was executed on.
 *              result - The result of the search.
 *
 * @return      none
 */
void zb_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result )
{
}

/******************************************************************************
 * @fn          zb_ReceiveDataIndication
 *
 * @brief       The zb_ReceiveDataIndication callback function is called
 *              asynchronously by the ZigBee stack to notify the application
 *              when data is received from a peer device.
 *
 * @param       source - The short address of the peer device that sent the data
 *              command - The commandId associated with the data
 *              len - The number of bytes in the pData parameter
 *              pData - The data sent by the peer device
 *
 * @return      none
 */
void zb_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{
}

/******************************************************************************
 * @fn          uartRxCB
 *
 * @brief       Callback function for UART 
 *
 * @param       port - UART port
 *              event - UART event that caused callback 
 *
 * @return      none
 */
void uartRxCB( uint8 port, uint8 event )
{
  uint8 pBuf[RX_BUF_LEN];
  uint16 len;
  EA=0;
  if ( event != HAL_UART_TX_EMPTY ) 
  {
  
    // Read from UART
    len = HalUARTRead( HAL_UART_PORT_0, pBuf, RX_BUF_LEN );
    
    if ( len>0 ) 
    {
      
       //HalUARTWrite(HAL_UART_PORT_0, pBuf, len);
         
      // 参数设置
      configset(pBuf,len, LOG_TYPE); 
      
    }
  }
   EA=1;
}

/******************************************************************************
 * @fn          sendReport
 *
 * @brief       获取并发送传感数据
 *
 * @param       none
 *              
 * @return      none
 */

#define SENSOR_LENGTH              8

static void sendDummyReport(void)
{
  uint8 pData[SENSOR_LENGTH+4];
  static uint8 reportNr=0;
  uint8 txOptions;
  uint16 val1,val2;
  //读取芯片内部温度和电源电压
  pData[SENSOR_TEMP_OFFSET] =  readTemp();
  pData[SENSOR_VOLTAGE_OFFSET] = readinVoltage(); 
  //读取父节点地址  
  pData[SENSOR_PARENT_OFFSET] =  LO_UINT16(parentShortAddr);
  pData[SENSOR_PARENT_OFFSET+ 1] = HI_UINT16(parentShortAddr);
  //传感数据
  pData[SENSOR_DATA_OFFSET+0]=LOG_TYPE;     //逻辑类型
  pData[SENSOR_DATA_OFFSET+1]=serl;     //传感器编号低位
  pData[SENSOR_DATA_OFFSET+2]=serh;     //传感器编号高位
  pData[SENSOR_DATA_OFFSET+3]=Dtype;     // 传感器类型
  //获取传感值
  readsensor(Dtype,&val1,&val2);
  pData[SENSOR_VAL_OFFSET+0]=LO_UINT16(val1); 
  pData[SENSOR_VAL_OFFSET+1]=HI_UINT16(val1);
  pData[SENSOR_VAL_OFFSET+2]=LO_UINT16(val2); 
  pData[SENSOR_VAL_OFFSET+3]=HI_UINT16(val2);
  //通过zignee无线发送
  // Set ACK request on each ACK_INTERVAL report
  // If a report failed, set ACK request on next report
  if ( ++reportNr<ACK_REQ_INTERVAL && reportFailureNr==0 ) 
  {
    txOptions = AF_TX_OPTIONS_NONE;
  }  
  else 
  {
    txOptions = AF_MSG_ACK_REQUEST;
    reportNr = 0;
  }
#if defined UART_LOOK  
    //HalUARTWrite(HAL_UART_PORT_0, pData,SENSOR_LENGTH+4);
    uart_printf("类型：0x%02x, 传感数值：%d, %d\r\n", Dtype, val1,val2);
#endif  
  // Destination address 0xFFFE: Destination address is sent to previously
  // established binding for the commandId.
  zb_SendDataRequest( 0xFFFE, SENSOR_REPORT_CMD_ID, SENSOR_LENGTH+4, pData, 0, txOptions, 0 );
}


/******************************************************************************
 * @fn          calcFCS
 *
 * @brief       This function calculates the FCS checksum for the serial message 
 *
 * @param       pBuf - Pointer to the end of a buffer to calculate the FCS.
 *              len - Length of the pBuf.
 *
 * @return      The calculated FCS.
 ******************************************************************************
 */
static uint8 calcFCS(uint8 *pBuf, uint8 len)
{
  uint8 rtrn = 0;

  while (len--)
  {
    rtrn ^= *pBuf++;
  }

  return rtrn;
}


/******************************************************************************
 * @fn          readTemp
 *
 * @brief       read temperature from ADC
 *
 * @param       none
 *              
 * @return      temperature
 */
static int8 readTemp(void)
{
  static uint16 voltageAtTemp22;
  static uint8 bCalibrate=TRUE; // Calibrate the first time the temp sensor is read
  uint16 value;
  int8 temp1;

  
  ATEST = 0x01;
  TR0  |= 0x01; 
  
  /* Clear ADC interrupt flag */
  ADCIF = 0;

  ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_512 | HAL_ADC_CHN_TEMP);

  /* Wait for the conversion to finish */
  while ( !ADCIF );

  /* Get the result */
  value = ADCL;
  value |= ((uint16) ADCH) << 8;

  // Use the 12 MSB of adcValue
  value >>= 4;
  
  /*
   * These parameters are typical values and need to be calibrated
   * See the datasheet for the appropriate chip for more details
   * also, the math below may not be very accurate
   */
    /* Assume ADC = 1480 at 25C and ADC = 4/C */
  #define VOLTAGE_AT_TEMP_25        1480
  #define TEMP_COEFFICIENT          4

  // Calibrate for 22C the first time the temp sensor is read.
  // This will assume that the demo is started up in temperature of 22C
  if(bCalibrate) {
    voltageAtTemp22=value;
    bCalibrate=FALSE;
  }
  ATEST = 0x00;
  TR0  &= ~0x01;    // 要清，不然影响P0_1
  temp1 = 25 + ( (value - voltageAtTemp22) / TEMP_COEFFICIENT );
  //temp1=temp;
  // Set 0C as minimum temperature, and 100C as max
  return temp1;
  
  // Only CC2530 is supported
 
}

/******************************************************************************
 * @fn          readinVoltage
 *
 * @brief       read voltage from ADC intel
 *
 * @param       none
 *              
 * @return      voltage
 */
static uint8 readinVoltage(void)
{
  
  uint16 value;

  // Clear ADC interrupt flag 
  ADCIF = 0;

  ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_128 | HAL_ADC_CHN_VDD3);

  // Wait for the conversion to finish 
  while ( !ADCIF );

  // Get the result
  value = ADCL;
  value |= ((uint16) ADCH) << 8;

  
  // value now contains measurement of Vdd/3
  // 0 indicates 0V and 32767 indicates 1.25V
  // voltage = (value*3*1.15)/32767 volts
  // we will multiply by this by 10 to allow units of 0.1 volts
  value = value >> 6;   // divide first by 2^6
  value = (uint16)(value * 34.5);
  value = value >> 9;   // ...and later by 2^9...to prevent overflow during multiplication

  return value;
  
}