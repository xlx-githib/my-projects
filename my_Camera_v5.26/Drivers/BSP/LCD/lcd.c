#include "lcd.h"
#include "lcdfont.h"
#include "usart.h"

/* LCD的画笔颜色和背景色 */
extern uint32_t g_point_color = 0xF800;    /* 画笔颜色 */
extern uint32_t g_back_color  = 0xFFFF;    /* 背景色 */

_lcd_dev lcddev;

void lcd_wr_data(volatile uint16_t data)
{
	data = data;//强制编译器访问data 消耗1~2CPU周期，高效，防止LCD连续写两次
	LCD->LCD_RAM = data;
}

void lcd_wr_regno(volatile uint16_t regno)
{
	regno = regno;//强制编译器访问regno 消耗1~2CPU周期，高效，防止LCD连续写两次
	LCD->LCD_REG = regno;
}

void lcd_write_reg(uint16_t regno, uint16_t data)
{
	LCD->LCD_REG = regno;
	LCD->LCD_RAM = data;
}

static void lcd_opt_delay(uint32_t i)//防止被编译器优化
{
    while (i--); 
}


static uint16_t lcd_rd_data(void)
{
    volatile uint16_t ram;  /* 防止被优化 */
    lcd_opt_delay(2);
    ram = LCD->LCD_RAM;
    return ram;
}

void lcd_write_ram_prepare(void)//准备写GRAM
{
    LCD->LCD_REG = lcddev.wramcmd;
}

void lcd_display_on(void)//LCD开启显示
{
    lcd_wr_regno(0x29);     /* 开启显示 */
}

void lcd_display_off(void)//LCD关闭显示
{
    lcd_wr_regno(0x28);     /* 关闭显示 */
}

void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t dirreg = 0;
    uint16_t temp;

    /* 根据扫描方式 设置 0x36/0x3600 寄存器 bit 5,6,7 位的值 */
    switch (dir)
    {
        case L2R_U2D:   /* 从左到右,从上到下 */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:   /* 从左到右,从下到上 */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:   /* 从右到左,从上到下 */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:   /* 从右到左,从下到上 */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:   /* 从上到下,从左到右 */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:   /* 从上到下,从右到左 */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:   /* 从下到上,从左到右 */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:   /* 从下到上,从右到左 */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0x36;  /* 对绝大部分驱动IC, 由0x36寄存器控制 */

     /* 9341 & 7789 & 7796 要设置BGR位 */
    if (lcddev.id == 0x9341)
    {
        regval |= 0x08;
    }

    lcd_write_reg(dirreg, regval);

    if (lcddev.id != 0x1963)                    /* 1963不做坐标处理 */
    {
        if (regval & 0x20)
        {
            if (lcddev.width < lcddev.height)   /* 交换X,Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
        else
        {
            if (lcddev.width > lcddev.height)   /* 交换X,Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
    }

    /* 设置显示区域(开窗)大小 */
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.width - 1) >> 8);
    lcd_wr_data((lcddev.width - 1) & 0xFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.height - 1) >> 8);
    lcd_wr_data((lcddev.height - 1) & 0xFF);
}

void lcd_display_dir(uint8_t dir)//显示方向
{
    lcddev.dir = dir;   /* 竖屏/横屏 */

    if (dir == 0)       /* 竖屏 */
    {
        lcddev.width = 240;
        lcddev.height = 320;			
				lcddev.wramcmd = 0x2C;
        lcddev.setxcmd = 0x2A;
        lcddev.setycmd = 0x2B;

    }
    else        /* 横屏 */
    {
        lcddev.width = 320;         /* 默认宽度 */
        lcddev.height = 240;        /* 默认高度 */
				lcddev.wramcmd = 0x2C;
        lcddev.setxcmd = 0x2A;
        lcddev.setycmd = 0x2B;
    }

    lcd_scan_dir(DFT_SCAN_DIR);     /* 默认扫描方向 */
}

void lcd_init(void)
{
		//printf("Start lcd_init.\r\n");
    /* 尝试9341 ID的读取 */
    lcd_wr_regno(0xD3);
    lcddev.id = lcd_rd_data();  /* dummy read */
    lcddev.id = lcd_rd_data();  /* 读到0x00 */
    lcddev.id = lcd_rd_data();  /* 读取93 */
    lcddev.id <<= 8;
    lcddev.id |= lcd_rd_data(); /* 读取41 */


    /* 特别注意, 如果在main函数里面屏蔽串口1初始化, 则会卡死在printf
     * 里面(卡死在f_putc函数), 所以, 必须初始化串口1, 或者屏蔽掉下面
     * 这行 printf 语句 !!!!!!!
     */
    printf("LCD ID:%x\r\n", lcddev.id); /* 打印LCD ID */
		
    if (lcddev.id == 0x9341)
    {
        lcd_ex_ili9341_reginit();   /* 执行ILI9341初始化 */
    }
    
    lcd_display_dir(0); /* 默认为竖屏 */
    LCD_BL(1);          /* 点亮背光 */
    lcd_clear(WHITE);
}

void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t twidth, theight;
    twidth = sx + width - 1;
    theight = sy + height - 1;

   
   if (lcddev.id == 0x9341 && lcddev.dir != 1)     /* 1963竖屏特殊处理 */
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(sx >> 8);
        lcd_wr_data(sx & 0xFF);
        lcd_wr_data(twidth >> 8);
        lcd_wr_data(twidth & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(sy >> 8);
        lcd_wr_data(sy & 0xFF);
        lcd_wr_data(theight >> 8);
        lcd_wr_data(theight & 0xFF);
    }
    
}

void lcd_set_cursor(uint16_t x, uint16_t y)
{
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(x >> 8);
    lcd_wr_data(x & 0xFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(y >> 8);
    lcd_wr_data(y & 0xFF);
}

void lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;

    totalpoint *= lcddev.height;    /* 得到总点数 */
    lcd_set_cursor(0x00, 0x0000);   /* 设置光标位置 */
    lcd_write_ram_prepare();        /* 开始写入GRAM */

    for (index = 0; index < totalpoint; index++)
    {
        LCD->LCD_RAM = color;//LCD控制器的自动地址递增特性
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_cursor(x, y);       /* 设置光标位置 */
    lcd_write_ram_prepare();    /* 开始写入GRAM */
    LCD->LCD_RAM = color;
}

static uint32_t lcd_pow(uint8_t m, uint8_t n)//m的n次方
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result;
}

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';    /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[chr];  /* 调用1206字体 */
            break;
        case 16:
            pfont = (uint8_t *)asc2_1608[chr];  /* 调用1608字体 */
            break;
        case 24:
            pfont = (uint8_t *)asc2_2412[chr];  /* 调用2412字体 */
            break;
        case 32:
            pfont = (uint8_t *)asc2_3216[chr];  /* 调用3216字体 */
            break;
        default:
            return ;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];                            /* 获取字符的点阵数据 */

        for (t1 = 0; t1 < 8; t1++)                  /* 一个字节8个点 */
        {
            if (temp & 0x80)                        /* 有效点,需要显示 */
            {
                lcd_draw_point(x, y, color);        /* 画点出来,要显示这个点 */
            }
            else if (mode == 0)                     /* 无效点,不显示 */
            {
                lcd_draw_point(x, y, g_back_color); /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }

            temp <<= 1;                             /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= lcddev.height)return;          /* 超区域了 */

            if ((y - y0) == size)                   /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= lcddev.width)
                {
                    return;       /* x坐标超区域了 */
                }

                break;
            }
        }
    }
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))               /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2) * t, y, ' ', size, 0, color);  /* 显示空格,占位 */
                continue;       /* 继续下个一位 */
            }
            else
            {
                enshow = 1;     /* 使能显示 */
            }
        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, 0, color);   /* 显示字符 */
    }
}

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)       /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;    /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                if (mode & 0x80)    /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size / 2) * t, y, '0', size, mode & 0x01, color);    /* 用0占位 */
                }
                else
                {
                    lcd_show_char(x + (size / 2) * t, y, ' ', size, mode & 0x01, color);    /* 用空格占位 */
                }

                continue;
            }
            else
            {
                enshow = 1;         /* 使能显示 */
            }

        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, mode & 0x01, color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
        {
            break;      /* 退出 */
        }

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

void lcd_ex_ili9341_reginit(void)
{
    /* ==================== 电源管理相关 ==================== */
    
    /* 0xCF - 电源控制B (Power Control B)
     * 控制内部电源电路的放电时间和驱动能力
     */
    lcd_wr_regno(0xCF);     // 选中电源控制B寄存器
    lcd_wr_data(0x00);      // 保留位，必须写0
    lcd_wr_data(0xC1);      // 电源驱动能力设置：1100_0001，设置内部供电驱动强度
    lcd_wr_data(0x30);      // 放电时间：0011_0000，约5ms放电保护时间

    /* 0xED - 电源序列控制 (Power Sequence Control)
     * 控制多路电源轨（VGH、VGL、VCOM）的上电顺序和软启动时间
     * 必须按特定顺序启动各电压轨，否则可能损坏液晶
     */
    lcd_wr_regno(0xED);     // 选中电源序列寄存器
    lcd_wr_data(0x64);      // 软启动时间0x64，让电压缓慢上升到目标值
    lcd_wr_data(0x03);      // 序列步骤1：先启动VGL（负压）
    lcd_wr_data(0x12);      // 序列步骤2：再启动VGH（正压）
    lcd_wr_data(0x81);      // 序列步骤3：最后启动VCOM（公共电极电压）

    /* 0xE8 - 驱动时序控制 (Driver Timing Control)
     * 设置LCD面板栅极(Gate)和源极(Source)驱动器的时序参数
     * 确保像素数据在正确的时间写入正确的位置
     */
    lcd_wr_regno(0xE8);     // 选中驱动时序寄存器
    lcd_wr_data(0x85);      // 栅极驱动时序：行扫描时钟=0x85，控制行切换速度
    lcd_wr_data(0x10);      // 源极驱动时序：列数据时钟=0x10，控制列数据锁存时机
    lcd_wr_data(0x7A);      // 时序微调参数0x7A，补偿信号传输延迟

    /* 0xCB - VCOM控制1 (VCOM Control 1)
     * VCOM(Voltage Common)是液晶公共电极电压
     * 设置直流偏置和交流幅度，直接影响显示对比度和闪烁
     */
    lcd_wr_regno(0xCB);     // 选中VCOM控制寄存器
    lcd_wr_data(0x39);      // VCOM电压高阈值0x39，正极性时的参考电压
    lcd_wr_data(0x2C);      // VCOM电压低阈值0x2C，负极性时的参考电压
    lcd_wr_data(0x00);      // 保留位
    lcd_wr_data(0x34);      // VCOM偏移量0x34，微调电压中心值
    lcd_wr_data(0x02);      // VCOM使能和模式：0x02=使能VCOM自动调整

    /* 0xF7 - 升压比率控制 (Pump Ratio Control)
     * 控制内部电荷泵的升压倍数
     * 将3.3V供电升高到驱动液晶需要的10V以上电压
     */
    lcd_wr_regno(0xF7);     // 选中升压比率寄存器
    lcd_wr_data(0x20);      // 升压倍率0x20=2倍升压，3.3V→约6.6V后再倍压

    /* 0xEA - 内部电源控制 (Internal Power Control)
     * 微调内部参考电压源和电源管理细节
     */
    lcd_wr_regno(0xEA);     // 选中内部电源寄存器
    lcd_wr_data(0x00);      // 内部参考电压设置：默认值
    lcd_wr_data(0x00);      // 电源管理参数：默认模式

    /* 0xC0 - 电源控制1 (Power Control 1)
     * 设置VRH基准参考电压
     * VRH影响Gamma校正和亮度曲线
     */
    lcd_wr_regno(0xC0);     // 选中电源控制1寄存器
    lcd_wr_data(0x1B);      // VRH[5:0]=0x1B=27，VRH=4.65V(典型值)
                            // 决定了"最亮"的电压上限

    /* 0xC1 - 电源控制2 (Power Control 2)
     * 设置SAP(源极放大器偏置)和BT(升压频率)
     */
    lcd_wr_regno(0xC1);     // 选中电源控制2寄存器
    lcd_wr_data(0x01);      // SAP[2:0]=001：放大器偏置电流最小档
                            // BT[3:0]=0001：升压频率选择，平衡效率和噪声

    /* 0xC5 - VCM控制 (VCM Control)
     * 设置VCOM电压变化速率和稳定时间
     * 切换太快→噪声，太慢→残影
     */
    lcd_wr_regno(0xC5);     // 选中VCM控制寄存器
    lcd_wr_data(0x30);      // VCM速率控制0x30，中等变化速度
    lcd_wr_data(0x30);      // VCM稳定时间0x30，等待电压稳定后再刷新

    /* 0xC7 - VCM控制2 (VCM Control 2)
     * VCOM电压额外偏置微调
     */
    lcd_wr_regno(0xC7);     // 选中VCM控制2寄存器
    lcd_wr_data(0xB7);      // VCM偏置0xB7，进一步优化闪烁的参数


    /* ==================== 显示模式设置 ==================== */

    /* 0x36 - 内存访问控制 (Memory Access Control) [关键寄存器]
     * 控制屏幕扫描方向和RGB/BGR顺序
     * bit7=MY(行方向) bit6=MX(列方向) bit5=MV(行列交换)
     * bit4=ML(垂直刷新) bit3=BGR(颜色顺序) bit2=MH(水平刷新)
     */
    lcd_wr_regno(0x36);     // 选中内存访问控制寄存器
    lcd_wr_data(0x48);      // 0x48=0100_1000
                            // MY=0：行从上到下递增
                            // MX=1：列从左到右递增
                            // MV=0：不交换行列(竖屏模式)
                            // ML=0：垂直刷新从上到下
                            // BGR=1：颜色数据为BGR顺序(先蓝后红)
                            // MH=0：水平刷新从左到右

    /* 0x3A - 像素格式设置 (Pixel Format Set) [关键寄存器]
     * 设置每个像素的色深位数
     * 0x50=RGB555, 0x55=RGB565(16位), 0x66=RGB666(18位)
     */
    lcd_wr_regno(0x3A);     // 选中像素格式寄存器
    lcd_wr_data(0x55);      // 0x55=RGB565(16位色深)
                            // R:5位(bit15~11) G:6位(bit10~5) B:5位(bit4~0)
                            // 必须与OV2640输出格式一致


    /* ==================== 帧速率控制 ==================== */

    /* 0xB1 - 帧速率控制 (Frame Rate Control)
     * 设置屏幕刷新率，控制一秒刷新多少次
     * 太低→闪烁，太高→功耗大
     */
    lcd_wr_regno(0xB1);     // 选中帧速率寄存器
    lcd_wr_data(0x00);      // 时钟分频比=0，不分频
    lcd_wr_data(0x1A);      // 帧频率0x1A，约70Hz刷新率
                            // 70Hz=每秒刷新70次，人眼不闪烁的最佳平衡点

    /* 0xB6 - 显示功能控制 (Display Function Control)
     * 设置驱动信号的电压范围和驱动模式
     */
    lcd_wr_regno(0xB6);     // 选中显示功能寄存器
    lcd_wr_data(0x0A);      // 栅极驱动电压范围0x0A
    lcd_wr_data(0xA2);      // 源极驱动电压范围0xA2
                            // 确保驱动信号刚好让液晶完全翻转又不损坏


    /* ==================== Gamma校正 ==================== */

    /* 0xF2 - Gamma功能控制
     * 开启/关闭Gamma(灰度)校正
     */
    lcd_wr_regno(0xF2);     // 选中Gamma功能寄存器
    lcd_wr_data(0x00);      // 0x00=使能Gamma校正
                            // Gamma校正让亮度变化更均匀，暗部细节不丢失

    /* 0x26 - Gamma曲线选择
     * 选择预设的Gamma响应曲线
     */
    lcd_wr_regno(0x26);     // 选中Gamma曲线寄存器
    lcd_wr_data(0x01);      // 选择曲线1(共4条可选)，ILI9341标准推荐

    /* 0xE0 - 正极性Gamma校正微调
     * 像素电压高于VCOM时(正极性)的15级灰阶电压调节
     * 液晶需要正负交替驱动防止老化
     */
    lcd_wr_regno(0xE0);     // 选中正极性Gamma寄存器
    lcd_wr_data(0x0F);      // VP1：最暗灰阶参考电压
    lcd_wr_data(0x2A);      // VP2
    lcd_wr_data(0x28);      // VP3
    lcd_wr_data(0x08);      // VP4
    lcd_wr_data(0x0E);      // VP5
    lcd_wr_data(0x08);      // VP6
    lcd_wr_data(0x54);      // VP7
    lcd_wr_data(0xA9);      // VP8
    lcd_wr_data(0x43);      // VP9
    lcd_wr_data(0x0A);      // VP10
    lcd_wr_data(0x0F);      // VP11
    lcd_wr_data(0x00);      // VP12
    lcd_wr_data(0x00);      // VP13
    lcd_wr_data(0x00);      // VP14
    lcd_wr_data(0x00);      // VP15：最亮灰阶参考电压
                            // 15个点精确校准从黑到白的过渡曲线

    /* 0xE1 - 负极性Gamma校正微调
     * 像素电压低于VCOM时(负极性)的15级灰阶电压调节
     */
    lcd_wr_regno(0xE1);     // 选中负极性Gamma寄存器
    lcd_wr_data(0x00);      // VN1
    lcd_wr_data(0x15);      // VN2
    lcd_wr_data(0x17);      // VN3
    lcd_wr_data(0x07);      // VN4
    lcd_wr_data(0x11);      // VN5
    lcd_wr_data(0x06);      // VN6
    lcd_wr_data(0x2B);      // VN7
    lcd_wr_data(0x56);      // VN8
    lcd_wr_data(0x3C);      // VN9
    lcd_wr_data(0x05);      // VN10
    lcd_wr_data(0x10);      // VN11
    lcd_wr_data(0x0F);      // VN12
    lcd_wr_data(0x3F);      // VN13
    lcd_wr_data(0x3F);      // VN14
    lcd_wr_data(0x0F);      // VN15
                            // 正负Gamma对称校准，保证256级灰阶均匀


    /* ==================== 显示区域设置 ==================== */

    /* 0x2B - 行地址设置 (Page Address Set)
     * 设置LCD垂直显示范围(行数)
     * ILI9341物理分辨率：240×320
     */
    lcd_wr_regno(0x2B);     // 选中行地址寄存器
    lcd_wr_data(0x00);      // 起始行高字节=0
    lcd_wr_data(0x00);      // 起始行低字节=0，从第0行开始
    lcd_wr_data(0x01);      // 结束行高字节=1
    lcd_wr_data(0x3F);      // 结束行低字节=0x3F
                            // 结束行=0x013F=319，共320行
                            // 显示范围：0~319行

    /* 0x2A - 列地址设置 (Column Address Set)
     * 设置LCD水平显示范围(列数)
     */
    lcd_wr_regno(0x2A);     // 选中列地址寄存器
    lcd_wr_data(0x00);      // 起始列高字节=0
    lcd_wr_data(0x00);      // 起始列低字节=0，从第0列开始
    lcd_wr_data(0x00);      // 结束列高字节=0
    lcd_wr_data(0xEF);      // 结束列低字节=0xEF
                            // 结束列=0xEF=239，共240列
                            // 显示范围：0~239列


    /* ==================== 启动显示 ==================== */

    /* 0x11 - 退出睡眠模式 (Sleep Out) [关键命令]
     * ILI9341上电默认处于睡眠省电状态
     * 必须发送此命令退出睡眠，屏幕才能显示
     * 退出后需等待120ms让内部电路完全启动
     */
    lcd_wr_regno(0x11);     // 发送退出睡眠命令
    delay_ms(120);          // 必须等待120ms！
                            // 内部电源、振荡器、升压电路在此时间内启动
                            // 时间不够→显示异常或黑屏

    /* 0x29 - 显示开启 (Display ON) [关键命令]
     * 所有配置完成后打开显示输出
     * 发送此命令后屏幕开始显示画面
     */
    lcd_wr_regno(0x29);     // 发送显示开启命令
                            // 屏幕正式亮起开始工作
}

