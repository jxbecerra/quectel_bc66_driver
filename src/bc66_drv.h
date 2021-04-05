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
	void (*func_init_ptr)(); 								///< uart initialize function pointer
	void (*func_delay)(uint32_t t);							///< delay function pointer
	int (*func_w_bytes_ptr)(uint8_t * txc, uint16_t len); 	///< write bytes function pointer
	int (*func_r_bytes_ptr)(uint8_t * rxc, uint16_t size ); ///< read bytes function pointer
	struct  {
		void (*MDM_PSM_EINT_N)(size_t pin_value);			///< Function pointer to interface: to handle PSM_EINT pin. 
		void (*MDM_PWRKEY_N)(size_t pin_value);				///< Function pointer to interface: to handle PWRKEY pin. 
		void (*MDM_RESET_N)(size_t pin_value);				///< Function pointer to interface: to handle RESET pin.
		void (*MDM_RI)();									///< Function pointer to interface: to handle ring interrupt pin.
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
	bc66_cmd_list_QBAND,			///< Get and Set Mobile Operation Band
	/* 7- USIM Related Commands */
	bc66_cmd_list_CIMI,				///< Request International Mobile Subscriber Identity
	bc66_cmd_list_CPIN,				///< Enter PIN
	/* 8- Power Consumption Commands */ 
	bc66_cmd_list_CPSMS,			///< Power Saving Mode Setting
	bc66_cmd_list_QNBIOTEVENT,		///< Enable/Disable NB-IoT Related Event Report
	bc66_cmd_list_QSCLK,			///< Configure Sleep Mode
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
/// bc66 library api return 
typedef enum 
{
	bc66_ret_success,					///< Modem data process/response successful.
	bc66_ret_timeout,					///< Response timeout.
	bc66_ret_error,						///< Modem response with error message. 
	bc66_ret_fail,						///< Modem response with fail message.
	bc66_ret_out_of_range,				///< At least some argument is out of range
	bc66_ret_not_init,
	bc66_ret_no_ip, 					///< Device has not IP ADDRESS
	bc66_ret_packet_retransmission,		///< Packet retransmission
	bc66_ret_packet_fail, 				///< Failed to send packet
	bc66_ret_err_protocol,				///< Connection Refused: Unacceptable Protocol Version
	bc66_ret_id_rejected,				///< Connection Refused: Identifier Rejected
	bc66_ret_no_cmd_implemented			///< RSP_NO_CMD_IMPEMENTED
} bc66_ret_t ;

//*****************************************************************************
/// Enumeration to specify the type of packet data protocol. 
typedef enum {
	pdp_type_ip, 					///< Internet Protocol (IETF STD 5).
	pdp_type_ipv6,					///< Internet Protocol version 6 (IETF RFC 2460).
	pdp_type_ipv4v6,				///< Dual IP stack (see 3GPP TS 24.301).
	pdp_type_non_ip					///< Transfer of Non-IP data to external packet network (see 3GPP TS 24.301).
} pdp_type_t ;

/// Struct to store IP ADDRESS. 
typedef struct {
	uint8_t	a1;
	uint8_t	a2;
	uint8_t	a3;
	uint8_t	a4;
} bc66_ip_add_t ;

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
bc66_ret_t bc66_init(bc66_obj_t *bc66_obj);

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
 * See \p bc66_ret_t return codes. 
 */
bc66_ret_t bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *exp_rsp, const char * arg_fmt, ...);

//*****************************************************************************
/**
 * @brief
 * Reset the module via Hardware PIN.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_hw_reset( void );

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

//*****************************************************************************
/**
 * @brief 
 * Function to get last modem response. 
 * If send a new AT command, the buffer which contain the last response will be erased.
 * 
 * @return 
 * Pointer to RX buffer with last response. 
 */
char * bc66_get_last_response( void );

//*****************************************************************************
/**
 * @brief 
 * Send AT command to sync baud rate. 
 * 
 * @return 
 * See \p bc66_ret_t return codes. 
 */
bool bc66_send_cmd_AT( void );

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
bc66_ret_t bc66_set_echo_mode( bool echo );

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
bc66_ret_t bc66_set_eps( unsigned int set );

//*****************************************************************************
/**
 * @brief 
 * Power Saving Mode Setting. 
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
bc66_ret_t bc66_set_power_saving_mode( int mode );

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
bc66_ret_t bc66_get_ipv4_address(bc66_ip_add_t * ip );

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
bc66_ret_t bc66_set_psd_conn(pdp_type_t pdp_type, const char * apn, const char * user, const char * pass );

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
bc66_ret_t bc66_set_mobile_bands( int band_number, ... );

//*****************************************************************************
/**
 * @brief 
 * Enter PIN AT command.
 * Return bc66_ret_success if Modem is READY.
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_is_ready( void );

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
bc66_ret_t bc66_set_nbiot_event_report(bool enable, bool event );

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
bc66_ret_t bc66_set_sleep_mode( uint8_t mode );

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
bc66_ret_t bc66_set_mqtt_parameters( uint16_t keepalive, bool dataformat, bool session , bool version );

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
bc66_ret_t bc66_open_net_mqtt_client(const char * server_ip, uint16_t server_port );

//*****************************************************************************
/**
 * @brief 
 * Close a Network for MQTT Client. 
 * 
 * @return 
 * See \p bc66_ret_t return codes.
 */
bc66_ret_t bc66_close_net_mqtt_client( void );

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
bc66_ret_t bc66_connect_mqtt_client(const char * client_id, const char * user, const char * pass );

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
bc66_ret_t bc66_disconn_mqtt_client( void );

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
bc66_ret_t bc66_publish_msg_mqtt( const char * topic, const char * msg, int qos );

