#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_random.h"

#include "driver/spi_master.h"
#include "driver/i2c_master.h"

#include <deque>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <numeric>

#include <Peripherals/SH1106Display.hpp>
#include <Peripherals/INA226.hpp>
#include <Peripherals/RotaryEncoder.hpp>
#include <Render.hpp>
#include <Selector.hpp>

//========================================

using namespace std::chrono_literals;

static const char* TAG = "oscilloscope";

constexpr auto SPI_HOST = SPI2_HOST;
constexpr auto I2C_PORT = I2C_NUM_0;

//========================================

class Main
{
public:
	using clock = std::chrono::high_resolution_clock;
	
	Main() = default;

	void run();

private:
	SH1106Display m_display  {};
	INA226        m_adc      {};
	RotaryEncoder m_knob     {};
	
	NumberSelectorItem<INA226::MeasurementType> m_min_voltage    {"vMin",        0.00, 0.00, 36.0, .005, "V" };
	NumberSelectorItem<INA226::MeasurementType> m_max_voltage    {"vMax",        0.05, 0.00, 36.0, .005, "V" };
	IntSelectorItem                             m_sample_rate_hz {"Sample rate", 100,  100,  2000, 100,  "Hz"};
	Selector m_selector {};
	
	std::deque<INA226::MeasurementType> m_samples {};
	size_t m_max_samples {};
	
	clock::time_point m_last_measurement_time = clock::now();
	
	void initDisplay();
	void initADC();
	void initKnob();
	
	void renderLoop();
	
	void update();
	void render();
	
};

//======================================== Initialization

void Main::initDisplay()
{
	spi_bus_config_t spi_bus_config = {};
	spi_bus_config.miso_io_num = -1;
	spi_bus_config.mosi_io_num = CONFIG_DISPLAY_PIN_MOSI;
	spi_bus_config.sclk_io_num = CONFIG_DISPLAY_PIN_SCK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0xFFFF;

	auto result = spi_bus_initialize(SPI_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "SPI bus initialized");
	
	m_display.setup(
		SPI_HOST,
		static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_DC),
		1'000'000 * CONFIG_DISPLAY_SPI_FREQ_MHZ
	);
	
	m_max_samples = m_display.getSize().x;
	
	ESP_LOGI(TAG, "display initialized");
}

void Main::initADC()
{
	i2c_master_bus_config_t i2c_bus_config = {};
	i2c_bus_config.i2c_port = I2C_PORT;
	i2c_bus_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_ADC_PIN_SDA);
	i2c_bus_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_ADC_PIN_SCL);
	i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_bus_config.glitch_ignore_cnt = 7;
	i2c_bus_config.flags.enable_internal_pullup = true;
	
	i2c_master_bus_handle_t i2c_bus_handle = nullptr;
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
	
	ESP_LOGI(TAG, "I2C bus initialized");
	
	m_adc.startup(
		i2c_bus_handle,
		1'000 * CONFIG_ADC_I2C_FREQ_KHZ,
		CONFIG_ADC_I2C_DEVICE_ADDRESS
	);
	
	m_adc.setConfiguration(
		INA226::SampleAverage1                  |
		INA226::BusVoltageConversionTime1_1ms   |
		INA226::ShuntVoltageConversionTime1_1ms |
		INA226::ModeShuntAndBusContinuous
	);
	
	ESP_LOGI(TAG, "ADC initialized");
}

void Main::initKnob()
{
	gpio_install_isr_service(0);
	ESP_LOGI(TAG, "ISR service initialized");
	
	m_knob.setup(
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_A),
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_B),
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_PRESS)
	);
	
	m_selector += &m_min_voltage;
	m_selector += &m_max_voltage;
	m_selector += &m_sample_rate_hz;
	
	ESP_LOGI(TAG, "knob initialized");
}

//======================================== Rendering

void Main::renderLoop()
{
	while (true)
	{
		update();
		render();
		
		vTaskDelay(1);
	}
}

void Main::update()
{
	// Measurement
	auto current_time = clock::now();
	if (current_time - m_last_measurement_time > std::chrono::duration<float>(1.f / m_sample_rate_hz.getValue()))
	{
		auto voltage = m_adc.readBusVoltage();
		
		m_samples.push_back(voltage);
		if (m_samples.size() > m_max_samples)
			m_samples.pop_front();
		
		m_last_measurement_time = current_time;
	}
	
	// Knob
	RotaryEncoder::Event event;
	while (m_knob.pollEvent(&event))
		m_selector.onEvent(event);
}

void Main::render()
{
	auto display_size = m_display.getSize();
	
	m_display.clear();
	
	// Plot
	if (!m_samples.empty())
	{
		Vector2i prev;
		for (size_t i = 0; i < m_samples.size(); i++)
		{
			Vector2i curr(
				i,
				display_size.y * (1.f - (m_samples[i] - m_min_voltage.getValue()) / (m_max_voltage.getValue() - m_min_voltage.getValue()))
			);
			
			if (i)
				Line(m_display, prev, curr);
			
			prev = curr;
		}
	}

	m_selector.render(m_display);
	m_display.flush();
}

//========================================

void Main::run()
{
	initDisplay();
	initADC();
	initKnob();

	renderLoop();
}

//========================================

extern "C" void app_main()
{
	Main instance;
	instance.run();
}

//========================================