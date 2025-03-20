#pragma once

#include "driver/i2c_master.h"

//========================================

class INA226
{
public:
	using MeasurementType = double;
	
	enum ConfigurationFlags: uint16_t
	{
		MODE1   = 1,
		MODE2   = 1 << 1,
		MODE3   = 1 << 2,
		VSHCT0  = 1 << 3,
		VSHCT1  = 1 << 4,
		VSHCT2  = 1 << 5,
		VBUSCT0 = 1 << 6,
		VBUSCT1 = 1 << 7,
		VBUSCT2 = 1 << 8,
		AVG0    = 1 << 9,
		AVG1    = 1 << 10,
		AVG2    = 1 << 11,
		RST     = 1 << 15,
		
		// Sample averaging modes
		// Determiming the number of samples that are collected and averaged
		SampleAverage1    = 0,
		SampleAverage4    = AVG0,
		SampleAverage16   = AVG1,
		SampleAverage64   = AVG1 | AVG0,
		SampleAverage128  = AVG2,
		SampleAverage256  = AVG2 | AVG0,
		SampleAverage512  = AVG2 | AVG1,
		SampleAverage1024 = AVG2 | AVG1 | AVG0,
		
		// Bus voltage conversion time
		// Sets the conversion time for the bus voltage measurement
		BusVoltageConversionTime140us   = 0,
		BusVoltageConversionTime204us   = VBUSCT0,
		BusVoltageConversionTime332us   = VBUSCT1,
		BusVoltageConversionTime588us   = VBUSCT1 | VBUSCT0,
		BusVoltageConversionTime1_1ms   = VBUSCT2,
		BusVoltageConversionTime2_116ms = VBUSCT2 | VBUSCT0,
		BusVoltageConversionTime4_156ms = VBUSCT2 | VBUSCT1,
		BusVoltageConversionTime8_244ms = VBUSCT2 | VBUSCT1 | VBUSCT0,
		
		// Shunt voltage conversion time
		// Sets the conversion time for the shunt voltage measurement
		ShuntVoltageConversionTime140us   = 0,
		ShuntVoltageConversionTime204us   = VSHCT0,
		ShuntVoltageConversionTime332us   = VSHCT1,
		ShuntVoltageConversionTime588us   = VSHCT1 | VSHCT0,
		ShuntVoltageConversionTime1_1ms   = VSHCT2,
		ShuntVoltageConversionTime2_116ms = VSHCT2 | VSHCT0,
		ShuntVoltageConversionTime4_156ms = VSHCT2 | VSHCT1,
		ShuntVoltageConversionTime8_244ms = VSHCT2 | VSHCT1 | VSHCT0,
		
		// Mode settings combinations
		// Selects continuous, triggered, or power-down mode of operation
		ModePowerDownTriggered     = 0,
		ModeShuntVoltageTriggeed   = MODE1,
		ModeBusVoltageTriggered    = MODE2,
		ModeShuntAndBusTriggered   = MODE2 | MODE1,
		ModePowerDownContinuous    = MODE3,
		ModeShuntVoltageContinuous = MODE3 | MODE1,
		ModeBusVoltageContinuous   = MODE3 | MODE2,
		ModeShuntAndBusContinuous  = MODE3 | MODE2 | MODE1,
		
		// Default configuration (power-on state)
		Default =
			SampleAverage1                  |
			BusVoltageConversionTime1_1ms   |
			ShuntVoltageConversionTime1_1ms |
			ModeShuntAndBusTriggered        |
			1 << 14                         |
			MODE3
	};
	
	enum AlertFlags: uint16_t
	{
		ShuntVoltageOverVoltage  = 1 << 15,
		ShuntVoltageUnderVoltage = 1 << 14,
		BusVoltageOverVoltage    = 1 << 13,
		BusVoltageUnderVoltage   = 1 << 12,
		PowerOverLimit           = 1 << 11,
		ConversionReady          = 1 << 10,
		
		AlertFunctionFlag        = 1 <<  4,
		ConversionReadyFlag      = 1 <<  3,
		MathOverflowFlag         = 1 <<  2,
		AlertPolarityFlag        = 1 <<  1,
		AlertLatchEnable         = 1
	};
	
	INA226() = default;
	INA226(const INA226& copy) = delete;
	~INA226();
	
	void startup(
		i2c_master_bus_handle_t i2c_bus_handle,
		int i2c_freq,
		uint8_t device_address = 0x45
	);
	
	void reset();
	
	MeasurementType readShuntVoltage();
	MeasurementType readBusVoltage();
	
	void     setConfiguration(uint16_t flags);
	uint16_t getConfiguration();
	
	void     setAlertMode(uint16_t flags);
	uint16_t getAlertMode();
	
	uint16_t getManufacturerID();
	uint16_t getDieID();
	
private:
	enum Register: uint8_t
	{
		Configuration  = 0x00, // All-register reset
		ShuntVoltage   = 0x01, // Shunt voltage measurement data
		BusVoltage     = 0x02, // Bus voltage measurement data
		Power          = 0x03, // Contains the value of the calculated power being delivered to the load
		Current        = 0x04, // Contains the value of the calculated current flowing through the shunt resistor
		Calibration    = 0x05, // Sets full-scale range LSB of current and power measurement; Overall system calibration
		MaskEnable     = 0x06, // Alert configuration and conversion ready flag
		AlertLimit     = 0x07, // Contains the limit value to compare to the selected Alert function
		ManufacturerID = 0xFE, // Contains unique manufacturer identification number
		DieID          = 0xFF  // Contains unique die identification
	};
	
	i2c_master_dev_handle_t m_device_handle = nullptr;

	uint16_t readRegister (Register addr);
	void     writeRegister(Register addr, uint16_t data);
	
	double convertMeasurement(uint16_t raw_value, double lsb);
	
};

//========================================