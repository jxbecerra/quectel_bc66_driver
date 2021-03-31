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
 * @file    bc66_drv.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "bc66_drv.h"

// commands defines 
#define CMD_END_LINE			"\r\n"				///< End of line command chars.

// responses defines 
#define RSP_OK 					"\r\nOK\r\n"		///< Ok response.
#define RSP_ERROR 				"\r\nERROR\r\n"		///< Error response.
#define RSP_END_OF_LINE			"\r\n"				///< End of line response chars.
#define RSP_TIMEOUT 			"BC66_TIMEOUT\r\n"	///< Answer when a timeout is occurred.
#define RSP_NO_CMD_IMPEMENTED 	"BC66_NO_CMD\r\n"	///< The command is not implemented.

#define MAX_RSP_SIZE	64		///< Max AT response size

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
 * - Write Command AT+<x>=
 * - Execution Command AT+<x>
 */

//*****************************************************************************
// global working buffers
static uint8_t tx_buffer[512];
static uint8_t rx_buffer[512];
static uint8_t rx_last_response[256];

// pointer to once object instance 
static bc66_obj_t *bc66 = NULL;

/// Buffer to save last valid answer
static char rsp_found[MAX_RSP_SIZE];

//*****************************************************************************
/// Command possibilities indicator flags. 
typedef enum { 
	TEST	= 0x1,				///< Command has test posibility
	READ 	= 0x2,				///< Command has read posibility
	WRITE 	= 0x4,				///< Command has write posibility
	EXE 	= 0x8				///< Command has execute posibility
} cmd_flgs_t ;

/// BC66 Command struct 
typedef const struct
{
	const char 	*cmd;			///< at command sentence
	cmd_flgs_t 	cmd_flags;		///< flags for command implementation (see @code flags enum)
	char 		*cmd_rsp;		///< expected command response
	uint32_t 	rsp_timeout;	///< response timeout [ms]
} bc66_at_cmd_t;

//*****************************************************************************
/// Define AT commands list: order must be equal to commands definition enum bc66_cmd_list_t
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
	{
		.cmd = "+QBAND",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

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
	{
		.cmd = "+QSCLK",
		.cmd_flags = TEST | READ | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 300,
	},

/* 9- Platform Related Commands */ 

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
		.rsp_timeout = 10000,	/* <pkt_timeout> (default 10 s), determined by network */
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
		.rsp_timeout = 40000,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
								(default 40 s), determined by network */
	},
	{
		.cmd = "+QMTUNS",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 40000,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
								(default 40 s), determined by network */
	},
	{
		.cmd = "+QMTPUB",
		.cmd_flags = TEST | WRITE,
		.cmd_rsp = RSP_OK,
		.rsp_timeout = 40000,	/* 	<pkt_timeout> + <pkt_timeout> ×<retry_times>
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
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_init(bc66_obj_t *bc66_obj)
{
	bc66_ret_t ret_code = bc66_ret_error;
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
		
		bc66->func_delay(250);
		ret_code = bc66_hw_reset();
		bc66->func_delay(250);

		// module power on
		bc66_power_on();

		bc66->func_delay(250);

		// reset module
//		bc66_hw_reset();
		ret_code = bc66_ret_success;
	}
	return ret_code;
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
	char * idx_start, * idx_stop;

	if( (idx_start = strstr( str, rsp )) ) {
		if( (idx_stop = strstr( idx_start+1, RSP_END_OF_LINE )) ) {
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
 * See \p bc66_ret_t return codes.
 */
static bc66_ret_t _bc66_find_at_response( const char * rsp, uint32_t timeout )
{
	uint8_t rx_temp_buffer[64]; 
	char * rsp_ptr;
	while( timeout ) {
		// printf("timeout: %u\n", timeout);
		bc66->func_delay(1);
		// get new received chars 
		memset(rx_temp_buffer,0,sizeof(rx_temp_buffer));
		bc66->func_r_bytes_ptr( rx_temp_buffer, sizeof(rx_temp_buffer) );
		// add new chars to RX buffer 
		strcat((char*)rx_buffer,(char*)rx_temp_buffer);
		if( (rsp_ptr = _bc66_at_parser((char *)rx_buffer, rsp)) ) {
			strcpy( (char*)rx_last_response, rsp_ptr );
			return bc66_ret_success;
		}
		timeout --;
	}

	return bc66_ret_timeout;
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
 * See \p bc66_ret_t return codes. 
 */
bc66_ret_t bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *exp_rsp, const char * arg_fmt, ...)
{
	// check if object was initialized
	if( bc66 == NULL ) { 
		return bc66_ret_not_init;
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

		default:	
			return bc66_ret_no_cmd_implemented;
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

	return bc66_ret_success;
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
 * Reset the module via Hardware PIN.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_hw_reset( void )
{
	if( bc66 ) {
		bc66->control_lines.MDM_RESET_N(1);
		bc66->func_delay(100);
		bc66->control_lines.MDM_RESET_N(0);
		bc66->func_delay(100);

		// return _bc66_find_at_response("Leaving", 5000 );
	}
	return bc66_ret_error;
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

//*****************************************************************************
/**
 * @brief 
 * Function to get last modem response. 
 * If send a new AT command, the buffer which contain the last response will be erased.
 * 
 * @return 
 * Pointer to RX buffer with last response. 
 */
char * bc66_get_last_response( void )
{
	return (char*)rx_last_response;
}

//*****************************************************************************
/**
 * @brief 
 * Send AT command to sync baud rate. 
 * 
 * @return 
 * See \p bc66_ret_t return codes. 
 */
bool bc66_send_cmd_AT( void )
{
	return bc66_send_at_command( BC66_CMD_EXE,bc66_cmd_list_AT,NULL,NULL);
}

//*****************************************************************************
/**
 * @brief 
 * Set Command Echo Mode. 
 * 
 * This Execution Command determines whether or not the UE echoes characters 
 * received from external MCU during command state. 
 * 
 * The command takes effect immediately. Remain valid after deep-sleep wakeup. 
 * The configuration will be saved to NVRAM (should execute AT&W after this command is issued).
 * 
 * @param echo 
 * - false: Echo mode OFF
 * - true: Echo mode ON
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_echo_mode( bool echo )
{ 
	return bc66_send_at_command(BC66_CMD_EXE,bc66_cmd_list_ATE,NULL,"%c", '0' + (int)echo );
}

//*****************************************************************************
/**
 * @brief 
 * EPS Network Registration Status. 
 * Configures the different unsolicited result codes for EPS 
 * Network Registration Status.
 * 
 * @param net : Disable or enable network registration URC. 
 * - 0 Disable network registration URC 
 * - 1 Enable network registration URC: +CEREG: <stat> 
 * - 2 Enable network registration and location information URC: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]] 
 * - 3 Enable network registration, location information and EMM cause value information URC: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]] 
 * - 4 For a UE that requests PSM, enable network registration and location information URC: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<Active-Time>],[<Periodic-TAU>]]]]  
 * - 5 For a UE that requests PSM, enable network registration, location information and EMM cause value information URC: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_eps( unsigned int set )
{ 
	if( set > 5 ) { 
		return bc66_ret_out_of_range;
	}

	return bc66_send_at_command( BC66_CMD_WRITE, bc66_cmd_list_CEREG, NULL, "%u", set );
}

//*****************************************************************************
/**
 * @brief 
 * Power Saving Mode Setting (PSM). 
 * 
 * @param mode 
 * Integer type. Disable or enable the use of PSM in the UE 
 * - 0 Disable the use of PSM 
 * - 1 Enable the use of PSM 
 * - 2 Disable the use of PSM and discard all parameters for PSM or, if available, reset to the default values.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_power_saving_mode( int mode )
{
	if( (0 <= mode) && (mode <= 2) ) {
		return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_CPSMS,NULL,"%c", '0' + (int)mode );
	} else { 
		return bc66_ret_out_of_range;
	}
}

//*****************************************************************************
/**
 * @brief 
 * This function returns the IP address of the device. Show PDP Addresses.
 * 
 * @param ip : pointer to struct variable to return IP ADDRESS.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */ 
bc66_ret_t bc66_get_ipv4_address(bc66_ip_add_t * ip )
{
	bc66_ret_t ret_code; 
	const char cmd_rsp[] = "+CGPADDR: 1,";
	char * rsp;
	// send command 
	ret_code = bc66_send_at_command( BC66_CMD_WRITE, bc66_cmd_list_CGPADDR, cmd_rsp, "1" );
	if( ret_code == bc66_ret_success ) { 
		// get ip address in text format 
		rsp = bc66_get_last_response();
		if( (rsp = strstr(rsp,cmd_rsp)) ) {
			if( (rsp = strchr(rsp, ',')) ) {
				rsp++;
				ip->a4 = atoi(rsp);
				if( (rsp = strchr(rsp, '.')) ) { 
					rsp++;
					ip->a3 = atoi(rsp);
					if( (rsp = strchr(rsp, '.')) ) { 
						rsp++;
						ip->a2 = atoi(rsp);
						if( (rsp = strchr(rsp, '.')) ) {
							rsp++;
							ip->a1 = atoi(rsp);
							return bc66_ret_success;
						}
					}
				}
			}
		}
	}
	return bc66_ret_no_ip;
}

//*****************************************************************************
/**
 * @brief 
 * Set Default PSD Connection
 * 
 * This command sets the PSD connection settings for PDN connection on power-up. 
 * When attaching to the NB-IoT network on power-on, a PDN connection setup must be performed. 
 * In order to allow this to happen, PDN connection settings must be stored in NVRAM, 
 * thus making it to be used by the modem during the attach procedure.
 * 
 * @param pdp_type 	: Specify the type of packet data protocol. 
 * @param apn 		: A logical name that is used to select the GGSN or the external packet data network. The maximum configurable APN length is 99 bytes.
 * @param user		: The user name for accessing to the IP network. (Optional)
 * @param pass		: The password for accessing to the IP network. (Optional)
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_psd_conn(pdp_type_t pdp_type, const char * apn, const char * user, const char * pass )
{
	char pdp[256];
	switch( pdp_type ) 
	{
		case pdp_type_ip: 
			strcpy(pdp,"\"IP\"");
			break; 
		
		case pdp_type_ipv6: 
			strcpy(pdp,"\"IPV6\"");
			break; 
		
		case pdp_type_ipv4v6: 
			strcpy(pdp,"\"IPV4V6\"");
			break; 
		
		case pdp_type_non_ip: 
			strcpy(pdp,"\"Non-IP\"");
			break; 

		default: 
			return bc66_ret_out_of_range;
	}
	if( apn == NULL ) { 
		return bc66_ret_out_of_range;
	}

	strcat(pdp,",\"");
	strcat(pdp,apn);
	strcat(pdp,"\"");

	if( user ) { 
		strcat(pdp,",\"");
		strcat(pdp,user);
		strcat(pdp,"\"");
	}

	if( pass ) { 
		strcat(pdp,",\"");
		strcat(pdp,pass);
		strcat(pdp,"\"");
	}

	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QCGDEFCONT,NULL,"%s", pdp);
}

//*****************************************************************************
/**
 * @brief 
 * Set Mobile Operation Band.
 * 
 * @param band_numb : band quantity. 
 * - 0 all bands. 
 * - 1 to 16 Number of bands to be locked.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_mobile_bands( int band_number, ... )
{ 
	va_list bands;
	char all_bands[32];
	sprintf( all_bands, "%u", band_number );

	if( band_number ) {
		va_start( bands, band_number );
	}

	for( int n = 0 ; n < band_number ; n ++ ) {
		char tmp[8]; 
		int next_band = va_arg( bands, int );
		// add next selected band 
		sprintf(tmp, ",%u", next_band );
		// add band to command line 
		strcat(all_bands,tmp);
	}
	
	if( band_number ) {
		va_end( bands );
	}

	return bc66_send_at_command( BC66_CMD_WRITE, bc66_cmd_list_QBAND, NULL,"%s", all_bands );
}

//*****************************************************************************
/**
 * @brief 
 * Enter PIN AT command.
 * Return bc66_ret_success if Modem is READY.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_is_ready( void )
{ 
	return bc66_send_at_command(BC66_CMD_READ,bc66_cmd_list_CPIN,"+CPIN: READY",NULL);
}

//*****************************************************************************
/**
 * @brief 
 * Enable/Disable NB-IoT Related Event Report. 
 * 
 * @param enable : Enable/disable a specific event report. 
 * - 0 Disable the indication of the specific event 
 * - 1 Enable the indication of the specific event by URC +QNBIOTEVENT: <event_value>
 * @param event : The reported event. 
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_nbiot_event_report(bool enable, bool event )
{
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QNBIOTEVENT, NULL,"%u,%u", (int)enable, (int)event );
}

//*****************************************************************************
/**
 * @brief 
 * Configures the TE’s sleep modes.
 * 
 * @param mode : 
 * - 0 Disable sleep modes 
 * - 1 Enable light sleep and deep sleep, wakeup by PSM_EINT (falling edge)  
 * - 2 Enable light sleep only, wakeup by the Main UART 
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_sleep_mode( uint8_t mode )
{ 
	if( mode > 2 ) { 
		return bc66_ret_out_of_range;
	}
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QSCLK,NULL, "%c",'0' + (int)mode);
}

//*****************************************************************************
/**
 * @brief 
 * Used to configure optional parameters of MQTT
 * 
 * @param keepalive	: Configure the keep-alive time. The range is 0-3600. 
 * The default value is 120. Unit: second. It defines the maximum time interval 
 * between messages received from a client. 
 * If the server does not receive a message from the client within 1.5 times of
 * the keep-alive time period, it disconnects the client as if the client has sent a
 * DISCONNECT message. 0 The client is not disconnected
 * @param dataformat : The format of sent and received data. 
 * - 0 Text format 
 * - 1 Hex format
 * @param session : The session type.
 * - 0 The server must store the subscriptions of the client after it is disconnected.
 * - 1 The server must discard any previously maintained information about the
 * client and treat the connection as "clean".
 * @param version : The version of MQTT protocol. 
 * - 0 MQTT v3.1 
 * - 1 MQTT v3.1.1
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_set_mqtt_parameters( uint16_t keepalive, bool dataformat, bool session , bool version )
{
	bc66_ret_t ret_code;
	const uint8_t TCP_connectID = 0;

	if( keepalive > 3600 ) { 
		return bc66_ret_out_of_range;
	}
	ret_code = bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTCFG,NULL,"\"keepalive\",%u,%u",TCP_connectID, keepalive);
	if( ret_code == bc66_ret_success ) { 
		bc66->func_delay(500);
		ret_code = bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTCFG,NULL,"\"dataformat\",%u,%u,%u", TCP_connectID, dataformat, dataformat );
		if( ret_code == bc66_ret_success ) { 
			bc66->func_delay(500);
			ret_code = bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTCFG,NULL,"\"session\",%u,%u", TCP_connectID, session );
			if( ret_code == bc66_ret_success ) { 
				bc66->func_delay(500);
				return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTCFG,NULL,"\"version\",%u", (3 + (int)version) );
			}
		}
	}
	return ret_code;	
}

//*****************************************************************************
/**
 * @brief 
 * Open a Network for MQTT Client. 
 * 
 * @param server_ip 	: server ip (string)
 * @param server_port 	: server port (0 to 65535)
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_open_net_mqtt_client(const char * server_ip, uint16_t server_port )
{
	const uint8_t TCP_connectID = 0;
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTOPEN,"+QMTOPEN: 0,0","%u,\"%s\",%u", TCP_connectID, server_ip, server_port);
}

//*****************************************************************************
/**
 * @brief 
 * Connect a Client to MQTT Server. 
 * 
 * @param client_id : The client identifier. The max length is 128 bytes.
 * @param user :  User name of the client. It can be used for authentication. 
 * The max length is 256 bytes.
 * @param pass :  Password corresponding to the user name of the client. 
 * It can be used for authentication. The max length is 256 bytes.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_connect_mqtt_client(const char * client_id, const char * user, const char * pass )
{
	const uint8_t TCP_connectID = 0;
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTCONN,"+QMTCONN: 0,0,0","%u,\"%s\",\"%s\",\"%s\"",TCP_connectID,client_id,user,pass);
}

//*****************************************************************************
/**
 * @brief 
 * Disconnect a Client from MQTT Server. 
 * 
 * Used when a client requests a disconnection from MQTT server. 
 * A DISCONNECT message is sent from the client to the server to indicate that 
 * it is about to close its TCP/IP connection.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_disconn_mqtt_client( void )
{
	const uint8_t TCP_connectID = 0;
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTDISC,"+QMTDISC: 0,0","%u", TCP_connectID);
}

//*****************************************************************************
/**
 * @brief 
 * Publish Messages. 
 * Used to publish messages by a client to a server for distribution to interested subscribers.
 * 
 * @param topic	: Topic that the client wants to subscribe to or unsubscribe from. 
 * The maximum length is 255 bytes. 
 * @param msg 	: The message that needs to be published. The maximum length is 700 bytes. 
 * If in data mode (after > is responded), the maximum length is 1024 bytes
 * @param qos	: Integer type. The QoS level at which the client wants to publish the messages.
 * - 0 At most once
 * - 1 At least once
 * - 2 Exactly once
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_publish_msg_mqtt( const char * topic, const char * msg, int qos )
{
	const uint8_t TCP_connectID = 0;
	/* Message identifier of packet. The range is 0-65535. It will be 0 onlywhen <qos>=0. */
	int msgID = 0;			// The range is 0-65535.
	/* Whether or not the server will retain the message after it has been 
	delivered to the current subscribers.
	0: The server will not retain the message after it has been delivered to the
	current subscribers 
	1: The server will retain the message after it has been delivered to the current
	subscribers */
	int retain = 0;
	return bc66_send_at_command(BC66_CMD_WRITE,bc66_cmd_list_QMTPUB,"+QMTPUB: 0,0,0","%u,%u,%u,%u,\"%s\",\"%s\"",TCP_connectID,msgID,qos,retain,topic,msg);
}


