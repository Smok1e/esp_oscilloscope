#include <Font.hpp>

//========================================

Font::Font(std::span<const uint8_t> data)
{
	const auto* header = reinterpret_cast<const FontHeader*>(data.data());
	m_glyph_size = Vector2u(header->glyph_size.x, header->glyph_size.y);
	m_glyph_size_bytes = header->glyph_size.bytes;
	
	m_ranges = std::span<const Range>(
		reinterpret_cast<const Range*>(header + 1),
		header->range_count
	);
	
	m_glyphs = std::span<const uint8_t>(
		reinterpret_cast<const uint8_t*>(m_ranges.data() + m_ranges.size()),
		1
	);
}

Font::Font(const uint8_t begin[], const uint8_t end[]):
	Font(std::span(begin, end))
{}

//========================================

const Vector2u& Font::getGlyphSize() const
{
	return m_glyph_size;
}

Vector2u Font::getTextSize(std::string_view text) const
{
	return getGlyphSize() * Vector2u(text.length(), 1);
}

size_t Font::getGlyphCount() const
{
	size_t count = 0;
	for (auto [begin, end]: m_ranges)
		count += end - begin + 1;
	
	return count;
}

const uint8_t* Font::getGlyph(char ch) const
{
	size_t offset = 0;
	for (auto [begin, end]: m_ranges)
	{
		if (begin <= ch && ch <= end)
			return m_glyphs.data() + (offset + ch - begin) * m_glyph_size_bytes;
		
		offset += end - begin + 1;
	}
	
	return nullptr;
}

const uint8_t* Font::operator[](char ch) const
{
	return getGlyph(ch);
}

//========================================