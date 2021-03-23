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


#define CMD_END_LINE	"\r\n"

#define RSP_OK 			"\r\nOK\r\n"
#define RSP_ERROR 		"\r\nERROR\r\n"
#define RSP_END_OF_LINE	"\r\n"

#define MAX_RSP_SIZE	64	///< Max AT response size

/**
 * AT Command Syntax
 * The AT or at prefix must be set at the beginning of each command line.
 * Entering <CR> will terminate a command line. 
 * Commands are usually followed by a response that includes <CR><LF><response><CR><LF>.
 * Throughout this document, only the responses are presented, <CR><LF> are omitted intentionally.
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

// global connection variables 
uint8_t TCP_connectID = 0;


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
	const char 	*cmd;			///> at command sentence
	cmd_flgs_t 	cmd_flags;		///> flags for command implementation (see @code flags enum)
	char 		*cmd_rsp;		///> expected command response
	uint32_t 	rsp_timeout;	///> response timeout [ms]
} bc66_at_cmd_t;

//*****************************************************************************
// Define AT commands list: order must be equal to commands definition enum bc66_cmd_list_t
const bc66_at_cmd_t bc66_cmds_list[] = {
/* 1- AT command */
	{
		.cmd = "\0",
		.cmd_flags = EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 2- Product Information Query Commands */
	{
		.cmd = "I",
		.cmd_flags = EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 3- UART function commands */
	{
		.cmd = "E",
		.cmd_flags = EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 4- Network State Query Commands */
	{
		.cmd = "+CEREG",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+CESQ",
		.cmd_flags = TEST | EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+CGATT",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 85000,
	},
	{
		.cmd = "+CGPADDR",
		.cmd_flags = TEST | READ | WRITE | EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 5- PDN and APN Commands */
	{
		.cmd = "+QCGDEFCONT",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 6- Other Network Commands */

/* 7- USIM Related Commands */
	{
		.cmd = "+CIMI",
		.cmd_flags = TEST | EXE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

	{
		.cmd = "+CPIN",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 5000,
	},

/* 9- Platform Related Commands */ 

/* 8- Power Consumption Commands */ 
	{
		.cmd = "+CPSMS",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+QNBIOTEVENT",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 10- Time-related Commands */

/* 11- Other Related Commands */ 
	{
		.cmd = "+QMTCFG",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+QMTOPEN",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 75000,
	},
	{
		.cmd = "AT+QMTCLOSE",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+QMTCONN",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 0,	/* <pkt_timeout> (default 10 s), determined by network */
	},
	{
		.cmd = "+QMTDISC",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},
	{
		.cmd = "+QMTSUB",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 0,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
								(default 40 s), determined by network */
	},
	{
		.cmd = "+QMTUNS",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 0,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
								(default 40 s), determined by network */
	},
	{
		.cmd = "+QMTPUB",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 0,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
								(default 40 s), determined by network */
	},
};

//*****************************************************************************

static void _bc66_rx_buffer_flush( void )
{
	memset(rx_buffer,0,sizeof(rx_buffer));
}

//*****************************************************************************
static void _bc66_tx_buffer_flush( void )
{
	memset(rx_buffer,0,sizeof(rx_buffer));
}

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
		_bc66_tx_buffer_flush();
		_bc66_rx_buffer_flush();
		
		// set local object pointer
		bc66 = bc66_obj;
		// call to uart (hal) initialize function
		bc66->func_init_ptr();

		// wait
		bc66_power_off();
		bc66_reset();
		bc66->func_delay(100);

		// module power on
		bc66_power_on();

		bc66->func_delay(250);

		// reset module
//		bc66_reset();
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
 * @brief 
 * Find an expected answer and remove them if from rx buffer it is found.
 * 
 * @param str	: RX responses buffer 
 * @param rsp	: extected at response or NULL
 * 
 * @return 
 * Extected AT response or NULL.
 */
static char * _bc66_at_parser(char * str, const char * rsp)
{
	static char rsp_found[MAX_RSP_SIZE];
	char * idx_start, * idx_stop;

	if( (idx_start = strstr( str, rsp )) ) {
		if( (idx_stop = strstr( idx_start, RSP_END_OF_LINE )) ) {
			// add end of line chars 
			idx_stop += strlen(RSP_END_OF_LINE);
			uint16_t length = (idx_stop - idx_start);
			
			if( length < MAX_RSP_SIZE ) { 
				// init response buffer 
				memset(rsp_found,0,sizeof(rsp_found));
				// get response - copy to new buffer
				strncpy(rsp_found, idx_start, length );
				// remove response from rx buffer
				strcpy(idx_start, idx_stop );
				// return expected response 
				return rsp_found;
			}
		}
	}

	return NULL;
}

//*****************************************************************************
/**
 * @brief 
 * Wait Modem response.
 * 
 * @param rsp	: response buffer pointer 
 * @param timeout: response wait time [ms]
 * 
 * @return 
 * Response text or TIMEOUT text. 
 */
static char * _bc66_find_at_response( const char * rsp, uint32_t timeout )
{
	uint8_t uart_char[2] = {0,0}; 
	char * rsp_ptr;
	while( timeout ) {
		bc66->func_delay(1);
		bc66->func_r_bytes_ptr( uart_char );
		strcat((char*)rx_buffer,(char*)uart_char);
		if( (rsp_ptr = _bc66_at_parser((char *)rx_buffer, rsp)) ) {
			return rsp_ptr;
		}
		timeout --;
	}

	return "TIMEOUT\r\n";
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
 * - Command aswer text
 * - OK
 * - ERROR
 * - TIMEOUT
 */
char * bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *exp_rsp, const char * arg_fmt, ...)
{
	// check if object was initialized
	if( bc66 == NULL ) { 
		return NULL;
	}

	// flush rx buffer to store all responses 
	_bc66_rx_buffer_flush();

	// send command AT 
	switch( cmd_type )
	{
		case BC66_CMD_TEST:
			if( bc66_cmds_list[cmd_lst].cmd_flags & TEST ) {
				sprintf((char*)tx_buffer,"AT%s=?",bc66_cmds_list[cmd_lst].cmd);
			}
			break;

		case BC66_CMD_READ:
			if( bc66_cmds_list[cmd_lst].cmd_flags & READ ) {
				sprintf((char*)tx_buffer,"AT%s?",bc66_cmds_list[cmd_lst].cmd);
			}
			break;

		case BC66_CMD_WRITE:
			if( bc66_cmds_list[cmd_lst].cmd_flags & WRITE ) {
				sprintf((char*)tx_buffer,"AT%s=",bc66_cmds_list[cmd_lst].cmd);
				if( arg_fmt ) { 
					// declare list of arguments 
					va_list args;
					// initialize the list 
					va_start ( args, arg_fmt );
					vsprintf((char*)&tx_buffer[strlen((const char *)tx_buffer)], (const char *)arg_fmt, args);
					// clear the list 
					va_end(args);
				}
			}
			break;

		case BC66_CMD_EXE:
			if( bc66_cmds_list[cmd_lst].cmd_flags & EXE ) {
				sprintf((char*)tx_buffer,"AT%s",bc66_cmds_list[cmd_lst].cmd);
			}
			break;

		default:	
			return "Command type not recognized";
			break;
	}

	// send command
	strcat((char*)tx_buffer,CMD_END_LINE);
	bc66->func_w_bytes_ptr((uint8_t*)tx_buffer,strlen((const char*)tx_buffer));

	// check expected response - +ATCMD: ... 
	if( exp_rsp ) {
		return _bc66_find_at_response((const char*)exp_rsp, bc66_cmds_list[cmd_lst].rsp_timeout);
	}

	// check command response - <CR><LF>OK<CR><LF> normally
	if( bc66_cmds_list[cmd_lst].cmd_rsp ) {
		return _bc66_find_at_response((const char*)bc66_cmds_list[cmd_lst].cmd_rsp, bc66_cmds_list[cmd_lst].rsp_timeout);
	}

	return NULL;
}

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
char * bc66_get_at_response( char * rsp )
{
	return _bc66_at_parser((char*)rx_buffer, (const char *)rsp);
}

//*****************************************************************************
/**
 * @brief
 * Reset the module when PIN is low.
 */
void bc66_reset( void )
{
	if( bc66 ) {
		bc66->control_lines.MDM_RESET_N(1);
		bc66->func_delay(100);
		bc66->control_lines.MDM_RESET_N(0);
	}
}

//*****************************************************************************
/**
 * @brief
 * Pull down PWRKEY to turn on the module
 */
void bc66_power_on()
{
	if( bc66 ) {
		bc66->control_lines.MDM_PWRKEY_N(1);
		bc66->func_delay(500);
		bc66->control_lines.MDM_PWRKEY_N(0);
	}
}

//*****************************************************************************
/**
 * @brief
 * Pull up PWRKEY to turn off the module
 */
void bc66_power_off()
{
	if( bc66 ) {
		bc66->control_lines.MDM_PWRKEY_N(0);
	}
}

