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
 * @file    example.h
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @brief
 *
 *
 * ---------------------------------------------------------------------------------------------
 *
 * @date    17 de marzo de 2021, 14:59
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

#include "src/bc66_drv.h"


static int write_bytes( uint8_t * b, uint8_t size )
{
	return printf("%s",b);
}

static int read_bytes( uint8_t * b, uint8_t size )
{
	return 0;
}

int main(int argc, char const *argv[])
{
	printf("BC66 use demonstration started");
	static bc66_obj_t myBC66 = {
		.func_init_ptr = NULL,
		.func_w_bytes_ptr = &write_bytes,
		.func_r_bytes_ptr = &read_bytes,
		.control_lines.MDM_PSM_EINT_N = 0,
		.control_lines.MDM_PWRKEY_N = 0,
		.control_lines.MDM_RESET_N = 0,
		.control_lines.MDM_RI = 0
	};
	bc66_init(&myBC66);

	printf("BC66 RESET \n");
	bc66_reset();
	printf("Send AT Command\n");
	bc66_send_at_command(BC66_CMD_EXE,bc66_cmd_list_at,NULL,0);
	return 0;
}
