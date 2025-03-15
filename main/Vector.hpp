#pragma once

#include <concepts>

//========================================

template<typename T>
concept Scalar = std::integral<T> || std::floating_point<T>;

template<Scalar T>
class Vector2
{
public:
	union
	{
		struct
		{
			T x, y;
		};
		
		T data[2];
	};
	
	Vector2();
	Vector2(T x, T y);
	
	template<Scalar U>
	Vector2(const Vector2<U>& copy);
	
	T lengthSqr() const;
	float length() const;
	
	template<Scalar U>
	Vector2<T> operator+(const Vector2<U>& rhs) const;
	
	template<Scalar U>
	Vector2<T> operator-(const Vector2<U>& rhs) const;
	
	template<Scalar U>
	Vector2<T> operator+(U scalar) const;
	
	template<Scalar U>
	Vector2<T> operator-(U scalar) const;
	
	template<Scalar U>
	Vector2<T> operator*(U scalar) const;
	
	template<Scalar U>
	Vector2<T> operator/(U scalar) const;
	
};

//========================================

template<Scalar T, Scalar U>
Vector2<T> operator+(U scalar, const Vector2<T> vec);

template<Scalar T, Scalar U>
Vector2<T> operator*(U scalar, const Vector2<T> vec);

//========================================

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned>;

//======================================== Constructors

template<Scalar T>
Vector2<T>::Vector2():
	data {}
{}

template<Scalar T>
Vector2<T>::Vector2(T x_, T y_):
	x(x_),
	y(y_)
{}

template<Scalar T>
template<Scalar U>
Vector2<T>::Vector2(const Vector2<U>& copy):
	x(static_cast<T>(copy.x)),
	y(static_cast<T>(copy.y))
{}

//======================================== Linear operations

template<Scalar T>
T Vector2<T>::lengthSqr() const
{
	return x*x + y*y;
}

template<Scalar T>
float Vector2<T>::length() const
{
	return sqrt(x*x + y*y);
}

//======================================== Arithmetic operations with vectors

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator+(const Vector2<U> &rhs) const
{
	return Vector2<T>(x + rhs.x, y + rhs.y);
}

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator-(const Vector2<U> &rhs) const
{
	return Vector2<T>(x - rhs.x, y - rhs.y);
}

//======================================== Arithmetic operations with rhs scalars

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator+(U scalar) const
{
	return Vector2<T>(x + scalar, y + scalar);
}

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator-(U scalar) const
{
	return Vector2<T>(x - scalar, y - scalar);
}

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator*(U scalar) const
{
	return Vector2<T>(x * scalar, y * scalar);
}

template<Scalar T>
template<Scalar U>
Vector2<T> Vector2<T>::operator/(U scalar) const
{
	return Vector2<T>(x / scalar, y / scalar);
}

//======================================== Arithmetic operations with lhs scalars

template<Scalar T, Scalar U>
Vector2<T> operator+(U scalar, const Vector2<T> vec)
{
	return Vector2<T>(scalar + vec.x, scalar + vec.y);
}

template<Scalar T, Scalar U>
Vector2<T> operator*(U scalar, const Vector2<T> vec)
{
	return Vector2<T>(scalar * vec.x, scalar * vec.y);
}

//========================================