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
 * 
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

#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
/**
 * 
 */
typedef struct {
	const void (*func_init_ptr)(); 					///> uart initialize function pointer
	int (*func_w_bytes_ptr)(uint8_t * txc, uint8_t size); ///> fputc function pointer
	int (*func_r_bytes_ptr)(uint8_t * rxc, uint8_t size); ///> fgetc function pointer
	struct  {
		void (*MDM_PSM_EINT_N)();				///> delay function pointer
		void (*MDM_PWRKEY_N)();					///> modem power key function pointer
		void (*MDM_RESET_N)();					///> modem reset function pointer
		void (*MDM_RI)();						///> modem ring interrupt function pointer
	}control_lines;
} bc66_obj_t ;

//*****************************************************************************
typedef enum { 
	BC66_CMD_TEST , 
	BC66_CMD_READ,
	BC66_CMD_WRITE,
	BC66_CMD_EXE
} bc66_cmd_type_t ;

typedef enum { 
	bc66_cmd_list_at,
	bc66_cmd_list_ati,
	bc66_cmd_list_size 
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
 * Function to send at command sentence to bc66 module through an external function communication. 
 * 
 * @param cmd_type : BC66_CMD_TEST, BC66_CMD_READ, BC66_CMD_WRITE or BC66_CMD_EXE type.
 * @param cmd_lst : command to send (see command list). 
 * @param rsp : pointer to expected response text. 
 * @param arg_fmt : arguments format (like printf function) and must be sended all arguments too.
 * 
 * @return 
 * Module response text.
 */
char * bc66_send_at_command(bc66_cmd_type_t cmd_type, const bc66_cmd_list_t cmd_lst, const char *rsp, const char * arg_fmt, ...);

//*****************************************************************************

void bc66_reset( void );
