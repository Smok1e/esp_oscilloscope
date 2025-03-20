#include "freertos/FreeRTOS.h"

#include "esp_log.h"

#include <bit>
#include <ratio>

#include <Peripherals/INA226.hpp>

//========================================

void INA226::startup(
	i2c_master_bus_handle_t i2c_bus_handle,
	int i2c_freq,
	uint8_t device_address
)
{
	i2c_device_config_t device_config = {};
	device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	device_config.device_address = device_address;
	device_config.scl_speed_hz = i2c_freq;
	
	ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &device_config, &m_device_handle));
}

INA226::~INA226()
{
	ESP_ERROR_CHECK(i2c_master_bus_rm_device(m_device_handle));
}

//========================================

void INA226::reset()
{
	setConfiguration(ConfigurationFlags::RST);
}

INA226::MeasurementType INA226::readShuntVoltage()
{
	return static_cast<int16_t>(readRegister(Register::ShuntVoltage)) * static_cast<MeasurementType>(.000025);
}

INA226::MeasurementType INA226::readBusVoltage()
{
	return static_cast<int16_t>(readRegister(Register::BusVoltage)) * static_cast<MeasurementType>(.00125);
}

void INA226::setConfiguration(uint16_t flags)
{
	writeRegister(Register::Configuration, flags);
}

uint16_t INA226::getConfiguration()
{
	return readRegister(Register::Configuration);
}

void INA226::setAlertMode(uint16_t flags)
{
	writeRegister(Register::MaskEnable, flags);
}

uint16_t INA226::getAlertMode()
{
	return readRegister(Register::MaskEnable);
}

uint16_t INA226::getManufacturerID()
{
	return readRegister(Register::ManufacturerID);
}

uint16_t INA226::getDieID()
{
	return readRegister(Register::DieID);
}

//========================================

uint16_t INA226::readRegister(Register addr)
{
	uint16_t buffer = 0;
	ESP_ERROR_CHECK(
		i2c_master_transmit_receive(
			m_device_handle,
			reinterpret_cast<uint8_t*>(&addr),
			sizeof(addr),
			reinterpret_cast<uint8_t*>(&buffer),
			sizeof(buffer),
			pdMS_TO_TICKS(1000)
		)
	);
	
	return std::byteswap(buffer);
}

void INA226::writeRegister(Register addr, uint16_t data)
{
	#pragma pack(push, 1)
	struct
	{
		Register addr;
		uint16_t data;
	} buffer = {};
	#pragma pack(pop)
	
	buffer.addr = addr;
	buffer.data = std::byteswap(data);
	
	ESP_ERROR_CHECK(
		i2c_master_transmit(
			m_device_handle,
			reinterpret_cast<uint8_t*>(&buffer),
			sizeof(buffer),
			pdMS_TO_TICKS(1000)
		)
	);
}

double INA226::convertMeasurement(uint16_t raw_value, double lsb)
{
	return raw_value * lsb;
}

//========================================