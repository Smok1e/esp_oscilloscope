#pragma once

#include <string_view>

#include <Peripherals/SH1106Display.hpp>
#include <Vector.hpp>

//========================================

extern const Vector2u GLYPH_SIZE;

//========================================

void Rectangle(
	SH1106Display& display,
	const Vector2i position,
	const Vector2i size,
	bool value = true
);

void Circle(
	SH1106Display& display,
	const Vector2f& center,
	float radius,
	bool value = true
);

void Line(
	SH1106Display& display,
	Vector2i a,
	Vector2i b,
	bool value = true
);

void Character(
	SH1106Display& display,
	const Vector2i& position,
	char ch,
	bool value = true
);

void Text(
	SH1106Display& display,
	const Vector2i& position,
	std::string_view text,
	bool value = true,
	bool fill  = false
);


Vector2u TextSize(std::string_view text);

const uint8_t* GetGlyph(char ch);

//========================================

template<typename... Args>
std::string_view FormatTmp(const char* format, const Args&... args)
{
	static char buffer[128] = "";
	
	return std::string_view(
		buffer,
		snprintf(buffer, std::size(buffer), format, args...)
	);
}

//========================================