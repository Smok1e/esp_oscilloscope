#pragma once

#include <string_view>
#include <initializer_list>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cassert>

#include "esp_timer.h"

#include <Selector.hpp>
#include <Render.hpp>
#include <Peripherals/RotaryEncoder.hpp>
#include <Peripherals/SH1106Display.hpp>

//================================ Basic selector item

class SelectorItem
{
public:
	SelectorItem(std::string_view label);
	
	std::string_view getLabel() const;
	
	virtual size_t serializeValue(char* buffer, size_t buffsize) const = 0;
	virtual void onRotate(int8_t delta) = 0;
	
protected:
	std::string_view m_label;
	
};

//================================ Number selector item

template<typename T>
concept NumberSelectorItemType = std::integral<T> || std::floating_point<T>;

template<NumberSelectorItemType T>
class NumberSelectorItem: public SelectorItem
{
public:
	NumberSelectorItem(
		std::string_view label,
		std::string_view format,
		T value,
		T min,
		T max,
		T step
	);
	
	void setValue(T value);
	T getValue() const;
	
	operator const T&() const;
	
	size_t serializeValue(char* buffer, size_t buffsize) const override;
	void onRotate(int8_t delta) override;
	
private:
	std::string_view m_format;
	
	T m_value;
	T m_min;
	T m_max;
	T m_step;
	
};

using IntSelectorItem    = NumberSelectorItem<int>;
using FloatSelectorItem  = NumberSelectorItem<float>;
using DoubleSelectorItem = NumberSelectorItem<double>;

//================================ Option selector item

template<typename T>
class OptionSelectorItem: public SelectorItem
{
public:
	using Option = std::pair<std::string_view, T>;
	
	OptionSelectorItem(
		std::string_view label,
		const std::initializer_list<Option>& options,
		T initial_value
	);
	
	OptionSelectorItem(
		std::string_view label,
		const std::initializer_list<Option>& options
	);

	void setSelectedOption(T option);
	T getSelectedOption() const;
	
	size_t serializeValue(char* buffer, size_t buffsize) const override;
	void onRotate(int8_t delta) override;
	
private:
	std::vector<Option> m_options;
	size_t m_selected_option;
	
};

class FlagSelectorItem: public OptionSelectorItem<bool>
{
public:
	explicit FlagSelectorItem(
		std::string_view label,
		bool             initial_value = false,
		std::string_view true_text     = "Yes",
		std::string_view false_text    = "No"
	);
	
	operator bool() const;

};

//================================ Selector

class Selector
{
public:
	Selector() = default;
	
	void addItem(SelectorItem* item);
	void operator+=(SelectorItem* item);
	
	bool onEvent(RotaryEncoder::Event event);
	void render(SH1106Display& display, const Font& font) const;
	
private:
	std::vector<SelectorItem*> m_items {};
	
	bool   m_shown          = false;
	bool   m_item_focused   = false;
	size_t m_selected_item  = 0;

	uint64_t m_last_rotation_time = esp_timer_get_time();
	
	// Animation
	uint64_t m_animation_start_time = esp_timer_get_time();
	enum class AnimationType: uint8_t
	{
		None,
		
		Show,
		Hide,
		Left,
		Right
	} m_animation_type = AnimationType::None;
	
	void startAnimation(AnimationType type);
	
};

//================================ Number selector item

template<NumberSelectorItemType T>
NumberSelectorItem<T>::NumberSelectorItem(
	std::string_view label,
	std::string_view format,
	T value,
	T min,
	T max,
	T step
):
	SelectorItem(label),
	m_format(format),
	m_value(value),
	m_min(min),
	m_max(max),
	m_step(step)
{}

template<NumberSelectorItemType T>
void NumberSelectorItem<T>::setValue(T value)
{
	m_value = value;
}

template<NumberSelectorItemType T>
T NumberSelectorItem<T>::getValue() const
{
	return m_value;
}

template<NumberSelectorItemType T>
NumberSelectorItem<T>::operator const T&() const
{
	return m_value;
}

template<NumberSelectorItemType T>
size_t NumberSelectorItem<T>::serializeValue(char* buffer, size_t buffsize) const
{
	return snprintf(buffer, buffsize, m_format.data(), m_value);
}

template<NumberSelectorItemType T>
void NumberSelectorItem<T>::onRotate(int8_t delta)
{
	m_value = std::clamp<T>(m_value + delta * m_step, m_min, m_max);
}

//================================ Option selector item

template<typename T>
OptionSelectorItem<T>::OptionSelectorItem(
	std::string_view label,
	const std::initializer_list<Option>& options,
	T initial_value
):
	SelectorItem(label),
	m_options(options),
	m_selected_option(
		std::ranges::distance(
			options.begin(),
			std::ranges::find_if(
				options,
				[&](const Option& option) -> bool
				{
					return option.second == initial_value;
				}
			)
		)
	)
{
	assert(m_selected_option < m_options.size());
}

template<typename T>
OptionSelectorItem<T>::OptionSelectorItem(
	std::string_view label,
	const std::initializer_list<Option>& options
):
	SelectorItem(label),
	m_options(options),
	m_selected_option(0)
{}

template<typename T>
void OptionSelectorItem<T>::setSelectedOption(T option)
{
	m_selected_option = std::ranges::distance(m_options.begin(), std::ranges::find(m_options, option));
	assert(m_selected_option < m_options.size());
}

template<typename T>
T OptionSelectorItem<T>::getSelectedOption() const
{
	return m_options[m_selected_option].second;
}

template<typename T>
size_t OptionSelectorItem<T>::serializeValue(char* buffer, size_t buffsize) const
{
	return m_options[m_selected_option].first.copy(buffer, buffsize);
}

template<typename T>
void OptionSelectorItem<T>::onRotate(int8_t delta)
{
	(m_selected_option += delta) %= m_options.size();
}

//================================