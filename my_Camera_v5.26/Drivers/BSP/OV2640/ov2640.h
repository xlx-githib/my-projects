#ifndef _OV2640_H
#define _OV2640_H

#include "gpio.h"
#include "sccb.h"
#include "delay.h"
#include "usart.h"

#define OV2640_PWDN(x) do{x ?\
	HAL_GPIO_WritePin(PWDN_GPIO_Port, PWDN_Pin, GPIO_PIN_SET):\
	HAL_GPIO_WritePin(PWDN_GPIO_Port, PWDN_Pin, GPIO_PIN_RESET);\
}while(0)

#define OV2640_RST(x) do{x ?\
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET):\
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);\
}while(0)

#define Image_FlipVer       1               

#define OV2640_MID          0x7FA2
#define OV2640_PID          0x2642
#define OV2640_ADDR         0x60            

/* SVGA 初始化表对应的传感器原始图像窗口(用于缩放预览的窗口计算) */
#define OV2640_SENSOR_WIDTH    800
#define OV2640_SENSOR_HEIGHT   600
 
//---------------------------ͼ�����������ʽ���ߴ�����-------------------------------------------//   

#define OV2640_DSP_R_BYPASS     0x05    // Rͨ����·���� ���ƺ�ɫͨ���Ƿ񾭹�DSP����
                                        
#define OV2640_DSP_Qs           0x44    // ������������ ����JPEGѹ��������/��������
                                        
#define OV2640_DSP_CTRL         0x50    // DSPͨ�ÿ��� bit[7]: �����ʽѡ�� bit[6:4]: ���˳�����
                                                                          
#define OV2640_DSP_HSIZE1       0x51    // ˮƽ�ߴ��8λ �������ͼ����ȵ�bit[7:0]
                                        
#define OV2640_DSP_VSIZE1       0x52    // ��ֱ�ߴ��8λ �������ͼ��߶ȵ�bit[7:0]                                      

#define OV2640_DSP_XOFFL        0x53    // Xƫ�Ƶ�8λ ����ͼ�񴰿ڵ�ˮƽ��ʼλ�õ�8λ
                                       
#define OV2640_DSP_YOFFL        0x54    // Yƫ�Ƶ�8λ ����ͼ�񴰿ڵĴ�ֱ��ʼλ�õ�8λ
                                        
#define OV2640_DSP_VHYX         0x55    // �ߴ�/ƫ�Ƹ�λ���
                                        // bit[7]: VSIZE��bit[8]
                                        // bit[6:4]: YOFF��bit[10:8]
                                        // bit[3]: HSIZE��bit[8]
                                        // bit[2:0]: XOFF��bit[10:8]

#define OV2640_DSP_DPRP         0x56    // ����/���Կ���

#define OV2640_DSP_TEST         0x57    // ���ԼĴ��� bit[7]: HSIZE��bit[9]
                                        
#define OV2640_DSP_ZMOW         0x5A    // ����������� �����������ź��������ȵ�8λ
                                        
#define OV2640_DSP_ZMOH         0x5B    // ��������߶� �����������ź������߶ȵ�8λ
                                        
#define OV2640_DSP_ZMHH         0x5C    // ���Ÿ�λ���
                                        // bit[7:6]: ZMOH��bit[9:8]
                                        // bit[1:0]: ZMOW��bit[9:8]

#define OV2640_DSP_BPADDR       0x7C    // BP����·����ַ ���ڷ���DSP�ڲ��Ĵ�����ĵ�ַָ��
                                        
#define OV2640_DSP_BPDATA       0x7D    // BP����·������ ͨ���˼Ĵ�����д0x7Cָ����ڲ�����
                                        
#define OV2640_DSP_CTRL2        0x86    // DSP���ƼĴ���2 ����DCMI�ӿ�ʱ������ʱ�Ӽ��Ե�
                                        
#define OV2640_DSP_CTRL3        0x87    // DSP���ƼĴ���3
                                        // bit[2]: ��������ʱ�ӷ�Χ
                                        // bit[1:0]: ���������������

#define OV2640_DSP_SIZEL        0x8C    // �������ߴ���ֽڻ��
                                        // bit[7:3]: HSIZE��bit[2:0]��VSIZE��bit[2:0]
                                        // bit[7]: HSIZE��bit[10]

#define OV2640_DSP_HSIZE2       0xC0    // ������ˮƽ�ߴ���ֽ� ���ô�����ʵ�������ˮƽ������bit[10:3]
                                        
#define OV2640_DSP_VSIZE2       0xC1    // ��������ֱ�ߴ���ֽ� ���ô�����ʵ������Ĵ�ֱ������bit[10:3]
                                        
#define OV2640_DSP_CTRL0        0xC2    // DSP����0
                                        // bit[7]: �г�ͬ���źż���
                                        // bit[6]: ����ʱ�Ӽ���
                                        // bit[5]: ������Ч��ƽ

#define OV2640_DSP_CTRL1        0xC3    // DSP����1
                                        // bit[7]: �Զ�����ʹ��
                                        // bit[6]: ����/�Աȶȵ���
                                        // bit[0]: ��/�����ģʽ

#define OV2640_DSP_R_DVP_SP     0xD3    // DVP�ӿ��ٶȿ���
                                        // bit[6:4]: ����DVP���ʱ�ӷ�Ƶ
                                        // bit[3:0]: ��������ʱ���ٶ�

#define OV2640_DSP_IMAGE_MODE   0xDA    // ͼ�������ʽ
                                        // bit[7]: ������λ
                                        // bit[6]: �ֽڽ���
                                        // bit[5]: ˮƽ����
                                        // bit[4]: ��ֱ��ת
                                        // bit[3:0]: �����ʽ
                                        //   0x00=YUV422
                                        //   0x08=RGB555
                                        //   0x09=RGB565
                                        //   0x10=JPEG

#define OV2640_DSP_RESET        0xE0    // DSP����λ
                                        // bit[7]: ��λDSP
                                        // bit[2]: ʹ�ܴ������ߴ�����

#define OV2640_DSP_MS_SP        0xF0    // ��ʱ���ٶ�����

#define OV2640_DSP_SS_ID        0x7F    // ���豸ID

#define OV2640_DSP_SS_CTRL      0xF8    // ���豸����

#define OV2640_DSP_MC_BIST      0xF9    // �洢���ڽ��Բ���

#define OV2640_DSP_MC_AL        0xFA    // �洢�����Ƶ�ַ���ֽ�

#define OV2640_DSP_MC_AH        0xFB    // �洢�����Ƶ�ַ���ֽ�

#define OV2640_DSP_MC_D         0xFC    // �洢����������

#define OV2640_DSP_P_STATUS     0xFE    // ����״̬��ȡ �ɶ�ȡSTROBE��VSYNC������״̬
                                        
#define OV2640_DSP_RA_DLMT      0xFF    // �Ĵ�����ַ����/ҳѡ��
                                        // ��������Ҫ�ļĴ�����
                                        // 0x00=ѡ�񴫸����Ĵ���ҳ
                                        // 0x01=ѡ��DSP�Ĵ���ҳ
                                        // ���мĴ�������ǰ��������˼Ĵ���

//----------------------------------ͼ��ɼ����ع⡢��ƽ��------------------------------------------------//

#define OV2640_SENSOR_GAIN       0x00    // ������� ����ͼ���������������
                                         
#define OV2640_SENSOR_COM1       0x03    // ͨ�ÿ���1
                                         // bit[7]: ������λ
                                         // bit[6]: ʡ��ģʽ
                                         // bit[5]: �����ʽѡ��

#define OV2640_SENSOR_REG04      0x04    // �Ĵ���04
                                         // bit[7]: ��ֱ��ת
                                         // bit[6]: ˮƽ����
                                         // bit[5]: ����BRͨ��
                                         // bit[4]: �ڰ�ģʽ

#define OV2640_SENSOR_REG08      0x08    // �Ĵ���08

#define OV2640_SENSOR_COM2       0x09    // ͨ�ÿ���2
                                         // bit[7]: ����ģʽ
                                         // bit[6]: ʱ��ʹ��

#define OV2640_SENSOR_PIDH       0x0A    // ��ƷID���ֽڣ�ֻ����
                                         // �̶�ֵ��0x26
                                         // ������֤оƬ�Ƿ�ΪOV2640

#define OV2640_SENSOR_PIDL       0x0B    // ��ƷID���ֽڣ�ֻ����
                                         // �̶�ֵ��0x42
                                         // PID����ֵ��0x2642

#define OV2640_SENSOR_COM3       0x0C    // ͨ�ÿ���3
                                         // bit[7]: ����ʹ��
                                         // bit[6]: �����ٶ�

#define OV2640_SENSOR_COM4       0x0D    // ͨ�ÿ���4 �����������
                                         
#define OV2640_SENSOR_AEC        0x10    // �Զ��ع���� �����ع�ʱ��ĸ�λ
                                        
#define OV2640_SENSOR_CLKRC      0x11    // ʱ�ӷ�Ƶ����
                                         // bit[7]: ʱ�ӷ�Ƶʹ��
                                         // bit[5:0]: ��Ƶϵ��

#define OV2640_SENSOR_COM7       0x12    // ͨ�ÿ���7����Ҫ����
                                         // bit[7]: ����λ
                                         // bit[6]: �ֱ���ѡ���λ
                                         // bit[5]: �����ʽ��RGB/YUV��
                                         // bit[4:2]: �ֱ�������
                                         // bit[1:0]: ������ʱ��Դ

#define OV2640_SENSOR_COM8       0x13    // ͨ�ÿ���8 ����ģ���źŴ���                                        

#define OV2640_SENSOR_COM9       0x14    // ͨ�ÿ���9 ģ���·����
                                         
#define OV2640_SENSOR_COM10      0x15    // ͨ�ÿ���10
                                         // bit[5]: HREF�źż���
                                         // bit[4]: VSYNC�źż���
                                         // bit[3]: PCLK�źż���
                                         // bit[0]: ���������ʽ

#define OV2640_SENSOR_HREFST     0x17    // HREF������Ч�źţ���ʼλ�� ����һ������Ч���صĿ�ʼλ�ø�8λ
                                         
#define OV2640_SENSOR_HREFEND    0x18    // HREF������Ч�źţ�����λ�� ����һ������Ч���صĽ���λ�ø�8λ
                                         
#define OV2640_SENSOR_VSTART     0x19    // ��ֱ��ʼλ�� ����һ֡����Ч�еĿ�ʼλ�ø�8λ
                                         
#define OV2640_SENSOR_VEND       0x1A    // ��ֱ����λ�� ����һ֡����Ч�еĽ���λ�ø�8λ
                                         
#define OV2640_SENSOR_MIDH       0x1C    // ������ID���ֽڣ�ֻ����
                                         // �̶�ֵ��0x7F

#define OV2640_SENSOR_MIDL       0x1D    // ������ID���ֽڣ�ֻ����
                                         // �̶�ֵ��0xA2
                                         // MID����ֵ��0x7FA2

#define OV2640_SENSOR_AEW        0x24    // �Զ��عⴰ������

#define OV2640_SENSOR_AEB        0x25    // �Զ��عⲹ�� �����ع�Ŀ�������ֵ                                         

#define OV2640_SENSOR_W          0x26    // ��ƽ�����

#define OV2640_SENSOR_REG2A      0x2A    // �Ĵ���2A

#define OV2640_SENSOR_FRARL      0x2B    // ֡���ʵ��ڵ��ֽ�

#define OV2640_SENSOR_ADDVSL     0x2D    // ADC�ο���ѹ����

#define OV2640_SENSOR_ADDVHS     0x2E    // ADC�ߵ�ѹ����

#define OV2640_SENSOR_YAVG       0x2F    // Y�����ȣ�ƽ��ֵ��ֻ���� ��ȡ��ǰ֡��ƽ������                                        

#define OV2640_SENSOR_REG32      0x32    // �Ĵ���32
                                         // bit[7:6]: �ֱ�������
                                         //   SVGA: 0x09
                                         //   UXGA: 0x36

#define OV2640_SENSOR_ARCOM2     0x34    // ģ���·����2

#define OV2640_SENSOR_REG45      0x45    // �Ĵ���45 ֡���ʵ���
                                         
#define OV2640_SENSOR_FLL        0x46    // ֡���ȵ��ֽ�

#define OV2640_SENSOR_FLH        0x47    // ֡���ȸ��ֽ�

#define OV2640_SENSOR_COM19      0x48    // ͨ�ÿ���19

#define OV2640_SENSOR_ZOOMS      0x49    // ���ֱ佹����

#define OV2640_SENSOR_COM22      0x4B    // ͨ�ÿ���22 �����/STROBE����
                                         
#define OV2640_SENSOR_COM25      0x4E    // ͨ�ÿ���25

#define OV2640_SENSOR_BD50       0x4F    // 50HzƵ������ ����50Hz�е��µ��ع�ʱ��
                                         
#define OV2640_SENSOR_BD60       0x50    // 60HzƵ������ ����60Hz�е��µ��ع�ʱ��
                                         
#define OV2640_SENSOR_REG5D      0x5D    // �Ĵ���5D

#define OV2640_SENSOR_REG5E      0x5E    // �Ĵ���5E

#define OV2640_SENSOR_REG5F      0x5F    // �Ĵ���5F

#define OV2640_SENSOR_REG60      0x60    // �Ĵ���60

#define OV2640_SENSOR_HISTO_LOW  0x61    // ֱ��ͼ���ֽڣ�ֻ���� ͳ��ͼ�����ȷֲ�
                                        
#define OV2640_SENSOR_HISTO_HIGH 0x62    // ֱ��ͼ���ֽڣ�ֻ���� ͳ��ͼ�����ȷֲ�
                                         
uint8_t ov2640_read_reg(uint16_t reg);
uint8_t ov2640_write_reg(uint16_t reg, uint8_t data);
uint8_t ov2640_init(void);
void ov2640_jpeg_mode(void);
void ov2640_rgb565_mode(void);
uint8_t ov2640_outsize_set(uint16_t width, uint16_t height);
uint8_t ov2640_image_win_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);
uint8_t ov2640_imagesize_set(uint16_t width, uint16_t height);
uint8_t ov2640_window_set(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);

#endif
