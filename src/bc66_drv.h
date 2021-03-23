/**
 *
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @copyright   Juan Cruz Becerra
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @file    bc66_drv.h
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @brief
 * BC66 NB-IoT modem driver. (https://www.quectel.com/product/bc66.htm)
 * 
 * AT Command Syntax
 * The AT or at prefix must be set at the beginning of each command line.
 * Entering <CR> will terminate a command line. 
 * Commands are usually followed by a response that includes <CR><LF><response><CR><LF>.
 * Throughout this document, only the responses are presented, <CR><LF> are omitted intentionally.
 * 
 * Types of AT Commands and Responses
 * - Test Command AT+<x>=?
 * - Read Command AT+<x>?
 * - Write Command AT+<x>=<n>
 * - Execution Command AT+<x>
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @date    03/15/2021
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @author    Eng. Juan Cruz Becerra
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @version    1.0.0
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//*****************************************************************************
/**
 * 
 */
typedef struct {
	const void (*func_init_ptr)(); 							///< uart initialize function pointer
	const void (*func_delay)(uint32_t t);						///< delay function pointer
	int (*func_w_bytes_ptr)(uint8_t * txc, uint8_t size); 	///< write bytes function pointer
	int (*func_r_bytes_ptr)(uint8_t * rxc ); 				///< read one-byte function pointer
	struct  {
		void (*MDM_PSM_EINT_N)(size_t pin_value);			///< delay function pointer
		void (*MDM_PWRKEY_N)(size_t pin_value);				///< modem power key function pointer
		void (*MDM_RESET_N)(size_t pin_value);				///< modem reset function pointer
		void (*MDM_RI)();									///< modem ring interrupt function pointer
	}control_lines;
} bc66_obj_t ;

//*****************************************************************************
/// AT command posibility. Erch command can test and/or read and/or write and/or execute. Use with \p bc66_send_at_command(...) function.
typedef enum { 
	BC66_CMD_TEST , 				///< Send AT TEST command.
	BC66_CMD_READ,					///< Send AT READ command.
	BC66_CMD_WRITE,					///< Send AT WRITE command.
	BC66_CMD_EXE					///< Send AT TEST command.
} bc66_cmd_type_t ;

/// This is the commands implemented list. 
typedef enum { 
	/* 1- AT command */
	bc66_cmd_list_AT,				///< AT command. Use to sync baud rate. 
	/* 2- Product Information Query Commands */
	bc66_cmd_list_ATI,				///< Display Product Identification Information
	/* 3- UART function commands */
	bc66_cmd_list_ATE,				///< Set Command Echo Mode
	/* 4- Network State Query Commands */
	bc66_cmd_list_CEREG,			///< EPS Network Registration Status
	bc66_cmd_list_CESQ,				///< Extended Signal Quality
	bc66_cmd_list_CGATT,			///< PS Attachment or Detachment
	bc66_cmd_list_CGPADDR,			///< Show PDP Addresses

	/* PDN and APN Commands */
	bc66_cmd_list_QCGDEFCONT,		///< Set Default PSD Connection Settings
	/* 6- Other Network Commands */

	/* 7- USIM Related Commands */
	bc66_cmd_list_CIMI,				///< Request International Mobile Subscriber Identity
	bc66_cmd_list_CPIN,				///< Enter PIN
	/* 8- Power Consumption Commands */ 
	bc66_cmd_list_CPSMS,			///< Power Saving Mode Setting
	bc66_cmd_list_QNBIOTEVENT,		///< Enable/Disable NB-IoT Related Event Report
	/* 9- Platform Related Commands */ 
	
	/* 10- Time-related Commands */
	
	/* 11- Other Related Commands */ 
	bc66_cmd_list_QMTCFG,			///< Configure Optional Parameters of MQTT
	bc66_cmd_list_QMTOPEN,			///< Open a Network for MQTT Client
	bc66_cmd_list_QMTCLOSE,			///< Close a Network for MQTT Client
	bc66_cmd_list_QMTCONN,			///< Connect a Client to MQTT Server
	bc66_cmd_list_QMTDISC,			///< Disconnect a Client from MQTT Server
	bc66_cmd_list_QMTSUB,			///< Subscribe to Topics
	bc66_cmd_list_QMTUNS,			///< Unsubscribe from Topics
	bc66_cmd_list_QMTPUB,			///< Publish Messages
	/* No command - list size */
	bc66_cmd_list_size				///< Is not a command. Only to know commands quantity.
} bc66_cmd_list_t ;

//*****************************************************************************
/**
 * @brief 
 * Function to initialize bc66 object. 
 * 
 * @param bc66_obj 
 */
void bc66_init( bc66_obj_t * bc66_obj );

//*****************************************************************************
/**
 * @brief 
 * Function to get any response stored in the RX buffer.
 * 
 * @param rsp	: response to get 
 * 
 * @return 
 * Response if found, NULL otherwise
 */
char * bc66_get_at_response( char * rsp );

//*****************************************************************************
/**
 * @brief 
 * Function to send at command sentence to bc66 module through an external function communication. 
 * 
 * @param cmd_type	: BC66_CMD_TEST, BC66_CMD_READ, BC66_CMD_WRITE or BC66_CMD_EXE type.
 * @param cmd_lst 	: command to send (see command list). 
 * @param rsp 		: pointer to expected response text. 
 * @param arg_fmt 	: arguments format (like printf function) and must be sended all arguments too.
 * 
 * @return 
 * - Command aswer text
 * - OK
 * - ERROR
 * - TIMEOUT
 */
char * bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *exp_rsp, const char * arg_fmt, ...);

//*****************************************************************************
/**
 * @brief
 * Reset the module when PIN is low.
 */
void bc66_reset( void );

//*****************************************************************************
/**
 * @brief
 * Pull down PWRKEY to turn on the module
 */
void bc66_power_on();

//*****************************************************************************
/**
 * @brief
 * Pull up PWRKEY to turn off the module
 */
void bc66_power_off();


