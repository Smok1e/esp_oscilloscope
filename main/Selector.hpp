#pragma once

#include <string_view>
#include <initializer_list>
#include <vector>
#include <algorithm>

#include <Selector.hpp>
#include <Render.hpp>
#include <Peripherals/RotaryEncoder.hpp>
#include <Peripherals/SH1106Display.hpp>

//================================ Basic selector item

class SelectorItem
{
public:
	SelectorItem(std::string_view label, std::string_view suffix = "");
	
	std::string_view getLabel() const;
	
	virtual std::string_view serializeValue() const = 0;
	virtual void onRotate(int8_t direction) = 0;
	
protected:
	std::string_view m_label;
	std::string_view m_suffix;
	
};

//================================ Number selector item

template<typename T>
concept NumberSelectorItemType = std::same_as<T, int> || std::same_as<T, float> || std::same_as<T, double>;

template<NumberSelectorItemType T>
class NumberSelectorItem: public SelectorItem
{
public:
	NumberSelectorItem(
		std::string_view label,
		T value,
		T min,
		T max,
		T step,
		std::string_view suffix = ""
	);
	
	void setValue(T value);
	T getValue() const;
	
	std::string_view serializeValue() const override;
	void onRotate(int8_t direction) override;
	
private:
	T m_value;
	T m_min;
	T m_max;
	T m_step;
	
};

using IntSelectorItem    = NumberSelectorItem<int>;
using FloatSelectorItem  = NumberSelectorItem<float>;
using DoubleSelectorItem = NumberSelectorItem<double>;

//================================ Selector

class Selector
{
public:
	Selector() = default;
	
	void addItem(SelectorItem* item);
	void operator+=(SelectorItem* item);
	
	bool onEvent(RotaryEncoder::Event event);
	void render(SH1106Display& display) const;
	
private:
	std::vector<SelectorItem*> m_items {};
	
	bool   m_shown         = false;
	bool   m_item_focused  = false;
	size_t m_selected_item = 0;
	
};

//================================

template<NumberSelectorItemType T>
NumberSelectorItem<T>::NumberSelectorItem(
	std::string_view label,
	T value,
	T min,
	T max,
	T step,
	std::string_view suffix /*= ""*/
):
	SelectorItem(label, suffix),
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
std::string_view NumberSelectorItem<T>::serializeValue() const
{
	if      constexpr (std::same_as<T, int   >) return FormatTmp("%d%.*s",  m_value, m_suffix.length(), m_suffix.data());
	else if constexpr (std::same_as<T, float >) return FormatTmp("%f%.*s",  m_value, m_suffix.length(), m_suffix.data());
	else if constexpr (std::same_as<T, double>) return FormatTmp("%lf%.*s", m_value, m_suffix.length(), m_suffix.data());
	else
		static_assert(false);
}

template<NumberSelectorItemType T>
void NumberSelectorItem<T>::onRotate(int8_t direction)
{
	m_value = std::clamp<T>(m_value + direction * m_step, m_min, m_max);
}

//================================