/*******************************************************************************
 * This file provides address values that exist in the DE1-SoC Computer
 ******************************************************************************/

#ifndef __SYSTEM_INFO__
#define __SYSTEM_INFO__

#define BOARD					"DE10-Lite"

/* Memory */
#define SDRAM_BASE				0x00000000
#define SDRAM_END				0x03FFFFFF
#define FPGA_ONCHIP_BASE		0x08000000
#define FPGA_ONCHIP_END			0x0800FFFF
#define FPGA_CHAR_BASE			0x09000000
#define FPGA_CHAR_END			0x09001FFF

/* Devices */
#define LEDR_BASE				0xFF200000
#define HEX3_HEX0_BASE			0xFF200020
#define HEX5_HEX4_BASE			0xFF200030
#define SW_BASE					0xFF200040
#define KEY_BASE				0xFF200050
#define JP1_BASE				0xFF200060
#define ARDUINO_GPIO			0xFF200100
#define ARDUINO_RESET_N			0xFF200110
#define JTAG_UART_BASE			0xFF201000
#define TIMER_BASE				0xFF202000
#define TIMER_2_BASE			0xFF202020
#define PIXEL_BUF_CTRL_BASE		0xFF203020
#define CHAR_BUF_CTRL_BASE		0xFF203030

#endif
