#include "max30102_control.h"

uint8_t max30102_Bus_Write(uint8_t Register_Address, uint8_t Word_Data)
{

	/* ���ô���EEPROM�漴��ȡָ�����У�������ȡ�����ֽ� */

	/* ��1��������I2C���������ź� */
	MAX30102_IIC_Start();

	/* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_WR); /* �˴���дָ�� */

	/* ��3��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��4���������ֽڵ�ַ */
	MAX30102_IIC_Send_Byte(Register_Address);
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��5������ʼд������ */
	MAX30102_IIC_Send_Byte(Word_Data);

	/* ��6��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();
	return 1; /* ִ�гɹ� */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();
	return 0;
}

uint8_t max30102_Bus_Read(uint8_t Register_Address)
{
	uint8_t data;

	/* ��1��������I2C���������ź� */
	MAX30102_IIC_Start();

	/* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_WR); /* �˴���дָ�� */

	/* ��3��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��4���������ֽڵ�ַ�� */
	MAX30102_IIC_Send_Byte((uint8_t)Register_Address);
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
	MAX30102_IIC_Start();

	/* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_RD); /* �˴��Ƕ�ָ�� */

	/* ��8��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��9������ȡ���� */
	{
		data = MAX30102_IIC_Read_Byte(0); /* ��1���ֽ� */

		MAX30102_IIC_NAck(); /* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
	}
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();
	return data; /* ִ�гɹ� ����dataֵ */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();
	return 0;
}

void max30102_FIFO_ReadWords(uint8_t Register_Address, uint16_t Word_Data[][2], uint8_t count)
{
	uint8_t i = 0;
	uint8_t no = count;
	uint8_t data1, data2;
	/* ��1��������I2C���������ź� */
	MAX30102_IIC_Start();

	/* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_WR); /* �˴���дָ�� */

	/* ��3��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��4���������ֽڵ�ַ�� */
	MAX30102_IIC_Send_Byte((uint8_t)Register_Address);
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
	MAX30102_IIC_Start();

	/* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_RD); /* �˴��Ƕ�ָ�� */

	/* ��8��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��9������ȡ���� */
	while (no)
	{
		data1 = MAX30102_IIC_Read_Byte(0);
		MAX30102_IIC_Ack();
		data2 = MAX30102_IIC_Read_Byte(0);
		MAX30102_IIC_Ack();
		Word_Data[i][0] = (((uint16_t)data1 << 8) | data2); //

		data1 = MAX30102_IIC_Read_Byte(0);
		MAX30102_IIC_Ack();
		data2 = MAX30102_IIC_Read_Byte(0);
		if (1 == no)
			MAX30102_IIC_NAck(); /* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
		else
			MAX30102_IIC_Ack();
		Word_Data[i][1] = (((uint16_t)data1 << 8) | data2);

		no--;
		i++;
	}
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();
}

void max30102_FIFO_ReadBytes(uint8_t Register_Address, uint8_t *Data)
{
	max30102_Bus_Read(REG_INTR_STATUS_1);	//��ȡ����� FIFO��PPG ���жϱ�־
	max30102_Bus_Read(REG_INTR_STATUS_2);	//��ȡ����� �¶ȡ�������������жϱ�־

	/* ��1��������I2C���������ź� */
	MAX30102_IIC_Start();

	/* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_WR); /* �˴���дָ�� */

	/* ��3��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��4���������ֽڵ�ַ�� */
	MAX30102_IIC_Send_Byte((uint8_t)Register_Address);
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
	MAX30102_IIC_Start();

	/* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
	MAX30102_IIC_Send_Byte(max30102_WR_address | I2C_RD); /* �˴��Ƕ�ָ�� */

	/* ��8��������ACK */
	if (MAX30102_IIC_Wait_Ack() != 0)
	{
		goto cmd_fail; /* EEPROM������Ӧ�� */
	}

	/* ��9������ȡ���� */
	Data[0] = MAX30102_IIC_Read_Byte(1);
	Data[1] = MAX30102_IIC_Read_Byte(1);
	Data[2] = MAX30102_IIC_Read_Byte(1);
	Data[3] = MAX30102_IIC_Read_Byte(1);
	Data[4] = MAX30102_IIC_Read_Byte(1);
	Data[5] = MAX30102_IIC_Read_Byte(0);
	/* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
	/* ����I2C����ֹͣ�ź� */
	MAX30102_IIC_Stop();

	//	uint8_t i;
	//	uint8_t fifo_wr_ptr;
	//	uint8_t firo_rd_ptr;
	//	uint8_t number_tp_read;
	//	//Get the FIFO_WR_PTR
	//	fifo_wr_ptr = max30102_Bus_Read(REG_FIFO_WR_PTR);
	//	//Get the FIFO_RD_PTR
	//	firo_rd_ptr = max30102_Bus_Read(REG_FIFO_RD_PTR);
	//
	//	number_tp_read = fifo_wr_ptr - firo_rd_ptr;
	//
	//	//for(i=0;i<number_tp_read;i++){
	//	if(number_tp_read>0){
	//		MAX30102_IIC_ReadBytes(max30102_WR_address,REG_FIFO_DATA,Data,6);
	//	}

	// max30102_Bus_Write(REG_FIFO_RD_PTR,fifo_wr_ptr);
}

void MAX30102_Reset(void)
{
	max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
	max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
}

void MAX30102_Init(void)
{

	//将INT引脚设置为输入模式，用于通过中断获取INT引脚状态
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitStructure.Pin = MAX30102_INT_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(MAX30102_INT_PORT, &GPIO_InitStructure);

	MAX30102_IIC_Init();

	MAX30102_Reset();

	max30102_Bus_Write(REG_INTR_ENABLE_1, 0xc0); // INTR setting
	max30102_Bus_Write(REG_INTR_ENABLE_2, 0x00);
	max30102_Bus_Write(REG_FIFO_WR_PTR, 0x00); // FIFO_WR_PTR[4:0]
	max30102_Bus_Write(REG_OVF_COUNTER, 0x00); // OVF_COUNTER[4:0]
	max30102_Bus_Write(REG_FIFO_RD_PTR, 0x00); // FIFO_RD_PTR[4:0]
	max30102_Bus_Write(REG_FIFO_CONFIG, 0x0f); // sample avg = 1, fifo rollover=false, fifo almost full = 17
	max30102_Bus_Write(REG_MODE_CONFIG, 0x03); // 0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
	max30102_Bus_Write(REG_SPO2_CONFIG, 0x27); // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS)
	max30102_Bus_Write(REG_LED1_PA, 0x3F);	   // 增加LED强度到最大值 (~25mA)
	max30102_Bus_Write(REG_LED2_PA, 0x3F);	   // 增加LED强度到最大值 (~25mA)
	max30102_Bus_Write(REG_PILOT_PA, 0x7f);	   // Choose value for ~ 25mA for Pilot LED
}

void maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
{
	//  char ach_i2c_data[2];
	//  ach_i2c_data[0]=uch_addr;
	//  ach_i2c_data[1]=uch_data;
	//
	//  MAX30102_IIC_WriteBytes(I2C_WRITE_ADDR, ach_i2c_data, 2);
	MAX30102_IIC_Write_One_Byte(I2C_WRITE_ADDR, uch_addr, uch_data);
}

void maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
{
	//  char ch_i2c_data;
	//  ch_i2c_data=uch_addr;
	//  MAX30102_IIC_WriteBytes(I2C_WRITE_ADDR, &ch_i2c_data, 1);
	//
	//  i2c.read(I2C_READ_ADDR, &ch_i2c_data, 1);
	//
	//   *puch_data=(uint8_t) ch_i2c_data;
	MAX30102_IIC_Read_One_Byte(I2C_WRITE_ADDR, uch_addr, puch_data);
}

void maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
	uint32_t un_temp;
	unsigned char uch_temp;
	char ach_i2c_data[6];
	*pun_red_led = 0;
	*pun_ir_led = 0;

	// read and clear status register
	maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
	maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);

	MAX30102_IIC_ReadBytes(I2C_WRITE_ADDR, REG_FIFO_DATA, (uint8_t *)ach_i2c_data, 6);

	un_temp = (unsigned char)ach_i2c_data[0];
	un_temp <<= 16;
	*pun_red_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[1];
	un_temp <<= 8;
	*pun_red_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[2];
	*pun_red_led += un_temp;

	un_temp = (unsigned char)ach_i2c_data[3];
	un_temp <<= 16;
	*pun_ir_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[4];
	un_temp <<= 8;
	*pun_ir_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[5];
	*pun_ir_led += un_temp;
	*pun_red_led &= 0x03FFFF; // Mask MSB [23:18]
	*pun_ir_led &= 0x03FFFF;  // Mask MSB [23:18]
}

