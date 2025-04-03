#include <Render.hpp>
#include <Vector.hpp>

//========================================

void Rectangle(
	SH1106Display& display,
	const Vector2i& position,
	const Vector2i& size,
	bool value /*= true*/
)
{
	Vector2i point;
	for (point.x = 0; point.x < size.x; point.x++)
		for (point.y = 0; point.y < size.y; point.y++)
			display.setPixel(position + point, value);
}

void RoundedRectangle(
	SH1106Display&  display,
	const Vector2i& position,
	const Vector2i& size,
	int             radius,
	bool            value /*= true*/,
	uint8_t         style /*= RoundedRectangleStyle::Default*/
)
{
	int radius_sqr = pow(radius, 2);
	
	Vector2i point;
	for (point.x = 0; point.x < size.x; point.x++)
	{
		for (point.y = 0; point.y < size.y; point.y++)
		{
			int point_radius_sqr = -1;
			
			if (
				style & RoundedRectangleStyle::Left &&
				point.x <= radius
			)
			{
				if (
					style & RoundedRectangleStyle::LeftTop &&
					point.y <= radius
				)
					point_radius_sqr = pow(point.x - radius, 2) + pow(point.y - radius, 2);
				
				else if (
					style & RoundedRectangleStyle::LeftBottom &&
					point.y >= size.y - radius
				)
					point_radius_sqr = pow(point.x - radius, 2) + pow(size.y - point.y - 1 - radius, 2);
			}
			
			else if (
				style & RoundedRectangleStyle::Right &&
				point.x >= size.x - radius
			)
			{
				if (
					style & RoundedRectangleStyle::RightTop &&
					point.y <= radius
				)
					point_radius_sqr = pow(size.x - point.x - radius - 1, 2) + pow(point.y - radius, 2);
				
				else if (
					style & RoundedRectangleStyle::RightBottom &&
					point.y >= size.y - radius
				)
					point_radius_sqr = pow(size.x - point.x - radius - 1, 2) + pow(size.y - point.y - 1 - radius, 2);
			}

			if (point_radius_sqr < 0)
				display.setPixel(
					position + point,
					style & RoundedRectangleStyle::Outline
						? value ^ (point.x == 0 || point.x == size.x - 1 || point.y == 0 || point.y == size.y - 1)
						: value
				);
			
			else if (point_radius_sqr <= radius_sqr)
				display.setPixel(
					position + point,
					style & RoundedRectangleStyle::Outline
						? value ^ (radius_sqr - point_radius_sqr <= radius)
						: value
				);
		}
	}
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
	const Font& font,
	const Vector2i& position,
	char ch,
	bool value /*= true */,
	bool fill  /*= false*/
)
{
	if (const auto* data = font[ch])
	{
		Vector2i point;
		for (point.y = 0; point.y < font.getGlyphSize().y; point.y++)
		{
			for (point.x = 0; point.x < font.getGlyphSize().x; point.x++)
			{
				uint16_t i = point.y * font.getGlyphSize().x + point.x;
				
				if ((data[i / 8] >> (i % 8)) & 1)
					display.setPixel(position + point, value);
				
				else if (fill)
					display.setPixel(position + point, !value);
			}
		}
	}
}

void Text(
	SH1106Display& display,
	const Font& font,
	const Vector2i& position,
	std::string_view text,
	bool value /*= true*/,
	bool fill  /*= false*/
)
{
	for (size_t i = 0; i < text.length(); i++)
		Character(display, font, position + i * Vector2i(font.getGlyphSize().x, 0), text[i], value, fill);
}

//========================================