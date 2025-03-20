#include "esp_log.h"

#include <cstring>

#include <Peripherals/RotaryEncoder.hpp>

//========================================

RotaryEncoder::~RotaryEncoder()
{
	if (m_pin_a != GPIO_NUM_NC)
		gpio_isr_handler_remove(m_pin_a);
	
	if (m_pin_b != GPIO_NUM_NC)
		gpio_isr_handler_remove(m_pin_b);
	
	if (m_pin_press != GPIO_NUM_NC)
		gpio_isr_handler_remove(m_pin_press);
	
	if (m_event_queue)
		vQueueDelete(m_event_queue);
}

//========================================

void RotaryEncoder::setup(
	gpio_num_t pin_a,
	gpio_num_t pin_b,
	gpio_num_t pin_press
)
{
	m_pin_a = pin_a;
	m_pin_b = pin_b;
	m_pin_press = pin_press;
	
	gpio_config_t config = {};
	config.pin_bit_mask = 1 << pin_a | 1 << pin_b | 1 << pin_press;
	config.mode = GPIO_MODE_INPUT;
	config.pull_up_en = GPIO_PULLUP_ENABLE;
	config.intr_type = GPIO_INTR_ANYEDGE;
	gpio_config(&config);
	
	m_event_queue = xQueueCreate(16, sizeof(Event));
	
	gpio_isr_handler_add(pin_a,     InterruptHandler, this);
	gpio_isr_handler_add(pin_b,     InterruptHandler, this);
	gpio_isr_handler_add(pin_press, InterruptHandler, this);
}

//========================================

void RotaryEncoder::setValue(int value)
{
	m_value = value;
}

int RotaryEncoder::getValue() const
{
	return m_value;
}

bool RotaryEncoder::isPressed() const
{
	return m_press;
}

bool RotaryEncoder::pollEvent(Event* event)
{
	return xQueueReceive(m_event_queue, event, 0) == pdPASS;
}

//========================================

void RotaryEncoder::InterruptHandler(void* arg)
{
	auto& instance = *reinterpret_cast<RotaryEncoder*>(arg);
	
	State state = {};
	state.a     = gpio_get_level(instance.m_pin_a);
	state.b     = gpio_get_level(instance.m_pin_b);
	state.press = gpio_get_level(instance.m_pin_press);
	
	if (state.ab != instance.m_last_state.ab)
	{
		switch (instance.m_last_state.ab << 2 | state.ab)
		{
			// Clockwise
			case 0b0010:
			case 0b0100:
			case 0b1011:
			case 0b1101:
				instance.updateCounter(1);
				break;
				
			// Counterclockwise
			case 0b0001:
			case 0b0111:
			case 0b1000:
			case 0b1110:
				instance.updateCounter(-1);
				break;
				
		}
	}
	
	if (state.press != instance.m_last_state.press)
		instance.enqueueEvent(
			(instance.m_press = !state.press)
				? Event::ButtonPressed
				: Event::ButtonReleased
		);
	
	instance.m_last_state = state;
}

void RotaryEncoder::enqueueEvent(Event event)
{
	xQueueSendFromISR(m_event_queue, &event, nullptr);
}

void RotaryEncoder::updateCounter(int8_t direction)
{
	constexpr int8_t rotation_period = 4;
	if (abs(m_counter += direction) >= rotation_period)
	{
		if (m_counter > 0)
		{
			enqueueEvent(Event::ValueIncremented);
			m_value++;
		}
		
		else
		{
			enqueueEvent(Event::ValueDecremented);
			m_value--;
		}
		
		m_counter = 0;
	}
}

//========================================