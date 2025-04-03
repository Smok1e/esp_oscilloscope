#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

#include "driver/spi_master.h"
#include "driver/i2c_master.h"

#include "rom/ets_sys.h"

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

constexpr auto SCREEN_SPI_HOST         = SPI2_HOST;
constexpr auto ADC_I2C_PORT            = I2C_NUM_0;

constexpr auto INTERNAL_ADC_UNIT       = ADC_UNIT_2;
constexpr auto INTERNAL_ADC_CHANNEL    = ADC_CHANNEL_0;
constexpr auto INTERNAL_ADC_ATTEN      = ADC_ATTEN_DB_2_5;
constexpr auto INTERNAL_ADC_RESOLUTION = ADC_BITWIDTH_DEFAULT;

extern const uint8_t FONT_BEGIN[] asm("_binary_font_bin_start");
extern const uint8_t FONT_END  [] asm("_binary_font_bin_end"  );

//========================================

class Main
{
public:
	Main() = default;

	void run();

private:
	// Peripherals
	SH1106Display m_display {};
	INA226        m_adc     {};
	RotaryEncoder m_knob    {};
	
	adc_oneshot_unit_handle_t m_internal_adc_handle      = nullptr;
	adc_cali_handle_t         m_internal_adc_cali_handle = nullptr;
	
	// Selector menu
	NumberSelectorItem<INA226::MeasurementType> m_min_voltage {
		"vMin",
		"%.3lf V",
		0.00,
		-36.0,
		 36.0,
		.005
	};
	
	NumberSelectorItem<INA226::MeasurementType> m_max_voltage {
		"vMax",
		"%.3lf V",
		0.05,
		0.00,
		36.0,
		.005
	};
	
	FlagSelectorItem m_autoscale {
		"Autoscale",
		true
	};
	
	IntSelectorItem m_sample_rate_hz {
		"Sample rate",
		"%d Hz",
		1000,
		100,
		25000,
		100
	};
	
	NumberSelectorItem<int> m_window_size_ms {
		"Window size",
		"%d ms",
		100,
		10,
		1000,
		1
	};
	
	FlagSelectorItem m_draw_line {
		"Draw line",
		false
	};
	
	enum class SignalSource: uint8_t
	{
		BusVoltage,
		ShuntVoltage,
		InternalADC,
		TestSine
	};
	
	OptionSelectorItem<SignalSource> m_signal_source {
		"Signal source",
		{
			{ "Bus voltage",   SignalSource::BusVoltage   },
			{ "Shunt voltage", SignalSource::ShuntVoltage },
			{ "Internal ADC",  SignalSource::InternalADC  },
			{ "Sine",          SignalSource::TestSine     }
		}
	};
	
	FlagSelectorItem m_invert_display {
		"Display",
		false,
		"Inverted",
		"Normal"
	};
	
	NumberSelectorItem<int8_t> m_contrast {
		"Contrast",
		"%d",
		0x3F,
		0,
		0x3F,
		1
	};
	
	Selector m_selector {};
	
	// Font
	Font m_font { FONT_BEGIN, FONT_END };
	
	// Samples
	size_t m_current_sample = 0;
	std::span<INA226::MeasurementType> m_samples {};
	QueueHandle_t m_queue = nullptr;
	
	void initDisplay();
	void initADC();
	void initInternalAdc();
	void initKnob();
	
	void renderLoop();
	void measurementLoop();
	
	float readInternalAdcVoltage();
	void resizeBuffer(size_t new_size);
	
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

	auto result = spi_bus_initialize(SCREEN_SPI_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "SPI bus initialized");
	
	m_display.setup(
		SCREEN_SPI_HOST,
		static_cast<gpio_num_t>(CONFIG_DISPLAY_PIN_DC),
		1'000'000 * CONFIG_DISPLAY_SPI_FREQ_MHZ
	);
	
	ESP_LOGI(TAG, "display initialized");
}

void Main::initADC()
{
	i2c_master_bus_config_t i2c_bus_config = {};
	i2c_bus_config.i2c_port = ADC_I2C_PORT;
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
		INA226::ShuntVoltageConversionTime140us |
		INA226::BusVoltageConversionTime140us   |
		INA226::ModeShuntAndBusContinuous
	);

	ESP_LOGI(TAG, "ADC initialized");
	
	m_queue = xQueueCreate(16, sizeof(uint32_t));
	resizeBuffer((m_sample_rate_hz.getValue() * m_window_size_ms.getValue()) / 1000);
}

void Main::initInternalAdc()
{
	adc_oneshot_unit_init_cfg_t init_cfg = {};
	init_cfg.unit_id = INTERNAL_ADC_UNIT;
	
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &m_internal_adc_handle));
	
	adc_oneshot_chan_cfg_t channel_cfg = {};
	channel_cfg.atten = INTERNAL_ADC_ATTEN;
	channel_cfg.bitwidth = INTERNAL_ADC_RESOLUTION;
	ESP_ERROR_CHECK(adc_oneshot_config_channel(m_internal_adc_handle, INTERNAL_ADC_CHANNEL, &channel_cfg));
	
	adc_cali_line_fitting_config_t cali_cfg = {};
	cali_cfg.unit_id = INTERNAL_ADC_UNIT;
	cali_cfg.atten = INTERNAL_ADC_ATTEN;
	cali_cfg.bitwidth = INTERNAL_ADC_RESOLUTION;
	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &m_internal_adc_cali_handle));
}

void Main::initKnob()
{
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	ESP_LOGI(TAG, "ISR service initialized");
	
	m_knob.setup(
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_A),
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_B),
		static_cast<gpio_num_t>(CONFIG_ENCODER_PIN_PRESS)
	);

	m_selector += &m_min_voltage;
	m_selector += &m_max_voltage;
	m_selector += &m_autoscale;
	m_selector += &m_sample_rate_hz;
	m_selector += &m_window_size_ms;
	m_selector += &m_draw_line;
	m_selector += &m_signal_source;
	m_selector += &m_invert_display;
	m_selector += &m_contrast;
	
	ESP_LOGI(TAG, "knob initialized");
	ESP_LOGI(
		TAG,
		"font size: %hux%hu | %zu glyphs",
		m_font.getGlyphSize().x,
		m_font.getGlyphSize().y,
		m_font.getGlyphCount()
	);
}

//======================================== Rendering

void Main::renderLoop()
{
	ESP_LOGI(
		TAG,
		"render loop is running on CPU%d",
		static_cast<int>(xTaskGetCoreID(xTaskGetCurrentTaskHandle()))
	);
	
	const auto& display_size = m_display.getSize();
	while (true)
	{
		// Queue
		uint32_t sample_time = 0;
		if (xQueueReceive(m_queue, &sample_time, 0) == pdPASS)
			ESP_LOGW(TAG, "can't keep sample rate: last sample took %" PRIu32 " us", sample_time);
		
		// Knob
		RotaryEncoder::Event event;
		while (m_knob.pollEvent(&event))
		{
			if (m_selector.onEvent(event))
				continue;
			
			switch (event.type)
			{
				case RotaryEncoder::Event::Rotation:
					m_min_voltage.setValue(m_min_voltage + .05 * event.delta);
					m_max_voltage.setValue(m_max_voltage + .05 * event.delta);
					break;
				
				default:
					break;
				
			}
		}
		
		if (m_invert_display != m_display.isInverted())
			m_display.setInverted(m_invert_display);
		
		if (m_contrast != m_display.getContrast())
			m_display.setContrast(m_contrast);
		
		// Plot
		m_display.clear();
		
		if (m_autoscale)
		{
			auto [min, max] = std::ranges::minmax_element(m_samples);
			m_min_voltage.setValue(*min);
			m_max_voltage.setValue(*max);
		}
		
		auto min_voltage = m_min_voltage.getValue();
		auto max_voltage = m_max_voltage.getValue();
		
		Vector2i prev(-1, 0);
		INA226::MeasurementType signal = 0;
		for (size_t i = 0, signal_count = 0; i < m_samples.size(); i++)
		{
			signal += m_samples[i];
			signal_count++;
			
			if (int x = i * display_size.x / m_samples.size(); x != prev.x)
			{
				signal /= signal_count;
				
				Vector2i curr(
					x,
					display_size.y * (1.f - (signal - min_voltage) / (max_voltage - min_voltage))
				);
				
				if (x)
					Line(m_display, prev, curr);
				
				prev = curr;
				signal = 0;
				signal_count = 0;
			}
		}
		
		if (m_draw_line)
		{
			auto line_x = m_current_sample * display_size.x / m_samples.size();
			Line(m_display, Vector2i(line_x, 0), Vector2i(line_x, display_size.y));
		}
	
		m_selector.render(m_display, m_font);
		m_display.flush();
		
		vTaskDelay(1);
	}
}

//======================================== Measurement

void Main::measurementLoop()
{
	// 60 ms
	
	ESP_LOGI(
		TAG,
		"measurement loop is running on CPU%d",
		static_cast<int>(xTaskGetCoreID(xTaskGetCurrentTaskHandle()))
	);

	auto start_time = esp_timer_get_time();
	auto last_sample_time = start_time;
	
	while (true)
	{
		auto sample_period_us = 1'000'000 / m_sample_rate_hz.getValue();
		auto time_since_last_sample = esp_timer_get_time() - last_sample_time;

		if (time_since_last_sample > sample_period_us)
			xQueueSend(m_queue, &time_since_last_sample, 0);
		
		else
			ets_delay_us(sample_period_us - time_since_last_sample);
		
		last_sample_time = esp_timer_get_time();
		
		if (
			size_t sample_count = (m_sample_rate_hz.getValue() * m_window_size_ms.getValue()) / 1000;
			m_samples.size() != sample_count
		)
			resizeBuffer(sample_count);
		
		auto& current_sample = m_samples[m_current_sample];
		switch (m_signal_source.getSelectedOption())
		{
			case SignalSource::BusVoltage:
				current_sample = m_adc.readBusVoltage();
				break;
				
			case SignalSource::ShuntVoltage:
				current_sample = m_adc.readShuntVoltage();
				break;
				
			case SignalSource::InternalADC:
				current_sample = readInternalAdcVoltage();
				break;
			
			case SignalSource::TestSine:
				current_sample = .1 * sin(50 * 2.0 * std::numbers::pi * static_cast<double>(esp_timer_get_time() - start_time) / 1'000'000);
				break;
				
		}
		
		(++m_current_sample) %= m_samples.size();
	}
}

float Main::readInternalAdcVoltage()
{
	int raw_voltage = 0;
	ESP_ERROR_CHECK(adc_oneshot_read(m_internal_adc_handle, INTERNAL_ADC_CHANNEL, &raw_voltage));
	
	int voltage_mv = 0;
	ESP_ERROR_CHECK(adc_cali_raw_to_voltage(m_internal_adc_cali_handle, raw_voltage, &voltage_mv));
	
	return static_cast<float>(voltage_mv) / 1000.f;
}

void Main::resizeBuffer(size_t new_size)
{
	if (m_samples.data())
		delete[] m_samples.data();
	
	m_samples = std::span(new INA226::MeasurementType[new_size], new_size);
	m_current_sample = 0;
}

//========================================

void Main::run()
{
	initDisplay();
	initADC();
	initInternalAdc();
	initKnob();

	// Running measurement loop on CPU1
	TaskHandle_t task_handle = nullptr;
	xTaskCreatePinnedToCore(
		[](void* arg)
		{
			reinterpret_cast<Main*>(arg)->measurementLoop();
		},
		"Measurement loop",
		4096,
		this,
		configMAX_PRIORITIES - 1,
		&task_handle,
		1 // CPU1
	);
	
	renderLoop();
}

//========================================

extern "C" void app_main()
{
	Main instance;
	instance.run();
}

//========================================