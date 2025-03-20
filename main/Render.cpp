#include <Render.hpp>
#include <Vector.hpp>

//========================================

const Vector2u GLYPH_SIZE(8, 16);

//========================================

void Rectangle(
	SH1106Display& display,
	const Vector2i position,
	const Vector2i size,
	bool value /*= true*/
)
{
	Vector2i point;
	for (point.x = 0; point.x < size.x; point.x++)
		for (point.y = 0; point.y < size.y; point.y++)
			display.setPixel(position + point, value);
}

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
	Vector2i a,
	Vector2i b,
	bool value /*= true*/
)
{
	auto display_size = display.getSize();
	
	auto dx = abs(b.x - a.x);
	auto sx = 1 - 2 * (a.x > b.x);
	
	auto dy = -abs(b.y - a.y);
	auto sy = 1 - 2 * (a.y > b.y);
	
	auto error = dx + dy;
	while (true)
	{
		display.setPixel(a, value);
		
		auto error2 = 2 * error;
		if (error2 >= dy)
		{
			if (a.x == b.x)
				break;
			
			a.x += sx;
			if (!(0 <= a.x && a.x < display_size.x))
				break;
			
			error += dy;
		}
		
		if (error2 <= dx)
		{
			if (a.y == b.y)
				break;
			
			a.y += sy;
			if (!(0 <= a.y && a.y < display_size.y))
				break;
			
			error += dx;
		}
	}
}

void Character(
	SH1106Display& display,
	const Vector2i& position,
	char ch,
	bool value /*= true*/
)
{
	const auto* glyph = GetGlyph(ch);
	if (!glyph)
		return;

	Vector2i point;
	for (point.y = 0; point.y < GLYPH_SIZE.y; point.y++)
		for (point.x = 0; point.x < 8; point.x++)
			if ((glyph[point.y] >> (7 - point.x)) & 1)
				display.setPixel(position + point, value);
}

void Text(
	SH1106Display& display,
	const Vector2i& position,
	std::string_view text,
	bool value /*= true*/,
	bool fill  /*= false*/
)
{
	if (fill)
		Rectangle(display, position, TextSize(text), !value);
	
	for (size_t i = 0; i < text.length(); i++)
		Character(display, position + i * Vector2i(GLYPH_SIZE.x, 0), text[i], value);
}

Vector2u TextSize(std::string_view text)
{
	return GLYPH_SIZE * Vector2u(text.length(), 1);
}

//========================================

extern const uint8_t FONT_DATA[] asm("_binary_font_bin_start");

const uint8_t* GetGlyph(char ch)
{
	constexpr size_t glyph_size_bytes = 16;
	
	char begin = FONT_DATA[0];
	char end   = FONT_DATA[1];
	
	return begin <= ch && ch <= end
		? FONT_DATA + 2 + glyph_size_bytes * (ch - begin)
		: nullptr;
}

//========================================