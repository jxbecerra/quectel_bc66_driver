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
 * The â€œATâ€� or â€œatâ€� prefix must be set at the beginning of each command line.
 * Entering <CR> will terminate a command line. 
 * Commands are usually followed by a response that includes â€œ<CR><LF><response><CR><LF>â€�.
 * Throughout this document, only the responses are presented, â€œ<CR><LF>â€� are omitted intentionally.
 * 
 * Types of AT Commands and Responses
 * - Test Command AT+<x>=?
 * - Read Command AT+<x>?
 * - Write Command AT+<x>=<â€¦>
 * - Execution Command AT+<x>
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @date    15 de marzo de 2021, 10:06
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "bc66_drv.h"


#define BC66_CMD_END_LINE	"\r"

#define RSP_OK 			"OK"
#define RSP_ERROR 		"ERROR"


/**
 * AT Command Syntax
 * The â€œATâ€� or â€œatâ€� prefix must be set at the beginning of each command line.
 * Entering <CR> will terminate a command line. 
 * Commands are usually followed by a response that includes â€œ<CR><LF><response><CR><LF>â€�.
 * Throughout this document, only the responses are presented, â€œ<CR><LF>â€� are omitted intentionally.
 * 
 * Types of AT Commands and Responses
 * - Test Command AT+<x>=?
 * - Read Command AT+<x>?
 * - Write Command AT+<x>=<â€¦>
 * - Execution Command AT+<x>
 */

//*****************************************************************************
// global working buffers
static uint8_t tx_buffer[128];
static uint8_t rx_buffer[128];

// pointer to once object instance 
static bc66_obj_t *bc66 = NULL;

//*****************************************************************************
// flags commands implementation 
typedef enum { 
	TEST	= 0x1,
	READ 	= 0x2,
	WRITE 	= 0x4,
	EXE 	= 0x8
} cmd_flgs_t ;

//
typedef const struct
{
	const char *cmd;			///> at command sentence 
	cmd_flgs_t cmd_flags;		///> flags for command implementation (see @code flags enum)
	char *rsp;					///> response buffer pointer 
	uint16_t rsp_timeout;		///> response timeout [ms]
} bc66_at_cmd_t;

//*****************************************************************************
// Define AT commands list: order must be equal to commands definition enum bc66_cmd_list_t
const bc66_at_cmd_t bc66_cmds_list[] = {
	{
		.cmd = "AT",
		.cmd_flags = EXE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

// Product Information Query Commands
	{
		.cmd = "ATI",
		.cmd_flags = EXE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

// UART Function Commands
	{
		.cmd = "ATE",
		.cmd_flags = EXE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

// Network State Query Commands
	{
		.cmd = "AT+CESQ",
		.cmd_flags = TEST | EXE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

	{
		.cmd = "AT+CEREG",
		.cmd_flags = TEST | READ | WRITE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

// USIM Related Commands
	{
		.cmd = "AT+CIMI",
		.cmd_flags = TEST | EXE,
		.rsp = RSP_OK,
		.rsp_timeout = 300,
	},

	{
		.cmd = "AT+CPIN",
		.cmd_flags = TEST | READ | WRITE,
		.rsp = RSP_OK,
		.rsp_timeout = 5000,
	}
};

//*****************************************************************************
/**
 * @brief 
 * Function to initialize bc66 object. 
 * 
 * @param bc66_obj 
 */
void bc66_init(bc66_obj_t *bc66_obj)
{
	if (bc66 == 0)
	{
		memset(tx_buffer,0,sizeof(tx_buffer));
		memset(rx_buffer,0,sizeof(rx_buffer));
		// call to uart (hal) initialize function
		bc66_obj->func_init_ptr();
		// set local object pointer 
		bc66 = bc66_obj;
	}
}

//*****************************************************************************
/**
 * @brief 
 * Function to initialize bc66 object. 
 * 
 * @param bc66_obj 
 */
void bc66_deinit(bc66_obj_t *bc66_obj)
{
	// clear local object pointer 
	bc66 = NULL;
}

//*****************************************************************************
/**
 * 
 */
char * bc66_wait_at_response(const char * rsp, uint16_t size)
{
	memset(rx_buffer,0,sizeof(rx_buffer));
	bc66->func_r_bytes_ptr(rx_buffer,strlen(rsp));
	return (char*)rx_buffer;
}

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
 * Module response text.
 */
char * bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *rsp, const char * arg_fmt, ...)
{
	// check if object was initialized
	if( bc66 == NULL ) { 
		return NULL;
	}

	switch( cmd_type )
	{
		case BC66_CMD_TEST:
			if( bc66_cmds_list[cmd_lst].cmd_flags & TEST ) {
				sprintf((char*)tx_buffer,"%s=?\r",bc66_cmds_list[cmd_lst].cmd);
				bc66->func_w_bytes_ptr((uint8_t*)tx_buffer,strlen((const char*)tx_buffer));
			}
			break;

		case BC66_CMD_READ:
			if( bc66_cmds_list[cmd_lst].cmd_flags & READ ) {
				sprintf((char*)tx_buffer,"%s?\r",bc66_cmds_list[cmd_lst].cmd);
				bc66->func_w_bytes_ptr((uint8_t*)tx_buffer,strlen((const char*)tx_buffer));
			}
			break;

		case BC66_CMD_WRITE:
			if( bc66_cmds_list[cmd_lst].cmd_flags & WRITE ) {
				sprintf((char*)tx_buffer,"%s=",bc66_cmds_list[cmd_lst].cmd);
				if( arg_fmt ) { 
					// declare list of arguments 
					va_list args;
					// initialize the list 
					va_start ( args, arg_fmt );
					vsprintf((char*)&tx_buffer[strlen((const char *)tx_buffer)], (const char *)arg_fmt, args);
					// clear the list 
					va_end(args);
					// send command 
					strcat((char*)tx_buffer,"\r");
					bc66->func_w_bytes_ptr((uint8_t*)tx_buffer,strlen((const char*)tx_buffer));
				}
			}
			break;

		case BC66_CMD_EXE:
			if( bc66_cmds_list[cmd_lst].cmd_flags & EXE ) {
				bc66->func_w_bytes_ptr((uint8_t*)bc66_cmds_list[cmd_lst].cmd,strlen((const char*)bc66_cmds_list[cmd_lst].cmd));
			}
			break;

		default:	
			return "Command type not reconized";
			break;
	}

	if( rsp || bc66_cmds_list[cmd_lst].rsp ) { 
		return bc66_wait_at_response((const char*)rsp,strlen((const char*)rsp));
	}

	return (char*)rx_buffer;
}

//*****************************************************************************
/**
 *
 */
void bc66_reset( void )
{
	if( bc66 ) {
		bc66->control_lines.MDM_RESET_N();
	}
}
//*****************************************************************************
/**
 *
 */
void bc66_power_on()
{
	if( bc66 ) {
		bc66->control_lines.MDM_PWRKEY_N();
	}
}
//*****************************************************************************
/**
 *
 */
void bc66_power_off()
{
	if( bc66 ) {
		bc66->control_lines.MDM_PWRKEY_N();
	}
}
//*****************************************************************************
/**
 * 
 */
void bc66_thread(void)
{
	; /* periodically call at command parser service */
}
