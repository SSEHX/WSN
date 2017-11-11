/**************************************************************************************************
  Filename:       DemoCollector.c

  Description:    Collector application for the Sensor Demo utilizing Simple API.

                  The collector node can be set in a state where it accepts 
                  incoming reports from the sensor nodes, and can send the reports
                  via the UART to a PC tool. The collector node in this state
                  functions as a gateway. The collector nodes that are not in the
                  gateway node function as routers in the network.  
                  
**************************************************************************************************/

/******************************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "sapi.h"
#include "hal_key.h"
#include "hal_led.h"
//#include "hal_lcd.h"
#include "hal_uart.h"
#include "DemoApp.h"

#include "OnBoard.h"  // ++by cl


/******************************************************************************
 * CONSTANTS
 */

#define REPORT_FAILURE_LIMIT               4
#define ACK_REQ_INTERVAL                      5   // each 5th packet is sent with ACK request

// General UART frame offsets
#define FRAME_SOF_OFFSET                     0
#define FRAME_LENGTH_OFFSET               1 
#define FRAME_CMD0_OFFSET                   2
#define FRAME_CMD1_OFFSET                   3
#define FRAME_DATA_OFFSET                   4

// ZB_RECEIVE_DATA_INDICATION offsets
#define ZB_RECV_SRC_OFFSET                  0
#define ZB_RECV_CMD_OFFSET                  2
#define ZB_RECV_LEN_OFFSET                  4
#define ZB_RECV_DATA_OFFSET                 6
#define ZB_RECV_FCS_OFFSET                  8

// ZB_RECEIVE_DATA_INDICATION frame length
#define ZB_RECV_LENGTH                      11+SENSOR_REPORT_LENGTH //15

// PING response frame length and offset
#define SYS_PING_RSP_LENGTH                 7 
#define SYS_PING_CMD_OFFSET                 1

// Stack Profile
#define ZIGBEE_2007                             0x0040
#define ZIGBEE_PRO_2007                     0x0041

#ifdef ZIGBEEPRO
#define STACK_PROFILE                       ZIGBEE_PRO_2007             
#else 
#define STACK_PROFILE                       ZIGBEE_2007
#endif

#define CPT_SOP                             0xFE
#define SYS_PING_REQUEST                    0x0021
#define SYS_PING_RESPONSE                   0x0161
#define ZB_RECEIVE_DATA_INDICATION          0x8746

// Application States
#define APP_INIT                            0
#define APP_START                           2
#define APP_BINDED                          3    

// Application osal event identifiers
#define MY_START_EVT                        0x0001
#define MY_REPORT_EVT                       0x0002
#define MY_FIND_COLLECTOR_EVT               0x0004





/******************************************************************************
 * LOCAL VARIABLES
 */

static uint8 appState = APP_INIT;
static uint8 reportState = FALSE;
static uint8 myStartRetryDelay = 10;           // milliseconds
static uint8 isGateWay = FALSE;
static uint16 myBindRetryDelay = 2000;       // milliseconds
static uint16 myReportPeriod = 2000;          // milliseconds

static uint8 reportFailureNr = 0;
static uint16 parentShortAddr;
static uint16 myShortAddr;

uint8 flag=0;


/******************************************************************************
 * LOCAL FUNCTIONS
 */

static uint8 calcFCS(uint8 *pBuf, uint8 len);
static void sysPingReqRcvd(void);
static void sysPingRsp(void);
static void sendDummyReport(void);



/******************************************************************************
 * GLOBAL VARIABLES
 */

// Inputs and Outputs for Collector device
#define NUM_OUT_CMD_COLLECTOR                2
#define NUM_IN_CMD_COLLECTOR                   2

// List of output and input commands for Collector device
const cId_t zb_InCmdList[NUM_IN_CMD_COLLECTOR] =
{
  	SENSOR_REPORT_CMD_ID,
       DUMMY_REPORT_CMD_ID
};

const cId_t zb_OutCmdList[NUM_OUT_CMD_COLLECTOR] =
{
	SENSOR_REPORT_CMD_ID,
 	DUMMY_REPORT_CMD_ID
};

// Define SimpleDescriptor for Collector device
const SimpleDescriptionFormat_t zb_SimpleDesc =
{
  	MY_ENDPOINT_ID,             		//  Endpoint
  	MY_PROFILE_ID,              		//  Profile ID
  	DEV_ID_COLLECTOR,           	//  Device ID
  	DEVICE_VERSION_COLLECTOR,   //  Device Version
  	0,                          			//  Reserved
  	NUM_IN_CMD_COLLECTOR,       	//  Number of Input Commands
  	(cId_t *) zb_InCmdList,     		//  Input Command List
  	NUM_OUT_CMD_COLLECTOR,      //  Number of Output Commands
  	(cId_t *) zb_OutCmdList     	//  Output Command List
};

/******************************************************************************
 * FUNCTIONS
 */

/******************************************************************************
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
	uint8 logicalType;
  
 	if(event & SYS_EVENT_MSG)   //0x8000
	{

	}
  
  	if( event & ZB_ENTRY_EVENT ) //0x1000
  	{  
    		// Initialise UART
    		initUart(uartRxCB);

		// blind LED 1 to indicate starting/joining a network
    		HalLedBlink ( HAL_LED_1, 0, 50, 500 );
    		HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
    
    		// Read logical device type from NV
    		zb_ReadConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType);  //default  logicalType == 0x01 (router); if config coordinator logicalType == 0x00;
   
    		// Start the device 
    		zb_StartRequest();


  	}
  
  	if ( event & MY_START_EVT ) //0x0001
  	{
    		zb_StartRequest();
  	}
  
  	if ( event & MY_REPORT_EVT ) //0x0002
  	{
    		if (isGateWay)
    		{
			osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT, myReportPeriod );
    		}
    		else if (appState == APP_BINDED) 
    		{
			#if defined SENSOR_SHT10
			//call_sht11();
			#endif
			//sendDummyReport();
      			osal_start_timerEx( sapi_TaskID, MY_REPORT_EVT, myReportPeriod );
    		}
  	}

	
	
  	if ( event & MY_FIND_COLLECTOR_EVT )//0x0004
  	{

		zb_ReadConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType);
                HalUARTWrite(HAL_UART_PORT_0,"  \r\n", 4);
		if(logicalType == 0x00)
		{
			zb_AllowBind( 0xFF );
			HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
			isGateWay = TRUE;	
		}
		else if(logicalType == 0x01)
		{
			// Turn OFF Allow Bind mode infinitly
			zb_AllowBind( 0x00 );
			HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
			isGateWay = FALSE;

			zb_BindDevice( TRUE, DUMMY_REPORT_CMD_ID, (uint8 *)NULL );
		
		}		
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
    		// Set LED 1 to indicate that node is operational on the network
    		HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
    
		// Change application state
		appState = APP_START;

		// Set event to bind to a collector
		osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );

		// Store parent short address
		zb_GetDeviceInfo(ZB_INFO_PARENT_SHORT_ADDR, &parentShortAddr);		
		
		zb_GetDeviceInfo(ZB_INFO_SHORT_ADDR, &myShortAddr);
		
  	}
  	else
  	{
    		// Try again later with a delay
    		osal_start_timerEx( sapi_TaskID, MY_START_EVT, myStartRetryDelay );
		
  	}
}

/******************************************************************************
 * @fn          zb_SendDataConfirm
 *
 * @brief       The zb_SendDataConfirm callback function is called by the
 *              ZigBee stack after a send data operation completes
 *
 * @param       handle - The handle identifying the data transmission.
 *              status - The status of the operation.
 *
 * @return      none
 */
void zb_SendDataConfirm( uint8 handle, uint8 status )
{
	if ( status != ZB_SUCCESS && !isGateWay ) 
	{

		if ( ++reportFailureNr>=REPORT_FAILURE_LIMIT ) 
		{   
			// Stop reporting
			osal_stop_timerEx( sapi_TaskID, MY_REPORT_EVT );

			// After failure reporting start automatically when the device
			// is binded to a new gateway
			reportState=TRUE;

			// Delete previous binding
			zb_BindDevice( FALSE, DUMMY_REPORT_CMD_ID, (uint8 *)NULL );

			// Try binding to a new gateway
			osal_set_event( sapi_TaskID, MY_FIND_COLLECTOR_EVT );
			reportFailureNr=0;
		}
	}
	else if ( !isGateWay ) 
	{
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
		appState = APP_BINDED;
		// Set LED2 to indicate binding successful
		HalLedSet ( HAL_LED_2, HAL_LED_MODE_ON );

		// After failure reporting start automatically when the device
		// is binded to a new gateway
		if ( reportState ) 
		{
			// Start reporting
			osal_set_event( sapi_TaskID, MY_REPORT_EVT );
		}

		osal_set_event( sapi_TaskID, MY_REPORT_EVT );

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
 * @brief       ���յ�zigbee��Ϣ���͵�����
 * @param       source - The short address of the peer device that sent the data
 *              command - The commandId associated with the data
 *              len - The number of bytes in the pData parameter
 *              pData - The data sent by the peer device
 *
 * @return      none
 */
void zb_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{ 
     HAL_TOGGLE_LED2(); //�յ�zigbee��Ϣָʾ��
     //zigbee��Ϣ���͵�����
     HalUARTWrite(HAL_UART_PORT_0,pData, len);
}


/******************************************************************************
 * @fn          uartRxCB
 *
 * @brief       Callback function for UART 
 *
 * @param       port - UART port
 *                    event - UART event that caused callback 
 *
 * @return      none
 */
void uartRxCB( uint8 port, uint8 event )
{
	uint8 pBuf[RX_BUF_LEN];
	uint16 cmd;
	uint16 len;

	if ( event != HAL_UART_TX_EMPTY ) 
	{
		// Read from UART
		len = HalUARTRead( HAL_UART_PORT_0, pBuf, RX_BUF_LEN );

		if ( len>0 ) 
		{
			cmd = BUILD_UINT16(pBuf[SYS_PING_CMD_OFFSET+ 1], pBuf[SYS_PING_CMD_OFFSET]);

			if( (pBuf[FRAME_SOF_OFFSET] == CPT_SOP) && (cmd == SYS_PING_REQUEST) ) 
			{
				sysPingReqRcvd();
			}
		}
	}
}

/******************************************************************************
 * @fn          sysPingReqRcvd
 *
 * @brief       Ping request received 
 *
 * @param       none
 *              
 * @return      none
 */
static void sysPingReqRcvd(void)
{
	sysPingRsp();
}

/******************************************************************************
 * @fn          sysPingRsp
 *
 * @brief       Build and send Ping response
 *
 * @param       none
 *              
 * @return      none
 */
static void sysPingRsp(void)
{
	uint8 pBuf[SYS_PING_RSP_LENGTH];

	// Start of Frame Delimiter
	pBuf[FRAME_SOF_OFFSET] = CPT_SOP;

	// Length
	pBuf[FRAME_LENGTH_OFFSET] = 2; 

	// Command type
	pBuf[FRAME_CMD0_OFFSET] = LO_UINT16(SYS_PING_RESPONSE); 
	pBuf[FRAME_CMD1_OFFSET] = HI_UINT16(SYS_PING_RESPONSE);

	// Stack profile
	pBuf[FRAME_DATA_OFFSET] = LO_UINT16(STACK_PROFILE);
	pBuf[FRAME_DATA_OFFSET+ 1] = HI_UINT16(STACK_PROFILE);

	// Frame Check Sequence
	pBuf[SYS_PING_RSP_LENGTH - 1] = calcFCS(&pBuf[FRAME_LENGTH_OFFSET], (SYS_PING_RSP_LENGTH - 2));

	// Write frame to UART
	HalUARTWrite(HAL_UART_PORT_0,pBuf, SYS_PING_RSP_LENGTH);
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

































