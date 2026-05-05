/*
**  ClanLib SDK
**  Copyright (c) 1997-2010 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
**    Mark Page
**    Harry Storbacka
*/

/// \addtogroup clanCore_Math clanCore Math
/// \{
#pragma once

template<typename Type>
class CL_Vec2;

template<typename Type>
class CL_Vec3;

/// \brief 2D vector
///
/// These vector templates are defined for:\n
/// char (CL_Vec2c), unsigned char (CL_Vec2uc), short (CL_Vec2s),\n
/// unsigned short (CL_Vec2us), int (CL_Vec2i), unsigned int (CL_Vec2ui), float (CL_Vec2f), double (CL_Vec2d)
/// \xmlonly !group=Core/Math! !header=core.h! \endxmlonly
template<typename Type>
class CL_Vec2
{
public:
	typedef Type datatype;

	union { Type x; Type s; Type r; };
	union { Type y; Type t; Type g; };

	CL_Vec2() : x(0), y(0) { }
	CL_Vec2(const CL_Vec3<Type> &copy) { x = copy.x; y = copy.y;}
	CL_Vec2(const Type &p1, const Type &p2) : x(p1), y(p2) { }

	CL_Vec2(const CL_Vec2<double> &copy);
	CL_Vec2(const CL_Vec2<float> &copy);
	CL_Vec2(const CL_Vec2<int> &copy);

/// \}
/// \name Operators
/// \{

public:
	const Type &operator[](unsigned int i) const { return ((Type *) this)[i]; }
	Type &operator[](unsigned int i) { return ((Type *) this)[i]; }
	operator Type *() { return (Type *) this; }
	operator Type * const() const { return (Type * const) this; }

	/// \brief += operator.
	void operator += (const CL_Vec2<Type>& vector) { x+= vector.x; y+= vector.y; }

	/// \brief += operator.
	void operator += ( Type value) { x+= value; y+= value; }

	/// \brief + operator.
	CL_Vec2<Type> operator + (const CL_Vec2<Type>& vector) const {return CL_Vec2<Type>(vector.x + x, vector.y + y);}

	/// \brief + operator.
	CL_Vec2<Type> operator + (Type value) const {return CL_Vec2<Type>(value + x, value + y);}

	/// \brief -= operator.
	void operator -= (const CL_Vec2<Type>& vector) { x-= vector.x; y-= vector.y; }

	/// \brief -= operator.
	void operator -= ( Type value) { x-= value; y-= value; }

	/// \brief - operator.
	CL_Vec2<Type> operator - (const CL_Vec2<Type>& vector) const {return CL_Vec2<Type>(x - vector.x, y - vector.y);}

	/// \brief - operator.
	CL_Vec2<Type> operator - (Type value) const {return CL_Vec2<Type>(x - value, y - value);}

	/// \brief - operator.
	CL_Vec2<Type> operator - () const {return CL_Vec2<Type>(-x , -y);}

	/// \brief *= operator.
	void operator *= (const CL_Vec2<Type>& vector) { x*= vector.x; y*= vector.y; }

	/// \brief *= operator.
	void operator *= ( Type value) { x*= value; y*= value; }

	/// \brief * operator.
	CL_Vec2<Type> operator * (const CL_Vec2<Type>& vector) const {return CL_Vec2<Type>(vector.x * x, vector.y * y);}

	/// \brief * operator.
	CL_Vec2<Type> operator * (Type value) const {return CL_Vec2<Type>(value * x, value * y);}

	/// \brief /= operator.
	void operator /= (const CL_Vec2<Type>& vector) { x/= vector.x; y/= vector.y; }

	/// \brief /= operator.
	void operator /= ( Type value) { x/= value; y/= value; }

	/// \brief / operator.
	CL_Vec2<Type> operator / (const CL_Vec2<Type>& vector) const {return CL_Vec2<Type>(x / vector.x, y / vector.y);}

	/// \brief / operator.
	CL_Vec2<Type> operator / (Type value) const {return CL_Vec2<Type>(x / value, y / value);}

	/// \brief = operator.
	CL_Vec2<Type> &operator = (const CL_Vec2<Type>& vector) { x = vector.x; y = vector.y; return *this; }

	/// \brief == operator.
	bool operator == (const CL_Vec2<Type>& vector) const {return ((x == vector.x) && (y == vector.y));}

	/// \brief != operator.
	bool operator != (const CL_Vec2<Type>& vector) const {return ((x != vector.x) || (y != vector.y));}
/// \}
};

typedef CL_Vec2<float> CL_Vec2f;