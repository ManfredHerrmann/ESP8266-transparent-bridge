/*
 * File	: user_main.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.	If not, see <http://www.gnu.org/licenses/>.
 */
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "driver/uart.h"
#include "task.h"
#include "user_interface.h"
#include "gpio.h"

#include "server.h"
#include "config.h"
#include "flash_param.h"

os_event_t		recvTaskQueue[recvTaskQueueLen];
extern  serverConnData connData[MAX_CONN];

#define MAX_UARTBUFFER (MAX_TXBUFFER/4)
static uint8 uartbuffer[MAX_UARTBUFFER];

static void ICACHE_FLASH_ATTR recvTask(os_event_t *events)
{
	uint8_t i;
	while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
	{
		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		uint16 length = 0;
		while ((READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) && (length<MAX_UARTBUFFER))
			uartbuffer[length++] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		for (i = 0; i < MAX_CONN; ++i) {
			if (connData[i].conn) {
				if (connData[i].skip_chars == 0) {
					espbuffsent(&connData[i], uartbuffer, length);
				} else if (connData[i].skip_chars >= length) {
					connData[i].skip_chars -= length;
				} else { // connData[i].skip_chars < length
					espbuffsent(&connData[i], uartbuffer+connData[i].skip_chars, length-connData[i].skip_chars);
					connData[i].skip_chars = 0;
				}
			}
		}
	}

	if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
	{
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	}
	else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST))
	{
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}
	ETS_UART_INTR_ENABLE();
}


// UartDev is defined and initialized in rom code.
extern UartDevice    UartDev;

void user_init(void)
{

	uint8_t i;
	//wifi_set_opmode(3); //STA+AP

	#ifdef CONFIG_DYNAMIC
		flash_param_t *flash_param;
		flash_param_init();
		flash_param = flash_param_get();
		UartDev.data_bits = GETUART_DATABITS(flash_param->uartconf0);
		UartDev.parity = GETUART_PARITYMODE(flash_param->uartconf0);
		UartDev.stop_bits = GETUART_STOPBITS(flash_param->uartconf0);
		uart_init(flash_param->baud, BIT_RATE_115200);
	#else
		UartDev.data_bits = EIGHT_BITS;
		UartDev.parity = NONE_BITS;
		UartDev.stop_bits = ONE_STOP_BIT;
		uart_init(BIT_RATE_115200, BIT_RATE_115200);
	#endif
	os_printf("size flash_param_t %d\n", sizeof(flash_param_t));

#ifdef CONFIG_RESET
	//set GPIO0 to output mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	//disable pulldown
	PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO0_U);
	//enable pull up R
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

	//set GPIO2 to output mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	//disable pulldown
	PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO2_U);
	//enable pull up R
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
#endif


	#ifdef CONFIG_STATIC
		// refresh wifi config
		config_execute();
	#endif

	#ifdef CONFIG_DYNAMIC
		serverInit(flash_param->port);
	#else
		serverInit(23);
	#endif

	for (i = 0; i < 16; ++i)
		uart0_sendStr("\r\n");

	wifi_set_sleep_type(NONE_SLEEP_T);

	REG_SET_BIT(0x3ff00014, BIT(0));
	os_update_cpu_frequency(160);

	system_os_task(recvTask, recvTaskPrio, recvTaskQueue, recvTaskQueueLen);
}
