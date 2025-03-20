#include <Selector.hpp>

//================================ Items

SelectorItem::SelectorItem(std::string_view label, std::string_view suffix /*= ""*/):
	m_label(label),
	m_suffix(suffix)
{}

std::string_view SelectorItem::getLabel() const
{
	return m_label;
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
	switch (event)
	{
		case RotaryEncoder::ButtonPressed:
			if (!m_shown)
				return m_shown = true;
			
			if (!m_item_focused)
				return m_item_focused = true;
			
			m_shown = m_item_focused = false;
			return true;
			
		case RotaryEncoder::ValueIncremented:
			if (m_shown)
			{
				if (m_item_focused)
					m_items[m_selected_item]->onRotate(1);
				
				else if (m_selected_item < m_items.size() - 1)
					m_selected_item++;
				
				return true;
			}
			
			break;
			
		case RotaryEncoder::ValueDecremented:
			if (m_shown)
			{
				if (m_item_focused)
					m_items[m_selected_item]->onRotate(-1);
				
				else if (m_selected_item > 0)
					m_selected_item--;
				
				return true;
			}
			
			break;
			
		default:
			break;
			
	}
	
	return false;
}

void Selector::render(SH1106Display& display) const
{
	if (!m_shown)
		return;
	
	const auto* item = m_items[m_selected_item];
	auto label = item->getLabel();
	auto display_size = display.getSize();
	
	Text(
		display,
		Vector2i(display_size.x / 2 - label.length() * GLYPH_SIZE.x / 2, 0),
		label,
		m_item_focused,
		true
	);
	
	auto value = item->serializeValue();
	Text(
		display,
		Vector2i(display_size.x / 2 - value.length() * GLYPH_SIZE.x / 2, GLYPH_SIZE.y),
		value,
		!m_item_focused,
		true
	);
}

//================================