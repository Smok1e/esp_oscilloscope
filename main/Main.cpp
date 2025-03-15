#include "FreeRTOS/FreeRTOS.h"

#include "esp_log.h"
#include "esp_random.h"

#include "nvs_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include <random>
#include <chrono>
#include <numbers>

#include <SH1106Display.hpp>
#include <Vector.hpp>

//========================================

constexpr auto SPI_HOST = SPI2_HOST;
static const char* TAG = "oscilloscope";

void Circle(
	SH1106Display& display,
	const Vector2f& center,
	float radius,
	bool value = true
);

void Line(
	SH1106Display& display,
	const Vector2f& a,
	const Vector2f& b,
	bool value = true
);

//========================================

extern "C" void app_main()
{
	spi_bus_config_t spi_bus_config = {};
	spi_bus_config.miso_io_num = -1;
	spi_bus_config.mosi_io_num = CONFIG_SCREEN_PIN_MOSI;
	spi_bus_config.sclk_io_num = CONFIG_SCREEN_PIN_SCK;
	spi_bus_config.quadwp_io_num = -1;
	spi_bus_config.quadhd_io_num = -1;
	spi_bus_config.max_transfer_sz = 0xFFFF;

	auto result = spi_bus_initialize(SPI_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "spi bus initialized");
	
	SH1106Display display(
		SPI_HOST,
		static_cast<gpio_num_t>(CONFIG_SCREEN_PIN_DC),
		1'000'000 * CONFIG_SCREEN_SPI_FREQ_MHZ
	);
	
	display.startup();
	
	auto size = display.getSize();
	auto start_time = std::chrono::system_clock::now();
	
	while (true)
	{
		display.clear();
		
		Line(display, Vector2f(0, size.y / 2), Vector2f(size.x, size.y / 2));
		Line(display, Vector2f(size.x / 2, 0), Vector2f(size.x / 2, size.y));
		
		auto seconds = std::chrono::duration<float>(std::chrono::system_clock::now() - start_time).count();
		
		for (size_t x = 0; x < size.x; x++)
		{
			auto t = 5 * seconds + 20.f * static_cast<float>(x) / size.x;
			
			Circle(display, Vector2f(x, size.y * (1.f - .5f * (.8f * sin(t) + 1.f))), 1);
		}
		
		display.flush();

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

//========================================

void Circle(
	SH1106Display& display,
	const Vector2f& center,
	float radius,
	bool value /*= true*/
)
{
	float radius_sqr = radius*radius;
	
	Vector2f point;
	for (point.x = -radius; point.x <= radius; point.x++)
		for (point.y = -radius; point.y <= radius; point.y++)
			if (point.lengthSqr() < radius_sqr)
				display.setPixel(center + point, value);
}

void Line(
	SH1106Display& display,
	const Vector2f& a,
	const Vector2f& b,
	bool value /*= true*/
)
{
	float dt = 1.f / (b - a).length();
	
	for (float t = 0; t <= 1.f; t += dt)
		display.setPixel(a + t * (b - a), value);
}

//========================================