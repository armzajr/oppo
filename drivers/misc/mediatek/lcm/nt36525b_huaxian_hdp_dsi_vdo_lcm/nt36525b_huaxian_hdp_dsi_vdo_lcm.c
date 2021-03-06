/***********************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** File: - nt36525b_huaxian_hdp_dsi_vdo_lcm.c
** Description: source file for lcm nt36525b+huaxian in kernel stage
**
** Version: 1.0
** Date : 2018/11/20
** Author: Benzhong.Hou@mm.display.lcd,
**
** ------------------------------- Revision History: -------------------------------
**  	<author>		<data> 	   <version >	       <desc>
**  houbenzhong       2018/11/20     1.0     source file for lcm nt36525b+huaxian in kernel stage
**
****************************************************************/

#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include <linux/update_tpfw_notifier.h>
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define SET_LCM_VDD18_PIN(v)	(lcm_util.set_gpio_lcm_vddio_ctl((v)))
#define SET_LCM_VSP_PIN(v)	(lcm_util.set_gpio_lcd_enp_bias((v)))
#define SET_LCM_VSN_PIN(v)	(lcm_util.set_gpio_lcd_enn_bias((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1520)

#define LCM_PHYSICAL_WIDTH									(68000)
#define LCM_PHYSICAL_HEIGHT									(143000)

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

static struct LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 70, {} }
};

static int blmap_table[] = {
			36, 16,
			16, 22,
			17, 21,
			19, 20,
			19, 20,
			20, 17,
			22, 15,
			22, 14,
			24, 10,
			24, 8,
			26, 4,
			27, 0,
			29, 9,
			29, 9,
			30, 14,
			33, 25,
			34, 30,
			36, 44,
			37, 49,
			40, 65,
			40, 69,
			43, 88,
			46, 109,
			47, 112,
			50, 135,
			53, 161,
			53, 163,
			60, 220,
			60, 223,
			64, 257,
			63, 255,
			71, 334,
			71, 331,
			75, 375,
			80, 422,
			84, 473,
			89, 529,
			88, 518,
			99, 653,
			98, 640,
			103, 707,
			117, 878,
			115, 862,
			122, 947,
			128, 1039,
			135, 1138,
			132, 1102,
			149, 1355,
			157, 1478,
			166, 1611,
			163, 1563,
			183, 1900,
			180, 1844,
			203, 2232,
			199, 2169,
			209, 2344,
			236, 2821,
			232, 2742,
			243, 2958,
			255, 3188,
			268, 3433,
			282, 3705,
			317, 4400,
			176, 1555};

static struct LCM_setting_table init_setting_cmd[] = {
	//{ 0xFF, 0x03, {0x98, 0x81, 0x03} },
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xFF,0x01,{0x20}},
	{0xFB,0x01,{0x01}},
	{0x05,0x01,{0x69}},
	{0x07,0x01,{0x32}},
	{0x08,0x01,{0x9E}},
	{0x0E,0x01,{0x4B}},
	{0x0F,0x01,{0x37}},
	{0x30,0x01,{0x10}},
	{0x69,0x01,{0xA0}},
	{0x87,0x01,{0x02}},
	{0x94,0x01,{0x00}},
	{0x95,0x01,{0xF5}},
	{0x96,0x01,{0xF5}},
	{0xFF,0x01,{0x20}},
	{0xFB,0x01,{0x01}},
	{0xB0,0x10,{0x00,0x08,0x00,0x23,0x00,0x46,0x00,0x63,0x00,0x7E,0x00,0x96,0x00,0xAA,0x00,0xBD}},
	{0xB1,0x10,{0x00,0xCE,0x01,0x05,0x01,0x31,0x01,0x6F,0x01,0xA1,0x01,0xEA,0x02,0x24,0x02,0x26}},
	{0xB2,0x10,{0x02,0x5E,0x02,0xA0,0x02,0xC8,0x02,0xFD,0x03,0x21,0x03,0x4C,0x03,0x5A,0x03,0x6B}},
	{0xB3,0x0C,{0x03,0x7C,0x03,0x92,0x03,0xAC,0x03,0xC5,0x03,0xD9,0x03,0xDA}},
	{0xB4,0x10,{0x00,0x08,0x00,0x20,0x00,0x46,0x00,0x65,0x00,0x7F,0x00,0x96,0x00,0xAB,0x00,0xBE}},
	{0xB5,0x10,{0x00,0xCF,0x01,0x07,0x01,0x31,0x01,0x71,0x01,0xA0,0x01,0xE9,0x02,0x23,0x02,0x25}},
	{0xB6,0x10,{0x02,0x5D,0x02,0x9F,0x02,0xC7,0x02,0xFC,0x03,0x1F,0x03,0x49,0x03,0x56,0x03,0x65}},
	{0xB7,0x0C,{0x03,0x74,0x03,0x86,0x03,0x99,0x03,0xA6,0x03,0xB4,0x03,0xB6}},
	{0xB8,0x10,{0x00,0x08,0x00,0x21,0x00,0x4A,0x00,0x68,0x00,0x85,0x00,0x9E,0x00,0xB4,0x00,0xC5}},
	{0xB9,0x10,{0x00,0xD7,0x01,0x0F,0x01,0x39,0x01,0x78,0x01,0xA7,0x01,0xEF,0x02,0x28,0x02,0x2A}},
	{0xBA,0x10,{0x02,0x61,0x02,0xA2,0x02,0xC9,0x02,0xFF,0x03,0x22,0x03,0x4E,0x03,0x5C,0x03,0x6B}},
	{0xBB,0x0C,{0x03,0x7D,0x03,0x95,0x03,0xAD,0x03,0xCF,0x03,0xDA,0x03,0xDA}},
	{0xFF,0x01,{0x21}},
	{0xFB,0x01,{0x01}},
	{0xB0,0x10,{0x00,0x00,0x00,0x1B,0x00,0x3E,0x00,0x5B,0x00,0x76,0x00,0x8E,0x00,0xA2,0x00,0xB5}},
	{0xB1,0x10,{0x00,0xC6,0x00,0xFD,0x01,0x29,0x01,0x67,0x01,0x99,0x01,0xE2,0x02,0x1C,0x02,0x1E}},
	{0xB2,0x10,{0x02,0x56,0x02,0x98,0x02,0xC0,0x02,0xF5,0x03,0x19,0x03,0x44,0x03,0x52,0x03,0x63}},
	{0xB3,0x0C,{0x03,0x74,0x03,0x8A,0x03,0xA4,0x03,0xBD,0x03,0xD1,0x03,0xD2}},
	{0xB4,0x10,{0x00,0x00,0x00,0x18,0x00,0x3E,0x00,0x5D,0x00,0x77,0x00,0x8E,0x00,0xA3,0x00,0xB6}},
	{0xB5,0x10,{0x00,0xC7,0x00,0xFF,0x01,0x29,0x01,0x69,0x01,0x98,0x01,0xE1,0x02,0x1B,0x02,0x1D}},
	{0xB6,0x10,{0x02,0x55,0x02,0x97,0x02,0xBF,0x02,0xF4,0x03,0x17,0x03,0x41,0x03,0x4E,0x03,0x5D}},
	{0xB7,0x0C,{0x03,0x6C,0x03,0x7E,0x03,0x91,0x03,0x9E,0x03,0xAC,0x03,0xAE}},
	{0xB8,0x10,{0x00,0x00,0x00,0x19,0x00,0x42,0x00,0x60,0x00,0x7D,0x00,0x96,0x00,0xAC,0x00,0xBD}},
	{0xB9,0x10,{0x00,0xCF,0x01,0x07,0x01,0x31,0x01,0x70,0x01,0x9F,0x01,0xE7,0x02,0x20,0x02,0x22}},
	{0xBA,0x10,{0x02,0x59,0x02,0x9A,0x02,0xC1,0x02,0xF7,0x03,0x1A,0x03,0x46,0x03,0x54,0x03,0x63}},
	{0xBB,0x0C,{0x03,0x75,0x03,0x8D,0x03,0xA5,0x03,0xC7,0x03,0xD2,0x03,0xD2}},
	{0xFF,0x01,{0x23}},
	{0xFB,0x01,{0x01}},
	{0x00,0x01,{0x60}},
	{0x05,0x01,{0x24}},
	{0x07,0x01,{0x00}},
	{0x12,0x01,{0xAB}},
	{0x15,0x01,{0xF5}},
	{0x16,0x01,{0x0B}},
	{0x45,0x01,{0xF0}},
	{0x46,0x01,{0xED}},
	{0x47,0x01,{0xEA}},
	{0x48,0x01,{0xE7}},
	{0x49,0x01,{0xE4}},
	{0x4A,0x01,{0xE1}},
	{0x4B,0x01,{0xDE}},
	{0x4C,0x01,{0xDB}},
	{0x4D,0x01,{0xD8}},
	{0x4E,0x01,{0xD5}},
	{0x4F,0x01,{0xD2}},
	{0x50,0x01,{0xCF}},
	{0x51,0x01,{0xCC}},
	{0x52,0x01,{0xC9}},
	{0x53,0x01,{0xC6}},
	{0x54,0x01,{0xC3}},
	{0xFF,0x01,{0x24}},
	{0xFB,0x01,{0x01}},
	{0x00,0x01,{0x23}},
	{0x01,0x01,{0x00}},
	{0x02,0x01,{0x20}},
	{0x03,0x01,{0x00}},
	{0x04,0x01,{0x23}},
	{0x05,0x01,{0x1E}},
	{0x06,0x01,{0x00}},
	{0x07,0x01,{0x15}},
	{0x08,0x01,{0x14}},
	{0x09,0x01,{0x0C}},
	{0x0A,0x01,{0x0D}},
	{0x0B,0x01,{0x04}},
	{0x0C,0x01,{0x21}},
	{0x0D,0x01,{0x00}},
	{0x0E,0x01,{0x00}},
	{0x0F,0x01,{0x00}},
	{0x10,0x01,{0x00}},
	{0x11,0x01,{0x00}},
	{0x12,0x01,{0x00}},
	{0x13,0x01,{0x00}},
	{0x14,0x01,{0x01}},
	{0x15,0x01,{0x01}},
	{0x16,0x01,{0x23}},
	{0x17,0x01,{0x00}},
	{0x18,0x01,{0x20}},
	{0x19,0x01,{0x00}},
	{0x1A,0x01,{0x23}},
	{0x1B,0x01,{0x1E}},
	{0x1C,0x01,{0x00}},
	{0x1D,0x01,{0x0D}},
	{0x1E,0x01,{0x0C}},
	{0x1F,0x01,{0x15}},
	{0x20,0x01,{0x14}},
	{0x21,0x01,{0x04}},
	{0x22,0x01,{0x21}},
	{0x23,0x01,{0x00}},
	{0x24,0x01,{0x00}},
	{0x25,0x01,{0x00}},
	{0x26,0x01,{0x00}},
	{0x27,0x01,{0x00}},
	{0x28,0x01,{0x00}},
	{0x29,0x01,{0x00}},
	{0x2A,0x01,{0x01}},
	{0x2B,0x01,{0x01}},
	{0x2F,0x01,{0x03}},
	{0x33,0x01,{0x04}},
	{0x37,0x01,{0x11}},
	{0x39,0x01,{0x00}},
	{0x3A,0x01,{0x01}},
	{0x3B,0x01,{0xAD}},
	{0x3D,0x01,{0x91}},
	{0x3F,0x01,{0x0A}},
	{0x43,0x01,{0x0A}},
	{0x47,0x01,{0x44}},
	{0x4A,0x01,{0x05}},
	{0x4B,0x01,{0x50}},
	{0x4C,0x01,{0x51}},
	{0x4D,0x01,{0x12}},
	{0x4E,0x01,{0x34}},
	{0x51,0x01,{0x43}},
	{0x52,0x01,{0x21}},
	{0x55,0x01,{0x04}},
	{0x56,0x01,{0x12}},
	{0x58,0x01,{0x20}},
	{0x59,0x01,{0x00}},
	{0x5A,0x01,{0x08}},
	{0x5B,0x01,{0xA6}},
	{0x5C,0x01,{0x00}},
	{0x5D,0x01,{0x0A}},
	{0x5E,0x01,{0x04}},
	{0x60,0x01,{0x80}},
	{0x61,0x01,{0x7C}},
	{0x64,0x01,{0x10}},
	{0x68,0x01,{0x12}},
	{0x69,0x01,{0x34}},
	{0x6A,0x01,{0x43}},
	{0x6B,0x01,{0x21}},
	{0x6C,0x01,{0x04}},
	{0x6D,0x01,{0x22}},
	{0x6E,0x01,{0x20}},
	{0x6F,0x01,{0x00}},
	{0x70,0x01,{0xB0}},
	{0x71,0x01,{0xA5}},
	{0x72,0x01,{0x00}},
	{0x73,0x01,{0x0A}},
	{0x74,0x01,{0x04}},
	{0x85,0x01,{0x02}},
	{0x92,0x01,{0xAD}},
	{0x93,0x01,{0x0A}},
	{0x94,0x01,{0x08}},
	{0x9B,0x01,{0x0F}},
	{0x9D,0x01,{0x30}},
	{0xAB,0x01,{0x00}},
	{0xAC,0x01,{0x00}},
	{0xAD,0x01,{0x00}},
	{0xB0,0x01,{0x05}},
	{0xB1,0x01,{0xA7}},
	{0xC2,0x01,{0x82}},
	{0xC5,0x01,{0x30}},
	{0xFF,0x01,{0x25}},
	{0xFB,0x01,{0x01}},
	{0x0A,0x01,{0x82}},
	{0x0B,0x01,{0x99}},
	{0x0C,0x01,{0x01}},
	{0x17,0x01,{0x82}},
	{0x18,0x01,{0x06}},
	{0x19,0x01,{0x0F}},
	{0x1E,0x01,{0x00}},
	{0x1F,0x01,{0x08}},
	{0x20,0x01,{0xA6}},
	{0x21,0x01,{0xB0}},
	{0x22,0x01,{0xA5}},
	{0x23,0x01,{0x05}},
	{0x24,0x01,{0xA7}},
	{0x25,0x01,{0x00}},
	{0x26,0x01,{0x08}},
	{0x27,0x01,{0xA6}},
	{0x28,0x01,{0xB0}},
	{0x29,0x01,{0xA5}},
	{0x2A,0x01,{0x05}},
	{0x2B,0x01,{0xA7}},
	{0x2D,0x01,{0x80}},
	{0x2F,0x01,{0x00}},
	{0x30,0x01,{0x00}},
	{0x31,0x01,{0x20}},
	{0x32,0x01,{0x00}},
	{0x33,0x01,{0x08}},
	{0x34,0x01,{0xA6}},
	{0x35,0x01,{0xB0}},
	{0x36,0x01,{0xA5}},
	{0x37,0x01,{0x05}},
	{0x38,0x01,{0xA7}},
	{0x40,0x01,{0x00}},
	{0x41,0x01,{0x00}},
	{0x42,0x01,{0x08}},
	{0x43,0x01,{0xA6}},
	{0x44,0x01,{0xB0}},
	{0x45,0x01,{0xA5}},
	{0x46,0x01,{0x05}},
	{0x47,0x01,{0x9D}},
	{0x4A,0x01,{0x00}},
	{0x4B,0x01,{0x05}},
	{0x4C,0x01,{0xA6}},
	{0x4F,0x01,{0x08}},
	{0x50,0x01,{0xA5}},
	{0x51,0x01,{0xB0}},
	{0x52,0x01,{0xA5}},
	{0x53,0x01,{0x05}},
	{0x54,0x01,{0xA7}},
	{0x55,0x01,{0x05}},
	{0x56,0x01,{0xA7}},
	{0x58,0x01,{0x00}},
	{0x59,0x01,{0x00}},
	{0x5A,0x01,{0x22}},
	{0x5B,0x01,{0xC0}},
	{0x5C,0x01,{0x00}},
	{0x5D,0x01,{0x05}},
	{0x5E,0x01,{0xB0}},
	{0x61,0x01,{0x08}},
	{0x62,0x01,{0xA6}},
	{0x63,0x01,{0xB0}},
	{0x64,0x01,{0xA5}},
	{0x65,0x01,{0x05}},
	{0x66,0x01,{0xA7}},
	{0x6B,0x01,{0x44}},
	{0x6C,0x01,{0x0D}},
	{0x6D,0x01,{0x0D}},
	{0x6E,0x01,{0x0F}},
	{0x6F,0x01,{0x0F}},
	{0x78,0x01,{0x00}},
	{0x79,0x01,{0x60}},
	{0x7A,0x01,{0x00}},
	{0x7B,0x01,{0x01}},
	{0x8A,0x01,{0x00}},
	{0xCA,0x01,{0x1D}},
	{0xCC,0x01,{0x0D}},
	{0xCD,0x01,{0x00}},
	{0xCE,0x01,{0x00}},
	{0xCF,0x01,{0x0D}},
	{0xD0,0x01,{0x00}},
	{0xD1,0x01,{0x00}},
	{0xD3,0x01,{0x11}},
	{0xD4,0x01,{0xDD}},
	{0xD5,0x01,{0x31}},
	{0xD6,0x01,{0x1D}},
	{0xD7,0x01,{0x01}},
	{0xD8,0x01,{0x0D}},
	{0xFF,0x01,{0x26}},
	{0xFB,0x01,{0x01}},
	{0x00,0x01,{0xA1}},
	{0x04,0x01,{0x47}},
	{0x0C,0x01,{0x13}},
	{0x0D,0x01,{0x00}},
	{0x0E,0x01,{0x01}},
	{0x0F,0x01,{0x0A}},
	{0x10,0x01,{0x0B}},
	{0x11,0x01,{0x00}},
	{0x13,0x01,{0x4C}},
	{0x14,0x01,{0x88}},
	{0x16,0x01,{0x80}},
	{0x19,0x01,{0x0B}},
	{0x1A,0x01,{0x60}},
	{0x1B,0x01,{0x0A}},
	{0x1C,0x01,{0xA5}},
	{0x1D,0x01,{0x00}},
	{0x1E,0x01,{0xAD}},
	{0x1F,0x01,{0xAD}},
	{0x24,0x01,{0x01}},
	{0x25,0x01,{0xAD}},
	{0x2F,0x01,{0x00}},
	{0x30,0x01,{0xAD}},
	{0x31,0x01,{0x11}},
	{0x32,0x01,{0x11}},
	{0x34,0x01,{0x02}},
	{0x35,0x01,{0xAD}},
	{0x36,0x01,{0x67}},
	{0x37,0x01,{0x16}},
	{0x38,0x01,{0x11}},
	{0x3F,0x01,{0x04}},
	{0x40,0x01,{0xAD}},
	{0x41,0x01,{0x00}},
	{0x42,0x01,{0x00}},
	{0x49,0x01,{0x00}},
	{0x58,0x01,{0xD7}},
	{0x59,0x01,{0xD7}},
	{0x5A,0x01,{0xD7}},
	{0x5B,0x01,{0xD7}},
	{0x5C,0x01,{0x01}},
	{0x5D,0x01,{0x02}},
	{0x5E,0x01,{0x04}},
	{0x5F,0x01,{0x00}},
	{0x60,0x01,{0x00}},
	{0x61,0x01,{0x00}},
	{0x62,0x01,{0x00}},
	{0x63,0x01,{0x60}},
	{0x64,0x01,{0xB0}},
	{0x65,0x01,{0x05}},
	{0x66,0x01,{0x50}},
	{0x67,0x01,{0x08}},
	{0x68,0x01,{0xA4}},
	{0x69,0x01,{0xD6}},
	{0x6A,0x01,{0xA2}},
	{0x6B,0x01,{0x00}},
	{0x6C,0x01,{0x00}},
	{0x6D,0x01,{0x00}},
	{0x70,0x01,{0x05}},
	{0x71,0x01,{0xA7}},
	{0x73,0x01,{0x08}},
	{0x74,0x01,{0xA4}},
	{0x75,0x01,{0xD6}},
	{0x76,0x01,{0xA2}},
	{0x77,0x01,{0x05}},
	{0x78,0x01,{0xA7}},
	{0x7A,0x01,{0x08}},
	{0x7B,0x01,{0xA4}},
	{0x7C,0x01,{0xD6}},
	{0x7D,0x01,{0xA2}},
	{0x7E,0x01,{0x05}},
	{0x7F,0x01,{0xA7}},
	{0x82,0x01,{0x60}},
	{0x83,0x01,{0xB0}},
	{0x84,0x01,{0x05}},
	{0x85,0x01,{0x50}},
	{0x86,0x01,{0x08}},
	{0x87,0x01,{0xA4}},
	{0x88,0x01,{0xD6}},
	{0x89,0x01,{0xA2}},
	{0x8A,0x01,{0x05}},
	{0x8B,0x01,{0xA7}},
	{0x8C,0x01,{0x00}},
	{0x8D,0x01,{0x00}},
	{0x8E,0x01,{0x01}},
	{0x8F,0x01,{0x00}},
	{0x90,0x01,{0x00}},
	{0x91,0x01,{0x00}},
	{0x92,0x01,{0x05}},
	{0x93,0x01,{0xF0}},
	{0x94,0x01,{0x00}},
	{0x96,0x01,{0x00}},
	{0x99,0x01,{0x0D}},
	{0x9A,0x01,{0x36}},
	{0x9B,0x01,{0x0C}},
	{0x9C,0x01,{0x9E}},
	{0xFF,0x01,{0x23}},
	{0xFB,0x01,{0x01}},
	{0x00,0x01,{0x60}},
	{0x05,0x01,{0x24}},
	{0x07,0x01,{0x00}},
	{0x12,0x01,{0xAB}},
	{0x15,0x01,{0xF5}},
	{0x16,0x01,{0x0B}},
	{0xFF,0x01,{0x10}},
	{0xFB,0x01,{0x01}},
	{0xBA,0x01,{0x02}},
	{0x35,0x01,{0x00}},
	{0x53,0x01,{0x24}},
	{0x55,0x01,{0x01}},

	{0x11,0x01,{0x00}},
	{REGFLAG_DELAY,105,{} },
	{0x29,0x01,{0x00}},
	{REGFLAG_DELAY, 20, {} },
};

static struct LCM_setting_table bl_level[] = {
	/* { 0xFF, 0x03, {0x98, 0x81, 0x00} }, */
	{0x51, 2, {0x04, 0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));
	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	pr_debug("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;
	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 8;
	params->dsi.vertical_frontporch = 8;
	//params->dsi.vertical_frontporch_for_low_power = 540;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 100;
	params->dsi.horizontal_frontporch = 107;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	//params->dsi.HS_TRAIL = 6;
	//params->dsi.HS_PRPR = 5;
	params->dsi.CLK_HS_PRPR = 7;
#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 360;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 360;	/* this value must be in MTK suggested table */
#endif
	params->dsi.PLL_CK_CMD = 360;
	params->dsi.PLL_CK_VDO = 360;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	//params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->full_content = 1;
	params->corner_pattern_width = 720;
	params->corner_pattern_height = 82;
	params->corner_pattern_height_bot = 82;
#endif
	params->blmap = blmap_table;
	params->blmap_size = sizeof(blmap_table)/sizeof(blmap_table[0]);
	params->brightness_max = 2047;
	params->brightness_min = 6;
}

static void lcm_init_power(void)
{
	/*pr_debug("lcm_init_power\n");*/
	printk("lcm_init_power\n");
	MDELAY(1);
	SET_LCM_VSP_PIN(1);
	MDELAY(2);
	SET_LCM_VSN_PIN(1);
}

static void lcm_suspend_power(void)
{
	/*pr_debug("lcm_suspend_power\n");*/
	printk("lcm_suspend_power\n");
	MDELAY(1);
	SET_LCM_VSN_PIN(0);
	MDELAY(1);
	SET_LCM_VSP_PIN(0);
}

static void lcm_resume_power(void)
{
	printk("lcm_resume_power\n");
	SET_LCM_VSP_PIN(1);
	MDELAY(2);
	SET_LCM_VSN_PIN(1);
	MDELAY(1);
	//base voltage = 4.0 each step = 100mV; 4.0+20 * 0.1 = 6.0v;
	if ( display_bias_setting(18) )
		pr_err("fatal error: lcd gate ic setting failed \n");
	MDELAY(11);
	SET_RESET_PIN(0);
	MDELAY(3);
	SET_RESET_PIN(1);
	MDELAY(6);
}

static void lcm_init(void)
{
	/*pr_debug("lcm_init\n");*/
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
#ifdef ODM_WT_EDIT
//Bin.Su@ODM_WT.BSP.TP.Timing.2018/11/01,Adjust TP timing in suspend and resume status
	MDELAY(11);
	update_tpfw_notifier_call_chain(1,NULL);
	MDELAY(1);
#endif
	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_huaxian_lcm_mode = cmd mode :%d----\n", lcm_dsi_mode);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_huaxian_lcm_mode = vdo mode :%d----\n", lcm_dsi_mode);
	}
}

static void lcm_suspend(void)
{
	/*pr_debug("lcm_suspend\n");*/

	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

	/* SET_RESET_PIN(0); */
}

static void lcm_resume(void)
{
	/*pr_debug("lcm_resume\n");*/
	lcm_init();
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;

	unsigned char x0_MSB = (x0 & 0xFF);

	unsigned int data_array[2];
	unsigned char read_buf[2];
	unsigned int num1 = 0, num2 = 0;

	struct LCM_setting_table switch_table_page1[] = {
		{ 0xFF, 0x03, {0x98, 0x81, 0x01} }
	};

	struct LCM_setting_table switch_table_page0[] = {
		{ 0xFF, 0x03, {0x98, 0x81, 0x00} }
	};

	MDELAY(20);

	num1 = sizeof(switch_table_page1) / sizeof(struct LCM_setting_table);

	push_table(NULL, switch_table_page1, num1, 1);
	pr_debug("before read ATA check x0_MSB = 0x%x\n", x0_MSB);
	pr_debug("before read ATA check read buff = 0x%x\n", read_buf[0]);

	data_array[0] = 0x0002390A;	/* HS packet */
	data_array[1] = x0_MSB << 8 | 0x55;
	//data_array[2] = (x0_MSB);
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00013700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x55, read_buf, 1);

	pr_debug("after read ATA check size = 0x%x\n", read_buf[0]);

	num2 = sizeof(switch_table_page0) / sizeof(struct LCM_setting_table);

	push_table(NULL, switch_table_page0, num2, 1);

	if (read_buf[0] == x0_MSB)
		ret = 1;
	else
		ret = 0;

	return ret;
#else
	return 0;
#endif
}

static struct LCM_setting_table lcm_cabc_enter_setting[] = {
	{0xff, 0x01, {0x10}},
	{0xfb, 0x01, {0x01}},
	{0x53,1,{0x2c}},
	{0x55,1,{0x01}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_cabc_exit_setting[] = {
	{0xff, 0x01, {0x10}},
	{0xfb, 0x01, {0x01}},
	{0x53,1,{0x2c}},
	{0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_set_cabc_mode_cmdq(void *handle, unsigned int level)
{
	pr_debug("[lcd] nt36525b_huaxian_cabc_mode is %d \n", level);
	if (level) {
		push_table(handle,lcm_cabc_enter_setting, sizeof(lcm_cabc_enter_setting) / sizeof(struct LCM_setting_table), 1);
	} else {
		push_table(handle,lcm_cabc_exit_setting, sizeof(lcm_cabc_exit_setting) / sizeof(struct LCM_setting_table), 1);
	}
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	pr_debug("nt36525b_huaxian backlight_level = %d\n", level);
	bl_level[0].para_list[0] = 0x0007&(level >> 8);
	bl_level[0].para_list[1] = 0x00FF&(level);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK
	lcm_resume_power();
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(100);
	update_tpfw_notifier_call_chain(2,NULL);
	MDELAY(1);
	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_huaxian_lcm_mode = cmd mode esd recovery :%d----\n", lcm_dsi_mode);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_huaxian_lcm_mode = vdo mode esd recovery :%d----\n", lcm_dsi_mode);
	}
	pr_debug("lcm_esd_recovery\n");
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
	return FALSE;
#else
	return FALSE;
#endif
}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
/* customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register */
	if (mode == 0) {	/* V2C */
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;	/* mode control addr */
		lcm_switch_mode_cmd.val[0] = 0x13;	/* enabel GRAM firstly, ensure writing one frame to GRAM */
		lcm_switch_mode_cmd.val[1] = 0x10;	/* disable video mode secondly */
	} else {		/* C2V */
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;	/* disable GRAM and enable video mode */
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

#if (LCM_DSI_CMD_MODE)

/* partial update restrictions:
 * 1. roi width must be 1080 (full lcm width)
 * 2. vertical start (y) must be multiple of 16
 * 3. vertical height (h) must be multiple of 16
 */

static void lcm_validate_roi(int *x, int *y, int *width, int *height)
{
	unsigned int y1 = *y;
	unsigned int y2 = *height + y1 - 1;
	unsigned int x1, w, h;

	x1 = 0;
	w = FRAME_WIDTH;

	y1 = round_down(y1, 16);
	h = y2 - y1 + 1;

	/* in some cases, roi maybe empty. In this case we need to use minimu roi */
	if (h < 16)
		h = 16;

	h = round_up(h, 16);

	/* check height again */
	if (y1 >= FRAME_HEIGHT || y1 + h > FRAME_HEIGHT) {
		/* assign full screen roi */
		pr_debug("%s calc error,assign full roi:y=%d,h=%d\n", __func__, *y, *height);
		y1 = 0;
		h = FRAME_HEIGHT;
	}

	/*pr_debug("lcm_validate_roi (%d,%d,%d,%d) to (%d,%d,%d,%d)\n",*/
	/*	*x, *y, *width, *height, x1, y1, w, h);*/

	*x = x1;
	*width = w;
	*y = y1;
	*height = h;
}
#endif

struct LCM_DRIVER nt36525b_huaxian_hdp_dsi_vdo_lcm_drv = {
	.name = "nt36525b_huaxian_hdp_dsi_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_recover = lcm_esd_recover,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.set_cabc_mode_cmdq = lcm_set_cabc_mode_cmdq,
	.ata_check = lcm_ata_check,
	.switch_mode = lcm_switch_mode,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif
};
