#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (540)
#define FRAME_HEIGHT (960)

#define LCM_ID_HX8389B 0x89


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

 unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define REGFLAG_DELAY 0xFF
#define REGFLAG_END_OF_TABLE 0xFFF


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							0

static LCM_setting_table_V3 lcm_initialization_setting[] = {
	/*
	Note:
	Data ID will depends on the following rule.
	    count of parameters > 1 ==> DataID=0x39
	    count of parameters = 1 ==> DataID=0x15
	    count of parameters = 0 ==> DataID=0x05
	Structure format:
	    {DataID, DCS command, count of parameters, {parameter list}},
	    {REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, milliseconds, {}},
	*/

	// Set extension command
	{0x39, 0xB9,  3, {0xFF,0x83,0x89}},

	// Set MIPI control
	{0x39, 0xBA,  7, {0x41,0x93,0x00,0x16,0xA4,0x10,0x18}},

	{0x15, 0xC6,  1, {0x08}},

	// Set power (VGH=VSP*2-VSN, VGL=VSN*2-VSP)
	{0x39, 0xB1, 19, {0x00,0x00,0x07,0xEF,0x97,0x10,0x11,0x94,0xF1,0x26,
	                  0x2E,0x3F,0x3F,0x42,0x01,0x32,0xF7,0x20,0x80}},

	// Set power option
	{0x39, 0xDE,  3, {0x05,0x58,0x10}},

	// Set display related register
	{0x39, 0xB2,  7, {0x00,0x00,0x78,0x0E,0x03,0x3F,0x80}},

	// Set panel driving timing
	{0x39, 0xB4, 23, {0x80,0x08,0x00,0x32,0x10,0x07,0x32,0x10,0x03,0x32,
	                  0x10,0x07,0x27,0x01,0x5A,0x0B,0x37,0x05,0x40,0x14,
	                  0x50,0x58,0x0A}},

	// Set GIP (Gate In Panel)
	{0x39, 0xD5, 48, {0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x60,0x00,
	                  0x99,0x88,0x88,0x88,0x88,0x23,0x88,0x01,0x88,0x67,
	                  0x88,0x45,0x01,0x23,0x23,0x45,0x88,0x88,0x88,0x88,
	                  0x99,0x88,0x88,0x88,0x54,0x88,0x76,0x88,0x10,0x88,
	                  0x32,0x32,0x10,0x88,0x88,0x88,0x88,0x88}},

	// Set Gamma
	{0x39, 0xE0, 34, {0x00,0x18,0x1F,0x3B,0x3E,0x3F,0x2F,0x4A,0x07,0x0E,
	                  0x0F,0x13,0x16,0x13,0x13,0x0F,0x19,0x00,0x18,0x1F,
	                  0x3B,0x3E,0x3F,0x2F,0x4A,0x07,0x0E,0x0F,0x13,0x16,
	                  0x13,0x13,0x0F,0x19}},

	// Set DGC-LUT (Digital Gamma Curve Look-up Table)
	{0x39, 0xC1,127, {0x01,0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0x40,
	                  0x48,0x50,0x58,0x60,0x68,0x70,0x78,0x80,0x88,0x90,
	                  0x98,0xA0,0xA8,0xB0,0xB8,0xC0,0xC8,0xD0,0xD8,0xE0,
	                  0xE8,0xF0,0xF8,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
	                  0x00,0x00,0x00,0x00,0x08,0x10,0x18,0x20,0x28,0x30,
	                  0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x78,0x80,
	                  0x88,0x90,0x98,0xA0,0xA8,0xB0,0xB8,0xC0,0xC8,0xD0,
	                  0xD8,0xE0,0xE8,0xF0,0xF8,0xFF,0x00,0x00,0x00,0x00,
	                  0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x17,0x20,
	                  0x29,0x31,0x39,0x42,0x4A,0x53,0x5B,0x63,0x6B,0x74,
	                  0x7C,0x84,0x8C,0x94,0x9C,0xA4,0xAC,0xB5,0xBB,0xC3,
	                  0xCB,0xD3,0xDB,0xE2,0xEA,0xF2,0xF8,0xFF,0x00,0x00,
	                  0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	// Set VCOM = -0.990 V
	{0x39, 0xB6,  4, {0x00,0x93,0x00,0x93}},

	// Set panel (Normal black; Scan direction; RGB/BGR source driver direction)
	{0x15, 0xCC,  1, {0x02}},

	// Set internal TE (Tear Effect)
	{0x39, 0xB7,  3, {0x00,0x00,0x50}},

	// --- Resume ------------------------------------
	// Do resume within initial is NOT recommended!

	/*
	// Sleep out
	{0x05, 0x11, 0, {}},
	{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 120, {}},

	// Display on
	{0x05, 0x29, 0, {}},
	{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 20, {}},
	*/
};


static LCM_setting_table_V3 lcm_suspend_setting[] = {
	// Display off
	//{0x05, 0x28, 0, {}},
	//{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 20, {}},
	// Note:
	// Remove the Display off command is a workaround
	// to reduce the idle currect of Himax HX8389-B driver IC.

	// Sleep in
	{0x05, 0x10, 0, {}},
	{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 120, {}},
};


static LCM_setting_table_V3 lcm_resume_setting[] = {
	// Sleep out
	{0x05, 0x11, 0, {}},
	{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 120, {}},

	// Display on
	{0x05, 0x29, 0, {}},
	{REGFLAG_ESCAPE_ID, REGFLAG_DELAY_MS_V3, 20, {}},
};


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 0x05;// 3    2
		params->dsi.vertical_backporch					= 14;// 20   1
		params->dsi.vertical_frontporch					= 12; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 0x16;// 50  2
		params->dsi.horizontal_backporch				= 0x38;
		params->dsi.horizontal_frontporch				= 0x18;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 

		// Bit rate calculation
		//1 Every lane speed
		//params->dsi.pll_select=1;
		//params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
		params->dsi.PLL_CLOCK=250;
		//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
#if (LCM_DSI_CMD_MODE)
		//params->dsi.fbk_div =9;
#else
		//params->dsi.fbk_div =9;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
#endif
		//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
}

static void lcm_init(void)
{
	#ifdef BUILD_LK
	  printf("[lk][lcm] %s \n", __func__);
  #else
    printk("[kernel][lcm] %s \n", __func__);
  #endif
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(10);

	dsi_set_cmdq_V3(lcm_initialization_setting, sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]), 1);
	dsi_set_cmdq_V3(lcm_resume_setting, sizeof(lcm_resume_setting)/sizeof(lcm_resume_setting[0]), 1);
}



static void lcm_suspend(void)
{
		dsi_set_cmdq_V3(lcm_suspend_setting, sizeof(lcm_suspend_setting)/sizeof(lcm_suspend_setting[0]), 1);   
}


static void lcm_resume(void)
{
	lcm_init();

    #ifdef BUILD_LK
	  printf("[LK]------hx8389b----%s------\n",__func__);
    #else
	  printk("[KERNEL]------hx8389b----%s------\n",__func__);
    #endif	
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
	unsigned int id,id1=0;
	unsigned char buffer[2];
	unsigned int array[16];  

	SET_RESET_PIN(0);
	MDELAY(20); 
	SET_RESET_PIN(1);
	MDELAY(20); 


	array[0]=0x00043902;
	array[1]=0x8983FFB9;// page enable
	dsi_set_cmdq(array, 2, 1);
	//MDELAY(10);
//{0x39,0xBA,7,{0x41,0x93,0x00,0x16,0xA4,0x10,0x18}},	
	array[0]=0x00083902;
	array[1]=0x009341BA;// page enable
	array[2]=0x1810a416;
	dsi_set_cmdq(array, 3, 1);

	array[0] = 0x00043700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 3);
	id  =  buffer[1]; 
	id1 =  buffer[0];
	
#ifdef BUILD_LK
	printf("%s, id = 0x%08x id1=%x \n", __func__, id,id1);
#else
	printk("%s, id = 0x%08x  id1=%x \n",__func__, id,id1);
#endif

	return (LCM_ID_HX8389B == id)?1:0;

}



static unsigned int lcm_esd_check(void)
{
	unsigned int ret=FALSE;
  #ifndef BUILD_LK
	char  buffer[6];
	int   array[4];

#if 1
	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}
#endif
	array[0] = 0x00083700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 2);
	//printk(" esd buffer0 =%x,buffer1 =%x  \n",buffer[0],buffer[1]);
	//read_reg_v2(0x09,buffer,5);
	//printk(" esd buffer0=%x, buffer1 =%x buffer2=%x,buffer3=%x,buffer4=%x \n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
#if 1
	if(buffer[0]==0x1C)
	{
		ret=FALSE;
	}
	else
	{			 
		ret=TRUE;
	}
#endif
 #endif
 return ret;

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	
	#ifndef BUILD_LK
	printk("lcm_esd_recover  hx8389b_video_tianma \n");
	#endif
	return TRUE;
}




LCM_DRIVER hx8389b_qhd_dsi_vdo_tianma_lcm_drv = 
{
    .name			= "hx8389b_qhd_dsi_vdo_tianma_55",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
