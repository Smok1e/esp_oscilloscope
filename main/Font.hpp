#pragma once

#include <span>
#include <string_view>

#include <Vector.hpp>

//========================================

class Font
{
public:
	explicit Font(std::span<const uint8_t> data);
	Font(const uint8_t begin[], const uint8_t end[]);
	Font(const Font& copy) = delete;
	
	const Vector2u& getGlyphSize() const;
	Vector2u getTextSize(std::string_view text) const;
	
	size_t getGlyphCount() const;
	
	const uint8_t* getGlyph(char ch) const;
	const uint8_t* operator[](char ch) const;
	
private:
	#pragma pack(push, 1)
	
	struct Range
	{
		char begin;
		char end;
	};
	
	struct FontHeader
	{
		struct
		{
			uint8_t x;
			uint8_t y;
			uint8_t bytes;
		} glyph_size;
	
		uint16_t range_count;
	};

	#pragma pack(pop)

	Vector2u                 m_glyph_size       {};
	uint8_t                  m_glyph_size_bytes {};
	std::span<const Range>   m_ranges           {};
	std::span<const uint8_t> m_glyphs           {};
	
};

//========================================