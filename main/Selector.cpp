#include <Selector.hpp>

//================================ Basic selector item

SelectorItem::SelectorItem(std::string_view label):
	m_label(label)
{}

std::string_view SelectorItem::getLabel() const
{
	return m_label;
}

//================================ Flag selector item

FlagSelectorItem::FlagSelectorItem(
	std::string_view label,
	bool             initial_value /*= false*/,
	std::string_view true_text     /*= "Yes"*/,
	std::string_view false_text    /*= "No"*/
):
	OptionSelectorItem(
		label,
		{
			{true_text,  true },
			{false_text, false}
		},
		initial_value
	)
{}

FlagSelectorItem::operator bool() const
{
	return getSelectedOption();
}

//================================ Selector

void Selector::addItem(SelectorItem* item)
{
	m_items.push_back(item);
}

void Selector::operator+=(SelectorItem* item)
{
	addItem(item);
}

bool Selector::onEvent(RotaryEncoder::Event event)
{
	switch (event.type)
	{
		case RotaryEncoder::Event::Button:
			if (event.button)
			{
				if (!m_shown)
				{
					m_shown = true;
					m_item_focused = false;
					m_animation_start_time = esp_timer_get_time();
					
					return true;
				}
				
				if (!m_item_focused)
				{
					m_item_focused = true;
					return true;
				}
				
				m_shown = false;
				m_animation_start_time = esp_timer_get_time();
				
				return true;
			}
			
			return false;
			
		case RotaryEncoder::Event::Rotation:
			if (!m_shown)
				return false;
			
			if (m_item_focused)
			{
				auto current_time = esp_timer_get_time();
				uint8_t rps = 1'000'000 / (current_time - m_last_rotation_time);
				m_last_rotation_time = current_time;
				
				m_items[m_selected_item]->onRotate(rps > 10? 10 * event.delta: event.delta);
			}
			
			else
				m_selected_item = std::clamp<int>(m_selected_item + event.delta, 0, m_items.size() - 1);
			
			return true;
	}
	
	return false;
}

void Selector::render(SH1106Display& display, const Font& font) const
{
	constexpr int animation_time_ms = 100;
	constexpr int rounding_radius   = 2;
	
	float t = static_cast<float>(esp_timer_get_time() - m_animation_start_time) / (1000 * animation_time_ms);
	
	if (t > 1.f)
	{
		if (!m_shown)
			return;
		
		t = 1.f;
	}
	
	t *= t;
	
	const auto& item = *m_items[m_selected_item];
	
	char buffer[32] = "";
	std::string_view value(
		buffer,
		item.serializeValue(buffer, std::size(buffer))
	);

	auto rect_size = font.getGlyphSize() * Vector2i(
		std::max(item.getLabel().length(), value.length()),
		1
	);
	
	auto offset = (display.getSize() - Vector2i(rect_size.x, font.getGlyphSize().y * 2)) / 2;

	if (m_shown)
		offset.y += (1.f - t) * rect_size.y;
	
	else
		offset.y -= t * rect_size.y;
	
	auto draw_text = [&](const std::string_view& text, uint8_t y, bool selected, uint8_t style)
	{
		Vector2i position(offset.x, offset.y + y * font.getGlyphSize().y);
		
		RoundedRectangle(
			display,
			position - rounding_radius,
			rect_size + rounding_radius,
			rounding_radius,
			selected,
			!selected * RoundedRectangleStyle::Outline | style
		);
		
		Text(
			display,
			font,
			Vector2i(display.getSize().x / 2 - text.length() * font.getGlyphSize().x / 2, position.y),
			text,
			!selected
		);
	};
	
	draw_text(item.getLabel(), 0, !m_item_focused, RoundedRectangleStyle::Top   );
	draw_text(value,           1,  m_item_focused, RoundedRectangleStyle::Bottom);
}

//================================