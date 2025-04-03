#include <cmath>
#include <cstring>

#include <Peripherals/SH1106Display.hpp>

//========================================

Vector2u SH1106Display::s_max_size = Vector2u(132, 64);
size_t   SH1106Display::s_buffer_size = s_max_size.x * s_max_size.y;

//========================================

SH1106Display::~SH1106Display()
{
	heap_caps_free(m_pixel_data);
	ESP_ERROR_CHECK(spi_bus_remove_device(m_device_handle));
}

//========================================

void SH1106Display::setup(
	spi_host_device_t spi_host,
	gpio_num_t pin_dc,
	int spi_freq,
	const Vector2u& size /*= Vector2u(128, 64)*/
)
{
	m_pin_dc = pin_dc;
	m_size = size;
	
	m_pixel_data = reinterpret_cast<uint8_t*>(heap_caps_malloc(s_buffer_size, MALLOC_CAP_DMA));
	
	spi_device_interface_config_t device_config = {};
	device_config.clock_speed_hz = spi_freq;
	device_config.mode = 0;
	device_config.spics_io_num = -1;
	device_config.queue_size = 1;
	ESP_ERROR_CHECK(spi_bus_add_device(spi_host, &device_config, &m_device_handle));
	
	ESP_ERROR_CHECK(gpio_reset_pin(m_pin_dc));
	ESP_ERROR_CHECK(gpio_set_direction(m_pin_dc, GPIO_MODE_OUTPUT));

	sendCommand(Command::SetCommonOutputScanDirection | 8   );
	sendCommand(Command::SetSegmentRemap              | true);
	sendCommand(Command::SetDisplayOn                 | true);
	
	vTaskDelay(pdMS_TO_TICKS(100));
	
	clear();
	flush();
}

void SH1106Display::flush()
{
	constexpr size_t pages = 8;
	for (size_t page = 0; page < pages; page++)
	{
		sendCommand(Command::SetPageAddress | page);
		setColumnAddress(0);
		
		spi_transaction_t transaction = {};
		transaction.tx_buffer = m_pixel_data + page * s_max_size.x;
		transaction.length = s_max_size.x * 8;
		
		sendCommand(Command::SetReadModifyWriteStart);
		ESP_ERROR_CHECK(gpio_set_level(m_pin_dc, true));
		ESP_ERROR_CHECK(spi_device_transmit(m_device_handle, &transaction));
		sendCommand(Command::SetReadModifyWriteEnd);
	}
}

const Vector2u& SH1106Display::getSize() const
{
	return m_size;
}

bool SH1106Display::setPixel(const Vector2i& position, bool value)
{
	if (!(0 <= position.x && position.x < m_size.x && 0 <= position.y && position.y < m_size.y))
		return false;
	
	auto pixel = (position.y + (s_max_size.y - m_size.y) / 2) * s_max_size.x + (position.x + (s_max_size.x - m_size.x) / 2);
	
	auto byte = pixel % s_max_size.x + pixel / (8 * s_max_size.x) * s_max_size.x;
	auto bit = (pixel / s_max_size.x) % 8;
	
	(m_pixel_data[byte] &= ~(1 << bit)) |= (value << bit);
	return true;
}

void SH1106Display::setContrast(uint8_t contrast)
{
	sendCommand(Command::SetContrastControlMode);
	sendCommand(Command::SetContrast | (m_contrast = contrast));
}

uint8_t SH1106Display::getContrast() const
{
	return m_contrast;
}

void SH1106Display::setInverted(bool value)
{
	sendCommand(Command::SetReverseMode | (m_inverted = value));
}

bool SH1106Display::isInverted() const
{
	return m_inverted;
}

void SH1106Display::clear(bool value /*= false*/)
{
	std::fill(m_pixel_data, m_pixel_data + s_buffer_size, value * ~0);
}

//========================================

void SH1106Display::sendCommand(uint8_t cmd)
{
	gpio_set_level(m_pin_dc, false);
	
	spi_transaction_t transaction = {};
	transaction.tx_data[0] = cmd;
	transaction.flags = SPI_TRANS_USE_TXDATA;
	transaction.length = 8;
	
	ESP_ERROR_CHECK(spi_device_transmit(m_device_handle, &transaction));
}

void SH1106Display::setColumnAddress(uint8_t column)
{
	sendCommand(Command::SetColumnAddressLowerBits  | (column & 0x0F)     );
	sendCommand(Command::SetColumnAddressHigherBits | (column & 0xF0) >> 4);
}

//========================================