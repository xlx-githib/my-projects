#include "ov2640cfg.h"
#include "ov2640.h"

uint8_t ov2640_read_reg(uint16_t reg)
{
	uint8_t data = 0;
	sccb_start();
	sccb_write_byte(OV2640_ADDR);//0x60 д״̬
	delay_us(100);
	sccb_write_byte(reg);
	delay_us(100);
	sccb_stop();
	
	delay_us(100);
	sccb_start();
	sccb_write_byte(OV2640_ADDR | 0x01);//0x61 ��״̬
	delay_us(100);
	data = sccb_read_byte();
	delay_us(100);
	sccb_nack();
	sccb_stop();
	return data;
}

uint8_t ov2640_write_reg(uint16_t reg, uint8_t data)
{
	uint8_t ret = 0;
	sccb_start();
	delay_us(100);
	if(sccb_write_byte(OV2640_ADDR))ret = 1;
	delay_us(100);
	if(sccb_write_byte(reg))ret = 1;
	delay_us(100);
	if(sccb_write_byte(data))ret = 1;
	delay_us(100);
	sccb_stop();
	return ret;//0 == success  1 == fail
}

uint8_t ov2640_init(void)
{
	uint16_t reg;
	uint16_t i;
	
	OV2640_PWDN(0);//0 == ������ 1 == �رգ�ʡ��ģʽ��
	delay_ms(10);
	OV2640_RST(0);//����Ӳ����λ
	delay_ms(20);//�ȴ�Ӳ����λ
	OV2640_RST(1);//����Ӳ����λ
	delay_ms(10);
	
	sccb_init();
	delay_ms(5);
	ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x01);//0x01=ѡ��DSP�Ĵ���ҳ
	ov2640_write_reg(OV2640_SENSOR_COM7, 0x80);// ͨ�ÿ���7����Ҫ����
	//0x80 1000 0000                            bit[7]: ����λ
	//     7654 3210λ                          bit[6]: �ֱ���ѡ���λ
	//     bit[7]: ����λ                       bit[5]: �����ʽ��RGB/Y
	//                                          bit[4:2]: �ֱ�������
  //                                          bit[1:0]: ������ʱ��Դ
	delay_ms(50);
	reg = ov2640_read_reg(OV2640_SENSOR_MIDH);
	reg <<= 8;
	reg |= ov2640_read_reg(OV2640_SENSOR_MIDL);
	if(reg != OV2640_MID)
	{
		printf("ERROR: MID = %d\r\n", reg);
		return 1;
	}
	reg = ov2640_read_reg(OV2640_SENSOR_PIDH);
	reg <<= 8;
	reg |= ov2640_read_reg(OV2640_SENSOR_PIDL);
	if(reg != OV2640_PID)
	{
		printf("ERROR: PID = %d\r\n", reg);
		return 1;
	}
	
//	for(i = 0;i < sizeof(ov2640_uxga_init_reg_tbl) / 2;i++)
//	{
//		ov2640_write_reg(ov2640_uxga_init_reg_tbl[i][0], ov2640_uxga_init_reg_tbl[i][1]);
//	}
	for (i = 0; i < (sizeof(ov2640_svga_init_reg_tbl) / 2); i++)
  {
     ov2640_write_reg(ov2640_svga_init_reg_tbl[i][0], ov2640_svga_init_reg_tbl[i][1]);
  }
	return 0;//success
	
}

void ov2640_jpeg_mode(void)
{
    uint16_t i = 0;
	
    for (i = 0; i < (sizeof(ov2640_yuv422_reg_tbl) / 2); i++)
    {
        ov2640_write_reg(ov2640_yuv422_reg_tbl[i][0], ov2640_yuv422_reg_tbl[i][1]); 
    }
    for (i = 0; i < (sizeof(ov2640_jpeg_reg_tbl) / 2); i++)
    {
        ov2640_write_reg(ov2640_jpeg_reg_tbl[i][0], ov2640_jpeg_reg_tbl[i][1]);    
    }
}

void ov2640_rgb565_mode(void)
{
	uint16_t i;
	for(i = 0;i < sizeof(ov2640_rgb565_reg_tbl) / 2;i++)
	{
		ov2640_write_reg(ov2640_rgb565_reg_tbl[i][0], ov2640_rgb565_reg_tbl[i][1]);
	}
	return ;
}

uint8_t ov2640_outsize_set(uint16_t width, uint16_t height)
{
	uint8_t temp;
	uint16_t outw;
	uint16_t outh;
	
	if (width % 4) return 1;
  if (height % 4) return 2;

  outw = width / 4;
  outh = height / 4;
	
	ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x00);//0x00=ѡ�񴫸����Ĵ���ҳ
	ov2640_write_reg(OV2640_DSP_RESET, 0x04);// DSP����λ bit[7]: ��λDSP bit[2]: ʹ�ܴ������ߴ�����
	ov2640_write_reg(OV2640_DSP_ZMOW, outw & 0xFF);// ����������� �����������ź��������ȵ�8λ
	ov2640_write_reg(OV2640_DSP_ZMOH, outh & 0xFF);// ��������߶� �����������ź������߶ȵ�8λ
	
	temp = (outw >> 8) & 0x03;
	temp |= (outh >> 6) & 0x04;
	ov2640_write_reg(OV2640_DSP_ZMHH, temp);// ���Ÿ�λ���-------- bit[7:6]: ZMOH��bit[9:8]--------- bit[1:0]: ZMOW��bit[9:8]                                                                            
	ov2640_write_reg(OV2640_DSP_RESET, 0x00);// DSP����λ bit[7]: ��λDSP bit[2]: ʹ�ܴ������ߴ�����,�ڶ�λΪ0���رմ������ߴ����á�
                                       
  return 0;                                    
}

/**
 * @brief  设置传感器采集窗口(在原始图像上的起始坐标与大小)
 * @param  sx, sy       : 窗口在原始图像上的起始坐标
 * @param  width, height: 窗口大小
 * @retval 0 成功
 */
uint8_t ov2640_window_set(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t endx = sx + width / 2;
    uint16_t endy = sy + height / 2;
    uint8_t temp;

    ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x01);              /* 切到 DSP 寄存器页 */
    temp = ov2640_read_reg(OV2640_SENSOR_COM1);
    temp &= 0xF0;
    temp |= ((endy & 0x03) << 2) | (sy & 0x03);
    ov2640_write_reg(OV2640_SENSOR_COM1, temp);              /* Vref 起止低 2 位 */
    ov2640_write_reg(OV2640_SENSOR_VSTART, sy >> 2);         /* Vref 起始高 8 位 */
    ov2640_write_reg(OV2640_SENSOR_VEND, endy >> 2);         /* Vref 结束高 8 位 */

    temp = ov2640_read_reg(OV2640_SENSOR_REG32);
    temp &= 0xC0;
    temp |= ((endx & 0x07) << 3) | (sx & 0x07);
    ov2640_write_reg(OV2640_SENSOR_REG32, temp);             /* Href 起止低 3 位 */
    ov2640_write_reg(OV2640_SENSOR_HREFST, sx >> 3);         /* Href 起始高 8 位 */
    ov2640_write_reg(OV2640_SENSOR_HREFEND, endx >> 3);      /* Href 结束高 8 位 */
    return 0;
}

/**
 * @brief  设置 DVP 输出窗口(在采集窗口内的偏移与输出大小)
 * @param  offx, offy   : 输出窗口在采集窗口上的偏移
 * @param  width, height: 输出窗口大小, 必须为 4 的倍数
 * @retval 0 成功, 非 0 失败
 */
uint8_t ov2640_image_win_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    uint16_t hsize;
    uint16_t vsize;
    uint8_t temp;

    if (width % 4) return 1;
    if (height % 4) return 2;

    hsize = width / 4;
    vsize = height / 4;

    ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x00);
    ov2640_write_reg(OV2640_DSP_RESET, 0x04);
    ov2640_write_reg(OV2640_DSP_HSIZE1, hsize & 0xFF);
    ov2640_write_reg(OV2640_DSP_VSIZE1, vsize & 0xFF);
    ov2640_write_reg(OV2640_DSP_XOFFL, offx & 0xFF);
    ov2640_write_reg(OV2640_DSP_YOFFL, offy & 0xFF);
    temp = (vsize >> 1) & 0x80;
    temp |= (offy >> 4) & 0x70;
    temp |= (hsize >> 5) & 0x08;
    temp |= (offx >> 8) & 0x07;
    ov2640_write_reg(OV2640_DSP_VHYX, temp);
    ov2640_write_reg(OV2640_DSP_TEST, (hsize >> 2) & 0x80);
    ov2640_write_reg(OV2640_DSP_RESET, 0x00);
    return 0;
}

/**
 * @brief  设置传感器原始图像尺寸(UXGA/SVGA/CIF 等)
 * @param  width, height: 原始图像尺寸
 * @retval 0 成功
 */
uint8_t ov2640_imagesize_set(uint16_t width, uint16_t height)
{
    uint8_t temp;

    ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x00);
    ov2640_write_reg(OV2640_DSP_RESET, 0x04);
    ov2640_write_reg(OV2640_DSP_HSIZE2, (width >> 3) & 0xFF);   /* HSIZE 的 bit10:3 */
    ov2640_write_reg(OV2640_DSP_VSIZE2, (height >> 3) & 0xFF);  /* VSIZE 的 bit10:3 */

    temp = (width & 0x07) << 3;
    temp |= height & 0x07;
    temp |= (width >> 4) & 0x80;
    ov2640_write_reg(OV2640_DSP_SIZEL, temp);
    ov2640_write_reg(OV2640_DSP_RESET, 0x00);
    return 0;
}

