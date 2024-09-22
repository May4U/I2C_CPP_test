/************************** Dongguan-University of Technology -ACE**************************
 * @file bsp_i2c.cpp
 * @author Lann 梁健蘅 (rendezook@qq.com)
 * @brief 
 * @version v0.9
 * @date 2024-09-14
 *******************************************************************************************
 * @verbatim v0.9   支持硬件I2C
 * @verbatim
 * 
 * 
 * @copyright Copyright (c) 2024
 * 
************************** Dongguan-University of Technology -ACE***************************/
#include "bsp_i2c.hpp"

Bsp_HW_I2C_c *Bsp_HW_I2C_c::hw_i2c_instance_[I2C_DEVICE_CNT] = {nullptr};
uint8_t Bsp_HW_I2C_c::idx_ = 0;


/**
 * @brief I2C构造函数
 * 
 * @param I2C_Init_Config ：
 *          I2C_HandleTypeDef *i2c_handle;       // i2c handle
 *          uint8_t device_address;             // 设置写入数据的地址
 *          I2C_Work_Mode_e work_mode;       // 工作模式
 * @param hw_iic_callback 
 */
Bsp_HW_I2C_c::Bsp_HW_I2C_c(I2C_Init_Config_s *I2C_Init_Config ,
                            void (*hw_iic_callback)(Bsp_HW_I2C_c *I2C_Instance))
                            ://设置i2c实例和回调函数
                            i2c_handle_(I2C_Init_Config->i2c_handle),
                            device_address_(I2C_Init_Config->device_address),
                            work_mode_(I2C_Init_Config->work_mode),
                            hw_iic_callback(hw_iic_callback)
{
	//将当前实例加入指针数组
	hw_i2c_instance_[idx_++] = this;	
	if (idx_ >= I2C_DEVICE_CNT)
    {
        // 超过最大实例数，错误处理
        while (true)
        {
            
        }
    }
}

/**
 * @brief 硬件I2C发送数据
 * 
 * @param data 待发送的数据首地址指针
 * @param size 发送长度
 */
void Bsp_HW_I2C_c::HW_I2C_Transmit(uint8_t *data, uint16_t size)
{
    switch(this->work_mode_)
    {
    case I2C_BLOCK_MODE:
        HAL_I2C_Master_Transmit(this->i2c_handle_, this->device_address_, data, size, 100);//默认超时时间为100ms
    case I2C_IT_MODE:
        HAL_I2C_Master_Transmit_IT(this->i2c_handle_, this->device_address_, data, size);
    case I2C_DMA_MODE:
        HAL_I2C_Master_Transmit_DMA(this->i2c_handle_, this->device_address_, data, size);
    default:
        while (1)
            ; // 未知传输模式, 程序停止 
    }
}

/**
 * @brief 硬件I2C接收数据
 * 
 * @param data 接收数据的首地址指针
 * @param size 接收长度
 */
void Bsp_HW_I2C_c::HW_I2C_Receive(uint8_t * data, uint16_t size)
{
    // 初始化接收缓冲区地址以及接受长度, 用于中断回调函数
    this->rx_buffer = data;
    this->rx_len = size;

    switch(this->work_mode_)
    {
    case I2C_BLOCK_MODE:
        HAL_I2C_Master_Receive(this->i2c_handle_, this->device_address_, data, size, 100);//默认超时时间为100ms
    case I2C_IT_MODE:
        HAL_I2C_Master_Receive_IT(this->i2c_handle_, this->device_address_, data, size);
    case I2C_DMA_MODE:
        HAL_I2C_Master_Receive_DMA(this->i2c_handle_, this->device_address_, data, size);
    default:
        while (1)
            ; // 未知传输模式, 程序停止 
    }
}

/**
 * @brief I2C读取从机寄存器(内存),只支持阻塞模式,超时默认为1ms
 * 
 * @param mem_addr 要读取的从机内存地址,目前只支持8位地址
 * @param data 要读取或写入的数据首地址指针
 * @param size 要读取或写入的数据长度
 * @param mem8bit_flag 从机内存地址是否为8位
 */
void Bsp_HW_I2C_c::HW_I2CAccessMem(uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_Mem_Mode_e mem_mode,uint8_t mem8bit_flag)
{
    uint16_t bit_flag = mem8bit_flag ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;
    if (mem_mode == I2C_WRITE_MEM)
    {
        HAL_I2C_Mem_Write(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size, 100);
    } 
    else if (mem_mode == I2C_READ_MEM)
    {
        HAL_I2C_Mem_Read(this->i2c_handle_, this->device_address_, mem_addr, bit_flag, data, size, 100);
    }
    else
    {
        while (1)
            ; // 未知模式, 程序停止
    }
}

/*************************************************以下为回调函数************************************************************* */
/**
 * @brief 此回调函数会被HAL库两回调函数调用
 * 
 * @param hi2c 
 */
void Bsp_HW_I2C_c::Bsp_HW_I2C_Callback(I2C_HandleTypeDef *hi2c)
{
    // 如果是当前i2c硬件发出的complete,且dev_address和之前发起接收的地址相同,同时回到函数不为空, 则调用回调函数
    for (uint8_t i = 0; i < idx_; i++)
    {
        if (hw_i2c_instance_[i]->i2c_handle_ == hi2c && hi2c->Devaddress == hw_i2c_instance_[i]->device_address_)
        {
            if (hw_i2c_instance_[i]->hw_iic_callback != NULL) // 回调函数不为空
                hw_i2c_instance_[i]->hw_iic_callback(hw_i2c_instance_[i]);
            return;
        }
    }
}

/**
 * @brief IIC接收完成回调函数
 *
 * @param hi2c handle
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    //Bsp_HW_I2C_c::Bsp_HW_I2C_Callback(hi2c);
}

/**
 * @brief 内存访问回调函数,仅做形式上的封装,仍然使用HAL_I2C_MasterRxCpltCallback
 *
 * @param hi2c handle
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    //HAL_I2C_MasterRxCpltCallback(hi2c);
}


/**********************************以下为旧iic搬运的函数（其实可以用    HW_I2CAccessMem     代替）************************************ */
I2C_Result_t Bsp_HW_I2C_c::writeByte(uint8_t device_address, uint8_t register_address, uint8_t data)
{
	uint8_t d[2];

		/* Format array to send */
		d[0] = register_address;
		d[1] = data;

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 2, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
}

I2C_Result_t Bsp_HW_I2C_c::writeByte(uint8_t device_address, uint8_t data)
{
	uint8_t d[1];

		/* Format array to send */

		d[0] = data;

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 2, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
}

I2C_Result_t Bsp_HW_I2C_c::write2Bytes(uint8_t device_address, uint8_t register_address, uint16_t data)
{
	uint8_t d[3];

		/* Format array to send */
		d[0] = register_address;
		d[1] = (uint8_t)(data >> 8);
		d[2] = (uint8_t)(data & 255);

		/* Try to transmit via I2C_class */
		if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1), (uint8_t *)d, 3, 1000) != HAL_OK) {
			/* Check error */
			if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

			}

			/* Return error */
			return I2C_Result_Error;
		}

		/* Return OK */
		return I2C_Result_Ok;
}

I2C_Result_t Bsp_HW_I2C_c::readMultiBytes(uint8_t device_address, uint8_t register_address, uint8_t* data, uint16_t count)
{
	//if (HAL_I2C_Master_Transmit(hi2c, (uint8_t)device_address, &register_address, 1, 1000) != HAL_OK) {
	//device_address = 0x83;
	if (HAL_I2C_Master_Transmit(this->i2c_handle_, (device_address << 1) , &register_address, 1, 1000) != HAL_OK) {
		/* Check error */
		if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

		}

		/* Return error */
		return I2C_Result_Error;
	}

	/* Receive multiple byte */
	if (HAL_I2C_Master_Receive(this->i2c_handle_, (device_address << 1) + 1, data, count, 1000) != HAL_OK) {
		/* Check error */
		if (HAL_I2C_GetError(this->i2c_handle_) != HAL_I2C_ERROR_AF) {

		}

		/* Return error */
		return I2C_Result_Error;
	}

	/* Return OK */
	return I2C_Result_Ok;
}

uint8_t Bsp_HW_I2C_c::readOneByte(uint8_t device_address, uint8_t register_address)
{
	uint8_t data[1];
	if(readMultiBytes(device_address, register_address, data, 1) == I2C_Result_Error)
	{
		return 0;
	}
	return data[0];
}

