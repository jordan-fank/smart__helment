#include "OLED.h"

#include "OLED_Font.h"

/*引脚配置*/
#define SCL_Pin GPIO_PIN_6
#define SCL_GPIO_Port GPIOA
#define SDA_Pin GPIO_PIN_7
#define SDA_GPIO_Port GPIOA
#define OLED_W_SCL(x) HAL_GPIO_WritePin(GPIOA, SCL_Pin, (GPIO_PinState)(x))
#define OLED_W_SDA(x) HAL_GPIO_WritePin(GPIOA, SDA_Pin, (GPIO_PinState)(x))

/*引脚初始化*/
void OLED_I2C_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, SCL_Pin | SDA_Pin, GPIO_PIN_SET);

	/*Configure GPIO pins : PBPin PBPin */
	GPIO_InitStruct.Pin = SCL_Pin | SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
 * @brief  I2C开始
 * @param  无
 * @retval 无
 */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
 * @brief  I2C停止
 * @param  无
 * @retval 无
 */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
 * @brief  I2C发送一个字节
 * @param  Byte 要发送的一个字节
 * @retval 无
 */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SDA(1);//释放SDA总线
	OLED_W_SCL(1); //额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
 * @brief  OLED写命令
 * @param  Command 要写入的命令
 * @retval 无
 */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78); //从机地址
	OLED_I2C_SendByte(0x00); //写命令
	OLED_I2C_SendByte(Command);
	OLED_I2C_Stop();
}

/**
 * @brief  OLED写数据
 * @param  Data 要写入的数据
 * @retval 无
 */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78); //从机地址
	OLED_I2C_SendByte(0x40); //写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
 * @brief  OLED设置光标位置
 * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
 * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
 * @retval 无
 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);				 //设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4)); //设置X位置低4位
	OLED_WriteCommand(0x00 | (X & 0x0F));		 //设置X位置高4位
}

/**
 * @brief  OLED清屏
 * @param  无
 * @retval 无
 */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for (i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
 * @brief  OLED部分清屏
 * @param  Line 行位置，范围：1~4
 * @param  start 列开始位置，范围：1~16
 * @param  end 列开始位置，范围：1~16
 * @retval 无
 */
void OLED_Clear_Part(uint8_t Line, uint8_t start, uint8_t end)
{
	uint8_t i, Column;
	for (Column = start; Column <= end; Column++)
	{
		OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8); //设置光标位置在上半部分
		for (i = 0; i < 8; i++)
		{
			OLED_WriteData(0x00); //显示上半部分内容
		}
		OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); //设置光标位置在下半部分
		for (i = 0; i < 8; i++)
		{
			OLED_WriteData(0x00); //显示下半部分内容
		}
	}
}

/**
 * @brief  OLED显示一个字符
 * @param  Line 行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  Char 要显示的一个字符，范围：ASCII可见字符
 * @retval 无
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8); //设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]); //显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); //设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]); //显示下半部分内容
	}
}

/**
 * @brief  OLED显示字符串
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  String 要显示的字符串，范围：ASCII可见字符
 * @retval 无
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED显示一个中文字
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Chinese 要显示的中文字在字库数组中的位置
  * @retval 无
  */
void OLED_ShowWord(uint8_t Line, uint8_t Column, uint8_t Chinese)
{       
    uint8_t i;
    // 1. 左上角 (0-7)
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F16x16[Chinese][i]);    
    }

    // 2. 右上角 (8-15) -> 修正：i从0开始
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8 + 8);
    for (i = 0; i < 8; i++) 
    {
        OLED_WriteData(OLED_F16x16[Chinese][i+8]);    
    }

    // 3. 左下角 (16-23)
    OLED_SetCursor((Line - 1) * 2 +1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F16x16[Chinese][i+16]);
    }

    // 4. 右下角 (24-31) -> 修正：i从0开始
    OLED_SetCursor((Line - 1) * 2 +1, (Column - 1) * 8 + 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F16x16[Chinese][i+24]); // 注意这里偏移是 16+8=24
    }
}
/**
  * @brief  OLED显示一串中文字
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Chinese[] 要显示的中文字在字库数组中的位置，数组里放每个字的位置
  * @param	Length 要显示中文的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t *Chinese,uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowWord(Line, Column + i*2,Chinese[i]);
	}
}


/**
 * @brief  OLED次方函数
 * @retval 返回值等于X的Y次方
 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
 * @brief  OLED显示数字（十进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~4294967295
 * @param  Length 要显示数字的长度，范围：1~10
 * @retval 无
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
 * @brief  OLED显示数字（十进制，带符号数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：-2147483648~2147483647
 * @param  Length 要显示数字的长度，范围：1~10
 * @retval 无
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
 * @brief  OLED显示数字（十六进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
 * @param  Length 要显示数字的长度，范围：1~8
 * @retval 无
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
 * @brief  OLED显示数字（二进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
 * @param  Length 要显示数字的长度，范围：1~16
 * @retval 无
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
 * @brief  OLED初始化
 * @param  无
 * @retval 无
 */
void OLED_Init(void)
{
	uint32_t i, j;

	for (i = 0; i < 1000; i++) //上电延时
	{
		for (j = 0; j < 1000; j++)
			;
	}

	//OLED_I2C_Init(); //端口初始化

	OLED_WriteCommand(0xAE); //关闭显示

	OLED_WriteCommand(0xD5); //设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);

	OLED_WriteCommand(0xA8); //设置多路复用率
	OLED_WriteCommand(0x3F);

	OLED_WriteCommand(0xD3); //设置显示偏移
	OLED_WriteCommand(0x00);

	OLED_WriteCommand(0x40); //设置显示开始行

	OLED_WriteCommand(0xA1); //设置左右方向，0xA1正常 0xA0左右反置

	OLED_WriteCommand(0xC8); //设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA); //设置COM引脚硬件配置
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81); //设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9); //设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB); //设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4); //设置整个显示打开/关闭

	OLED_WriteCommand(0xA6); //设置正常/倒转显示

	OLED_WriteCommand(0x8D); //设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF); //开启显示

	OLED_Clear(); // OLED清屏
}

/**
 * @brief  显示进度条 (无缓冲直接写屏版)
 * @param  Progress: 进度值 0~100
 * @retval 无
 */
void OLED_ShowProgress(uint8_t Progress)
{
    uint8_t i;
    uint8_t Bar_Start_X = 14;  // 进度条起始列 (让100宽度的条居中: (128-100)/2 = 14)
    uint8_t Bar_Width = 100;   // 进度条总宽度

    if(Progress > 100) Progress = 100;

    // 1. 设置光标到第5页 (Page 5) 的起始位置
    // 注意：你的 OLED_SetCursor 第一个参数是 Y(0-7)，第二个是 X(0-127)
    OLED_SetCursor(5, Bar_Start_X); 

    // 2. 绘制已加载的部分 (实心)
    for(i = 0; i < Progress; i++)
    {
        OLED_WriteData(0xFF); // 0xFF 代表 8 个竖着的像素全亮，形成实心条
    }

    // 3. 绘制未加载的部分 (轨道虚线)
    // 0x24 (二进制 00100100) 会显示出两条细细的横线作为轨道，很有科技感
    for(i = Progress; i < Bar_Width; i++)
    {
        OLED_WriteData(0x24); 
    }
}



/**
 * @brief  底层工具：绘制16x16汉字 (适配 PCtoLCD2002 标准逐列式输出)
 * @note   适配数据格式：[上0][下0][上1][下1]... (上下交替)
 */
void OLED_DrawCN_Block(uint8_t x, uint8_t y, const uint8_t *data)
{
    uint8_t i;
    
    // 1. 画上半截 (Page y)
    // 这里的规律是：0, 2, 4, 6... (取偶数下标)
    OLED_SetCursor(y, x);
    for(i=0; i<16; i++) 
    {
        OLED_WriteData(data[2*i]); // 只取偶数位置的数据 (0, 2, 4...)
    }

    // 2. 画下半截 (Page y+1)
    // 这里的规律是：1, 3, 5, 7... (取奇数下标)
    OLED_SetCursor(y+1, x);
    for(i=0; i<16; i++) 
    {
        OLED_WriteData(data[2*i + 1]); // 只取奇数位置的数据 (1, 3, 5...)
    }
}

/**
 * @brief  智能显示汉字串 (所见即所得版)
 * @param  x: 起始列坐标 (0-112)
 * @param  y: 起始页坐标 (0-6)
 * @param  str: 字符串，例如 "温度25℃"
 */
void OLED_ShowStr(uint8_t x, uint8_t y, char *str)
{
    uint8_t i;
    // 计算字库里一共有多少个字
    uint8_t hz_count = sizeof(Hzk) / sizeof(ChineseCell_t); 
    
    while(*str != '\0') // 遍历输入的字符串
    {
        // 换行保护
        if(x > 112) { x = 0; y += 2; }
        if(y > 6) break;

        // --- 核心逻辑：查表 ---
        uint8_t found = 0;
        
        // 1. 尝试在字典里找这个字
        for(i=0; i<hz_count; i++)
        {
            // 获取当前字典里汉字的长度 (防止不同编码长度不一)
            size_t type_len = strlen(Hzk[i].Index);
            
            // 比对：如果当前字符串的前几位 == 字典里的汉字
            if(strncmp(str, Hzk[i].Index, type_len) == 0)
            {
                // 找到了！画出来
                OLED_DrawCN_Block(x, y, Hzk[i].Data);
                
                // 移位
                str += type_len; // 字符串指针跳过这个汉字
                x += 16;         // 屏幕光标右移16像素
                found = 1;
                break;
            }
        }
        
        // 2. 如果字典里没找到 (可能是英文字符，也可能是还没取模的汉字)
        if(!found)
        {
            // 如果是常规ASCII字符 (英文/数字)
            // 注意：这里假设你有英文的字库数组，如果还是用你原来的ShowChar，请保留你原来的逻辑
            // 这里为了演示，假设是英文
            if((uint8_t)*str <= 127) 
            {
                // 调用你原来的英文显示函数 (注意你的ShowChar参数是Line, Column)
                // 这里为了方便，我们简单适配一下你的坐标系
                // y是页(0-7)，Line是(1-4)。 Line = y/2 + 1
                OLED_ShowChar(y/2 + 1, x/8 + 1, *str); 
                
                str++; // 跳过1个字节
                x += 8; // 英文宽8像素
            }
            else 
            {
                // 是未知的汉字，直接跳过，避免死循环
                str++; 
            }
        }
    }
}


/**
 * @brief  显示BMP图片 (适配 Image2Lcd 垂直扫描+低位在前)
 * @param  x0: 起始列 (0-127)
 * @param  y0: 起始页 (0-7)
 * @param  w:  图片宽度
 * @param  h:  图片高度 (必须是8的倍数)
 * @param  BMP: 图片数组
 */
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, const uint8_t *BMP)
{
    uint8_t x, y;
    uint8_t page_height = h / 8; // 计算占几页

    for (y = 0; y < page_height; y++)
    {
        OLED_SetCursor(y0 + y, x0); // 设置光标到当前页
        for (x = 0; x < w; x++)
        {
            // 数据索引 = 当前页 * 图片宽 + 当前列
            OLED_WriteData(BMP[y * w + x]);
        }
    }
}

