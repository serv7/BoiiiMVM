#pragma once
#include <cmath>

#include <string>
#include <format>

#define MATH_ATTRIBUTE static inline
MATH_ATTRIBUTE constexpr float PI = 3.1415926f;

MATH_ATTRIBUTE float clamp(float _val, float _min, float _max)
{
	_val = (_val + _max - fabs(_val - _max)) * 0.5f;
	_val = (_val + _min + fabs(_val - _min)) * 0.5f;
	return _val;
}

MATH_ATTRIBUTE int iclamp(int _val, int _min, int _max)
{
	if (_val < _min)
		_val = _min;
	if (_val > _max)
		_val = _max;
	return _val;
}

MATH_ATTRIBUTE int negative(float _val)
{ 
	union { float f; unsigned int i; } u;
	u.f = _val;
	return (int)(u.i >> 31);
}

MATH_ATTRIBUTE __int32 inc_mod(__int32 _val) { return _val + 1 & (_val - 2) >> 31; }
MATH_ATTRIBUTE __int32 dec_mod(__int32 _val) { return _val - 1 + ((_val - 1) >> 31 & 3); }

MATH_ATTRIBUTE float rad(float _deg) { return _deg * (PI / 180.0f); }
MATH_ATTRIBUTE float deg(float _rad) { return _rad * (180.0f / PI); }

MATH_ATTRIBUTE float inverse_sqrt(float _val) { return 1.0f / sqrt(_val); }
MATH_ATTRIBUTE float inverse_sqrt_safe(float _val) { return 1.0f / sqrt(_val + FLT_MIN); }

MATH_ATTRIBUTE float wrap(float _f, float _lo, float _hi)
{
	float _range = _hi - _lo;
	return _f - floor((_f - _lo) / _range) * _range;
}

template <typename _ty>
MATH_ATTRIBUTE _ty linear(const _ty& _y0, const _ty& _y1, float _alpha) { return _y0 + (_y1 - _y0) * _alpha; }

template <typename _ty>
MATH_ATTRIBUTE _ty cosine(const _ty& _y0, const _ty& _y1, float _alpha)
{
	float _mu = (1.0f - cos(_alpha * PI)) / 2.0f;
	return (_y0 * (1.0f - _mu) + _y1 * _mu);
}

template <typename _ty>
MATH_ATTRIBUTE _ty cubic(const _ty& _y0, const _ty& _y1, const _ty& _y2, const _ty& _y3, float _alpha)
{
	float _sqAlpha = _alpha * _alpha;
	_ty _v0 = _y3 - _y2 - _y0 + _y1;
	_ty _v1 = _y0 - _y1 - _v0;
	_ty _v2 = _y2 - _y0;
	_ty _v3 = _y1;

	return(_v0 * _alpha * _sqAlpha + _v1 * _sqAlpha + _v2 * _alpha + _v3);
}

template <typename _ty>
MATH_ATTRIBUTE _ty catmull_rom(const _ty& _y0, const _ty& _y1, const _ty& _y2, const _ty& _y3, float _alpha)
{
	_ty _v0 = _y0 * (-0.5) + _y1 * 1.5 - _y2 * 1.5 + _y3 * 0.5;
	_ty _v1 = _y0 - _y1 * (2.5) + _y2 * 2 - _y3 * 0.5;
	_ty _v2 = _y0 * (-0.5) + _y2 * 0.5;
	_ty _v3 = _y1;

	return _v0 * _alpha * _alpha * _alpha + _v1 * _alpha * _alpha + _v2 * _alpha + _v3;
}

struct vector2;
struct vector3;
struct vector4;
struct angles;
struct quaternion;
struct matrix33;
struct matrix34;
struct matrix44;

struct color;
struct plane;
struct aabb;

struct vector2
{
	float x, y;

	vector2() : x(0.0f), y(0.0f) {}
	vector2(const float _val) { x = _val; y = _val; }
	vector2(const float _x, const float _y) { x = _x; y = _y; }
	vector2(const vector2& _vec) { x = _vec.x; y = _vec.y; }

	void operator=(const float _val) { x = _val; y = _val; }
	void operator=(const vector2& _vec) { x = _vec.x; y = _vec.y; }

	bool operator==(const float& _val) const { return (x == _val && y == _val); }
	bool operator==(const vector2& _vec) const { return (x == _vec.x && y == _vec.y); }
	bool operator!=(const float& _val) const { return (x != _val && y != _val); }
	bool operator!=(const vector2& _vec) const { return (x != _vec.x && y != _vec.y); }

	vector2 operator-(void) const { return { -x, -y }; }

	vector2 operator+(const float _val)	const { return { x + _val, y + _val }; }
	vector2 operator+(const vector2& _vec) const { return { x + _vec.x, y + _vec.y }; }
	vector2 operator-(const float _val) const { return { x - _val, y - _val }; }
	vector2 operator-(const vector2& _vec) const { return { x - _vec.x, y - _vec.y }; }
	vector2 operator*(const float _val)	const { return { x * _val, y * _val }; }
	vector2 operator*(const vector2& _vec) const { return { x * _vec.x, y * _vec.y }; }
	vector2 operator/(const float _val) const { return { x / _val, y / _val }; }
	vector2 operator/(const vector2& _vec) const { return { x / _vec.x, y / _vec.y }; }

	void operator+=(const float _val) { x += _val; y += _val; }
	void operator+=(const vector2& _vec) { x += _vec.x; y += _vec.y; }
	void operator-=(const float _val) { x -= _val; y -= _val; }
	void operator-=(const vector2& _vec) { x -= _vec.x; y -= _vec.y; }
	void operator*=(const float _val) { x *= _val; y *= _val; }
	void operator*=(const vector2& _vec) { x *= _vec.x; y *= _vec.y; }
	void operator/=(const float _val) { x /= _val; y /= _val; }
	void operator/=(const vector2& _vec) { x /= _vec.x; y /= _vec.y; }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float operator[](int _index) const { return ((float*)this)[_index]; }

	const float* data() const { return &x; }

	bool zero(float e = (float)0.0) const { return (fabs(x) <= e) && (fabs(y) <= e); }
	bool null() const { return *this == 0.0f; }

	vector2 abs() const { return { fabs(x), fabs(y) }; }

	float distance(const vector2& _vec) const { return sqrt((x - _vec.x) * (x - _vec.x) + (y - _vec.y) * (y - _vec.y)); }

	std::string string() const { return std::format("{}, {}", x, y); }
	const char* c_str() const { return std::format("{}, {}", x, y).c_str(); }
};

struct vector3
{
	float x, y, z;

	vector3() { }
	vector3(const float _val) { x = _val; y = _val; z = _val; }
	vector3(const float _x, const float _y, const float _z) { x = _x; y = _y; z = _z; }
	vector3(const vector3& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; }
	vector3(const matrix34& _mat);
	vector3(const matrix44& _mat);

	vector3 operator=(const float _val) { x = _val; y = _val; z = _val; return *this; }
	vector3 operator=(const vector3& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; return *this; }

	bool operator==(const float& _val) const { return fabs(x - _val) < 0.00001 && fabs(y - _val) < 0.00001 && fabs(z - _val) < 0.00001; }
	bool operator==(const vector3& _vec) const { return fabs(x - _vec.x) < 0.00001 && fabs(y - _vec.y) < 0.00001 && fabs(z - _vec.z) < 0.00001; }
	bool operator!=(const float& _val) const { return (x != _val && y != _val && z != _val); }
	bool operator!=(const vector3& _vec) const { return (x != _vec.x && y != _vec.y && z != _vec.z); }

	vector3 operator-(void) const { return { -x, -y, -z }; }
	vector3 operator+(const float _val) const { return { x + _val, y + _val, z + _val }; }
	vector3 operator+(const vector3& _vec) const { return { x + _vec.x, y + _vec.y, z + _vec.z }; }
	vector3 operator-(const float _val) const { return { x - _val, y - _val, z - _val }; }
	vector3 operator-(const vector3& _vec) const { return { x - _vec.x, y - _vec.y, z - _vec.z }; }
	vector3 operator*(const float _val) const { return { x * _val, y * _val, z * _val }; }
	vector3 operator*(const vector3& _vec) const { return { x * _vec.x, y * _vec.y, z * _vec.z }; }
	vector3 operator/(const float _val) const { return { x / _val, y / _val, z / _val }; }
	vector3 operator/(const vector3& _vec) const { return { x / _vec.x, y / _vec.y, z / _vec.z }; }

	void operator+=(const float _val) { x += _val; y += _val; z += _val; }
	void operator+=(const vector3& _vec) { x += _vec.x; y += _vec.y; z += _vec.z; }
	void operator-=(const float _val) { x -= _val; y -= _val; z -= _val; }
	void operator-=(const vector3& _vec) { x -= _vec.x; y -= _vec.y; z -= _vec.z; }
	void operator*=(const float _val) { x *= _val; y *= _val; z *= _val; }
	void operator*=(const vector3& _vec) { x *= _vec.x; y *= _vec.y; z *= _vec.z; }
	void operator/=(const float _val) { x /= _val; y /= _val; z /= _val; }
	void operator/=(const vector3& _vec) { x /= _vec.x; y /= _vec.y; z /= _vec.z; }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float operator[](int _index) const { return ((float*)this)[_index]; }

	const float* data() const { return &x; }
	bool zero(float _epsilon = (float)0.0) const { return (fabs(x) <= _epsilon) && (fabs(y) <= _epsilon) && (fabs(z) <= _epsilon); }
	bool equivalent(const vector3& _vec, float _epsilon = 0.05f) const { return ((fabs(x - _vec.x) <= _epsilon) && (fabs(y - _vec.y) <= _epsilon) && (fabs(z - _vec.z) <= _epsilon)); }
	bool unit(float _epsilon = 0.05f) const { return (fabs(1.0f - length() * length()) <= _epsilon); }
	bool null() const { return *this == 0.0f; }

	float length() const { return sqrtf(x * x + y * y + z * z); }
	vector3 abs() const { return { fabs(x), fabs(y), fabs(z) }; }
	
	float volume() const { return x * y * z; }
	float distance(const vector3& _vec) const { return sqrtf((x - _vec.x) * (x - _vec.x) + (y - _vec.y) * (y - _vec.y) + (z - _vec.z) * (z - _vec.z)); }
	vector3 direction(const vector3& _vec) const { return vector3((_vec - *this) / (_vec - *this).length()).normalized(); }

	float dot(const vector3& _vec) const { return x * _vec.x + y * _vec.y + z * _vec.z; }
	float operator | (const vector3& _vec) const { return dot(_vec); }

	vector3 cross(const vector3& _vec) const { return vector3(y * _vec.z - z * _vec.y, z * _vec.x - x * _vec.z, x * _vec.y - y * _vec.x); }
	vector3 operator % (const vector3& _vec) const { return cross(_vec); }
	vector3 operator ^ (const vector3& _vec) const { return cross(_vec); }

	void normalize() { float _len = inverse_sqrt_safe(x * x + y * y + z * z); x *= _len; y *= _len; z *= _len; }
	vector3 normalized() const { vector3 _vec = *this; _vec.normalize(); return _vec; }

	static vector3 transform_point(const matrix33&, const vector3&);

	std::string string() const { return std::format("{}, {}, {}", x, y, z); }
	const char* c_str() const { return std::format("{}, {}, {}", x, y, z).c_str(); }
};

struct angles
{
	float x, y, z;

	angles() { }
	angles(const float _val) { x = _val; y = _val; z = _val; }
	angles(const float _x, const float _y, const float _z) { x = _x; y = _y; z = _z; }
	angles(const angles& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; }
	angles(const quaternion& _quat) { rotation_quaternion(_quat); }
	angles(const matrix33& _mat);
	angles(const matrix34& _mat);
	angles(const matrix44& _mat);

	angles operator=(const float _val) { x = _val; y = _val; z = _val; return *this; }
	angles operator=(const angles& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; return *this; }

	bool operator==(const float& _val) const { return fabs(x - _val) < 0.00001 && fabs(y - _val) < 0.00001 && fabs(z - _val) < 0.00001; }
	bool operator==(const angles& _vec) const { return fabs(x - _vec.x) < 0.00001 && fabs(y - _vec.y) < 0.00001 && fabs(z - _vec.z) < 0.00001; }
	bool operator!=(const float& _val) const { return (x != _val && y != _val && z != _val); }
	bool operator!=(const angles& _vec) const { return (x != _vec.x && y != _vec.y && z != _vec.z); }

	angles operator-(void) const { return { -x, -y, -z }; }
	angles operator+(const float _val) const { return { x + _val, y + _val, z + _val }; }
	angles operator+(const angles& _vec) const { return { x + _vec.x, y + _vec.y, z + _vec.z }; }
	angles operator-(const float _val) const { return { x - _val, y - _val, z - _val }; }
	angles operator-(const angles& _vec) const { return { x - _vec.x, y - _vec.y, z - _vec.z }; }
	angles operator*(const float _val) const { return { x * _val, y * _val, z * _val }; }
	angles operator*(const angles& _vec) const { return { x * _vec.x, y * _vec.y, z * _vec.z }; }
	angles operator/(const float _val) const { return { x / _val, y / _val, z / _val }; }
	angles operator/(const angles& _vec) const { return { x / _vec.x, y / _vec.y, z / _vec.z }; }

	void operator+=(const float _val) { x += _val; y += _val; z += _val; }
	void operator+=(const angles& _vec) { x += _vec.x; y += _vec.y; z += _vec.z; }
	void operator-=(const float _val) { x -= _val; y -= _val; z -= _val; }
	void operator-=(const angles& _vec) { x -= _vec.x; y -= _vec.y; z -= _vec.z; }
	void operator*=(const float _val) { x *= _val; y *= _val; z *= _val; }
	void operator*=(const angles& _vec) { x *= _vec.x; y *= _vec.y; z *= _vec.z; }
	void operator/=(const float _val) { x /= _val; y /= _val; z /= _val; }
	void operator/=(const angles& _vec) { x /= _vec.x; y /= _vec.y; z /= _vec.z; }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float operator[](int _index) const { return ((float*)this)[_index]; }

	angles dergees() const { float _pi = PI; return (*this) * (180.0f / _pi); }
	angles radians() const { float _pi = PI; return (*this) * (_pi / 180.0f); }

	vector3 direction() const { return vector3(-sinf(z) * cosf(x), cosf(z) * cosf(x), sinf(x)); }

	bool in_range() const
	{
		const float _pi = (float)(PI + 0.001);
		return ((x > -_pi) && (x < _pi) && (y > -_pi) && (y < _pi) && (z > -_pi) && (z < _pi));
	}

	void normalize()
	{
		const float _pi = PI;
		x = wrap(x, -_pi, _pi);
		y = wrap(y, -_pi, _pi);
		z = wrap(z, -_pi, _pi);
	}

	angles normalized() const
	{
		const float _pi = PI;
		return angles(wrap(x, -_pi, _pi), wrap(y, -_pi, _pi), wrap(z, -_pi, _pi));
	}

	void rotation_quaternion(const quaternion& _quat);
	void rotation_matrix(const matrix33& _mat);
	void rotation_matrix(const matrix34& _mat);
	void rotation_matrix(const matrix44& _mat);

	std::string string() const { return std::format("{}, {}, {}", x, y, z); }
	const char* c_str() const { return std::format("{}, {}, {}", x, y, z).c_str(); }
};

struct vector4
{
	float x, y, z, w;

	vector4() { }
	vector4(const float _val) { x = _val; y = _val; z = _val; w = _val; }
	vector4(const float _x, const float _y, const float _z, const float _w) { x = _x; y = _y; z = _z; w = _w; }
	vector4(const vector3& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; }
	vector4(const vector4& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; w = _vec.w; }
	vector4(const color& _col);

	vector4 operator=(const float _val) { x = _val; y = _val; z = _val; w = _val; return *this; }
	vector4 operator=(const vector4& _vec) { x = _vec.x; y = _vec.y; z = _vec.z; w = _vec.w; return *this; }

	bool operator==(const float& _val) const { return (x == _val && y == _val && z == _val && w == _val); }
	bool operator==(const vector4& _vec) const { return (x == _vec.x && y == _vec.y && z == _vec.z && w == _vec.w); }
	bool operator!=(const float& _val) const { return (x != _val && y != _val && z != _val && w != _val); }
	bool operator!=(const vector4& _vec) const { return (x != _vec.x && y != _vec.y && z != _vec.z != _vec.w); }

	vector4 operator-(void) const { return { -x, -y, -z, -w }; }
	vector4 operator!(void) const { return { -x, -y, -z, w }; }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float operator[](int _index) const { return ((float*)this)[_index]; }

	vector4 operator+(const float _val) const { return { x + _val, y + _val, z + _val, w + _val }; }
	vector4 operator+(const vector4& _vec) const { return { x + _vec.x, y + _vec.y, z + _vec.z, w + _vec.w }; }
	vector4 operator-(const float _val) const { return { x - _val, y - _val, z - _val, w - _val }; }
	vector4 operator-(const vector4& _vec) const { return { x - _vec.x, y - _vec.y, z - _vec.z, w - _vec.w }; }
	vector4 operator*(const float _val) const { return { x * _val, y * _val, z * _val, w * _val }; }
	vector4 operator*(const vector4& _vec) const { return { x * _vec.x, y * _vec.y, z * _vec.z, w * _vec.w }; }
	vector4 operator/(const float _val) const { return { x / _val, y / _val, z / _val, w / _val }; }
	vector4 operator/(const vector4& _vec) const { return { x / _vec.x, y / _vec.y, z / _vec.z , w / _vec.w }; }

	void operator+=(const float _val) { x += _val; y += _val; z += _val; w += _val; }
	void operator+=(const vector4& _vec) { x += _vec.x; y += _vec.y; z += _vec.z; w += _vec.w; }
	void operator-=(const float _val) { x -= _val; y -= _val; z -= _val; z -= _val; }
	void operator-=(const vector4& _vec) { x -= _vec.x; y -= _vec.y; z -= _vec.z; w -= _vec.w; }
	void operator*=(const float _val) { x *= _val; y *= _val; z *= _val; w *= _val; }
	void operator*=(const vector4& _vec) { x *= _vec.x; y *= _vec.y; z *= _vec.z; z *= _vec.w; }
	void operator/=(const float _val) { x /= _val; y /= _val; z /= _val; w /= _val; }
	void operator/=(const vector4& _vec) { x /= _vec.x; y /= _vec.y; z /= _vec.z; w /= _vec.w; }

	const float* data() const { return &x; }

	bool null() const { return *this == 0.0f; }
	bool identity() const { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f); }
	void identify() { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; }
	vector4 identificated() const { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

	std::string string() const { return std::format("{}, {}, {}, {}", x, y, z, w); }
	const char* c_str() const { return std::format("{}, {}, {}, {}", x, y, z, w).c_str(); }
};

struct quaternion
{
	float x, y, z, w;

	quaternion() { }
	quaternion(const float _val) { x = _val; y = _val; z = _val; w = _val; }
	quaternion(const float _x, const float _y, const float _z, const float _w) { x = _x; y = _y; z = _z; w = _w; }
	quaternion(const quaternion& _quat) { x = _quat.x; y = _quat.y; z = _quat.z; w = _quat.w; }
	quaternion(const float _yaw, const float _pitch, const float _roll) { rotation_yaw_pitch_roll(_yaw, _pitch, _roll); }
	quaternion(const angles& _ang) { rotation_yaw_pitch_roll(_ang); }
	quaternion(const matrix33& _mat);
	quaternion(const matrix34& _mat);
	quaternion(const matrix44& _mat);

	quaternion operator=(const float _val) { x = _val; y = _val; z = _val; w = _val; return *this; }
	quaternion operator=(const quaternion& _quat) { x = _quat.x; y = _quat.y; z = _quat.z; w = _quat.w; return *this; }

	bool operator==(const float& _val) const { return (x == _val && y == _val && z == _val && w == _val); }
	bool operator==(const quaternion& _quat) const { return (x == _quat.x && y == _quat.y && z == _quat.z && w == _quat.w); }
	bool operator!=(const float& _val) const { return (x != _val && y != _val && z != _val && w != _val); }
	bool operator!=(const quaternion& _quat) const { return (x != _quat.x && y != _quat.y && z != _quat.z != _quat.w); }

	quaternion operator-() const { return quaternion(-x, -y, -z, -w); }
	quaternion operator!() const { return quaternion(-x, -y, -z, w); }

	quaternion operator+(const float _val) const { return quaternion(x + _val, y + _val, z + _val, w + _val); }
	quaternion operator+(const quaternion& _quat) const { return quaternion(x + _quat.x, y + _quat.y, z + _quat.z, w + _quat.w); }
	quaternion operator-(const float _val) const { return quaternion(x - _val, y - _val, z - _val, w - _val); }
	quaternion operator-(const quaternion& _quat) const { return quaternion(x - _quat.x, y - _quat.y, z - _quat.z, w - _quat.w); }

	void operator+=(const float _val) { x += _val; y += _val; z += _val; w += _val; }
	void operator+=(const quaternion& _quat) { x += _quat.x; y += _quat.y; z += _quat.z; w += _quat.w; }
	void operator-=(const float _val) { x -= _val; y -= _val; z -= _val; z -= _val; }
	void operator-=(const quaternion& _quat) { x -= _quat.x; y -= _quat.y; z -= _quat.z; w -= _quat.w; }

	quaternion operator*(const float _val) { return quaternion(x * _val, y * _val, z * _val, w * _val); }
	quaternion operator*(const quaternion& _quat) 
	{	return quaternion((((z * _quat.y) - (y * _quat.z)) + (x * _quat.w)) + (_quat.x * w),
		((x * _quat.z) - (_quat.x * z) + (_quat.w * y)) + (w * _quat.y),
		(((_quat.x * y) - (x * _quat.y)) + (_quat.w * z)) + (w * _quat.z),
		(w * _quat.w) - (((_quat.x * x) + (y * _quat.y)) + (_quat.z * z))
		);
	}
	quaternion operator/(const quaternion& _quat) const { return quaternion(!_quat * (*this)); }

	void operator*=(const float _val) { *this = *this * _val; }
	void operator*=(const quaternion& _quat) { *this = *this * _quat; }
	void operator/=(const quaternion& _quat) { *this = *this / _quat; }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float operator[](int _index) const { return ((float*)this)[_index]; }

	const float* data() const { return &x; }

	bool null() const { return *this == 0.0f; }
	bool identity() const { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f); }
	void identify() { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; }
	quaternion identificated() const { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

	quaternion clean() const { return { x, y, z, 0.0f }; }
	quaternion additive(const quaternion& _m, const quaternion& _c) const { return { _c.x, _c.y, _c.z, _m.w }; }

	float dot(const quaternion& _quat) const { return x * _quat.x + y * _quat.y + z * _quat.z + w * _quat.w; }
	float operator | (const quaternion& _quat) const { return dot(_quat); }

	void normalize() {  float _d = inverse_sqrt(w * w + x * x + y * y + z * z); w *= _d; x *= _d; y *= _d; z *= _d; }
	quaternion normalized() const { quaternion _quat = *this; _quat.normalize(); return _quat; }

	void rotation_view_direction(const vector3& _vec)
	{
		w = (0.70710676908493042f);
		x = (_vec.z * 0.70710676908493042f);
		y = (0.0f);
		z = (0.0f);

		float l = sqrtf(_vec.x * _vec.x + _vec.y * _vec.y);
		if (l > (0.00001f))
		{
			vector3 hv;
			hv.x = _vec.x / l;
			hv.y = _vec.y / l + 1.0f;
			hv.z = l + 1.0f;

			float r = sqrtf(hv.x * hv.x + hv.y * hv.y);
			float s = sqrtf(hv.z * hv.z + _vec.z * _vec.z);
			float hacos0 = 0.0f;
			float hasin0 = -1.0f;

			if (r > (0.00001f))
			{
				hacos0 = hv.y / r;
				hasin0 = -hv.x / r;
			}
			float hacos1 = hv.z / s;
			float hasin1 = _vec.z / s;
			w = (hacos0 * hacos1);
			x = (hacos0 * hasin1);
			y = (hasin0 * hasin1);
			z = (hasin0 * hacos1);
		}
	}

	void rotation_yaw_pitch_roll(const float _yaw, const float _pitch, const float _roll)
	{
		float sx = sinf(_yaw * 0.5f);	float cx = cosf(_yaw * 0.5f);
		float sy = sinf(_pitch * 0.5f);	float cy = cosf(_pitch * 0.5f);
		float sz = sinf(_roll * 0.5f);	float cz = cosf(_roll * 0.5f);

		w = cx * cy * cz + sx * sy * sz;
		x = cz * cy * sx - sz * sy * cx;
		y = cz * sy * cx + sz * cy * sx;
		z = sz * cy * cx - cz * sy * sx;
	}

	void rotation_yaw_pitch_roll(const angles& _ang)
	{
		float sx = sinf(_ang.x * 0.5f), cx = cosf(_ang.x * 0.5f);
		float sy = sinf(_ang.y * 0.5f), cy = cosf(_ang.y * 0.5f);
		float sz = sinf(_ang.z * 0.5f), cz = cosf(_ang.z * 0.5f);

		w = cx * cy * cz + sx * sy * sz;
		x = cz * cy * sx - sz * sy * cx;
		y = cz * sy * cx + sz * cy * sx;
		z = sz * cy * cx - cz * sy * sx;
	}
	void rotation_matrix(const matrix33& _mat);
	void rotation_matrix(const matrix34& _mat);
	void rotation_matrix(const matrix44& _mat);

	std::string string() const { return std::format("{}, {}, {}, {}", x, y, z, w); }
	const char* c_str() const { return std::format("{}, {}, {}, {}", x, y, z, w).c_str(); }
};

struct matrix33
{
	float m00, m01, m02;
	float m10, m11, m12;
	float m20, m21, m22;

	matrix33()
	{
		m00 = 1; m01 = 0; m02 = 0;
		m10 = 0; m11 = 1; m12 = 0;
		m20 = 0; m21 = 0; m22 = 1;
	}
	matrix33(const float _yaw, const float _pitch, const float _roll) { rotation_yaw_pitch_roll(_yaw, _pitch, _roll); }
	matrix33(const angles& _ang) { rotation_yaw_pitch_roll(_ang); }
	matrix33(const quaternion& _quat) { rotation_quaternion(_quat); }
	matrix33(const matrix33& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;
		m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;
		m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;
	}
	matrix33(const matrix34& _mat);
	matrix33(const matrix44& _mat);

	vector3 operator[](int _index) const { return column(_index); }

	matrix33 identificated() const
	{
		matrix33 _m;
		_m.m00 = 1; _m.m01 = 0; _m.m02 = 0;
		_m.m10 = 0; _m.m11 = 1; _m.m12 = 0;
		_m.m20 = 0; _m.m21 = 0; _m.m22 = 1;
		return _m;
	}

	vector3 column(int _index) const
	{
		vector3 _vec;
		if (_index == 0)
			_vec = vector3(m00, m10, m20);
		if (_index == 1)
			_vec = vector3(m01, m11, m21);
		if (_index == 2)
			_vec = vector3(m02, m12, m22);
		return _vec;
	}

	int orthonormal(float _threshold = 0.001, bool _rh = true) const
	{
		int a, b, c;
		if (_rh) {
			a = column(0).equivalent(column(1) % column(2), _threshold);
			b = column(1).equivalent(column(2) % column(0), _threshold);
			c = column(2).equivalent(column(0) % column(1), _threshold);
			return a & b & c;
		}
		else {
			float _d0 = fabs(column(0) | column(1));
			if (_d0 > _threshold)
				return false;
			float _d1 = fabs(column(0) | column(2));
			if (_d1 > _threshold)
				return false;
			float _d2 = fabs(column(1) | column(2));
			if (_d2 > _threshold)
				return false;

			a = (fabs(1 - (column(0) | column(0)))) < _threshold;
			b = (fabs(1 - (column(1) | column(1)))) < _threshold;
			c = (fabs(1 - (column(2) | column(2)))) < _threshold;
		}
		return a & b & c;
	}

	vector3 direction() const { return vector3(m01, m11, m21); }

	void rotation_yaw_pitch_roll(const float _yaw, const float _pitch, const float _roll)
	{
		float sx = sinf(_yaw), cx = cosf(_yaw); 
		float sy = sinf(_pitch), cy = cosf(_pitch);
		float sz = sinf(_roll), cz = cosf(_roll);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m01 = sycz * sx - cx * sz;	m02 = sycz * cx + sx * sz;
		m10 = cy * sz;	m11 = sysz * sx + cx * cz;	m12 = sysz * cx - sx * cz;
		m20 = -sy;		m21 = cy * sx;				m22 = cy * cx;
	}

	void rotation_yaw_pitch_roll(const angles& _ang)
	{ 
		float sx = sinf(_ang.x), cx = cosf(_ang.x);
		float sy = sinf(_ang.y), cy = cosf(_ang.y);
		float sz = sinf(_ang.z), cz = cosf(_ang.z);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m01 = sycz * sx - cx * sz;	m02 = sycz * cx + sx * sz;
		m10 = cy * sz;	m11 = sysz * sx + cx * cz;	m12 = sysz * cx - sx * cz;
		m20 = -sy;		m21 = cy * sx;				m22 = cy * cx;
	}

	void rotation_quaternion(const quaternion& _quat)
	{
		vector3 _vec = vector3(_quat.x, _quat.y, _quat.z) * 2.0f;
		double xx = 1.0f - _vec.x * _quat.x;	double yy = _vec.y * _quat.y;		double xw = _vec.x * _quat.w;
		double xy = _vec.y * _quat.x;			double yz = _vec.z * _quat.y;		double yw = _vec.y * _quat.w;
		double xz = _vec.z * _quat.x;			double zz = _vec.z * _quat.z;		double zw = _vec.z * _quat.w;

		m00 = 1.0f - yy - zz;		m01 = xy - zw;			m02 = xz + yw;
		m10 = xy + zw;				m11 = xx - zz;			m12 = yz - xw;
		m20 = xz - yw;				m21 = yz + xw;			m22 = xx - yy;
	}
};

struct matrix34
{
	float m00, m01, m02, m03;
	float m10, m11, m12, m13;
	float m20, m21, m22, m23;

	matrix34()
	{
		m00 = 1; m01 = 0; m02 = 0; m03 = 0;
		m10 = 0; m11 = 1; m12 = 0; m13 = 0;
		m20 = 0; m21 = 0; m22 = 1; m23 = 0;
	}
	matrix34(const float _yaw, const float _pitch, const float _roll) { rotation_yaw_pitch_roll(_yaw, _pitch, _roll); }
	matrix34(const angles& _ang) { rotation_yaw_pitch_roll(_ang); }
	matrix34(const quaternion& _quat) { rotation_quaternion(_quat); }
	matrix34(const matrix33& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;		m03 = 0;
		m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;		m13 = 0;
		m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;		m23 = 0;
	}
	matrix34(const matrix34& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;		m03 = _mat.m03;
		m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;		m13 = _mat.m13;
		m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;		m23 = _mat.m23;
	}
	matrix34(const matrix44& _mat);

	const float* data() const { return &m00; }

	vector3 operator[](int _index) const { return column(_index); }

	matrix34 identificated() const
	{
		matrix34 _m;
		_m.m00 = 1; _m.m01 = 0; _m.m02 = 0;
		_m.m10 = 0; _m.m11 = 1; _m.m12 = 0;
		_m.m20 = 0; _m.m21 = 0; _m.m22 = 1;
		return _m;
	}

	vector3 column(int _index) const
	{
		vector3 _vec;
		if (_index == 0)
			_vec = vector3(m00, m10, m20);
		if (_index == 1)
			_vec = vector3(m01, m11, m21);
		if (_index == 2)
			_vec = vector3(m02, m12, m22);
		return _vec;
	}

	vector3 translation() const { return vector3(m03, m13, m23); }
	void translate(const vector3& _vec) { m03 = _vec.x, m13 = _vec.y, m23 = _vec.z; }

	vector3 direction() const { return vector3(m01, m11, m21); }

	void orthonormalize()
	{
		vector3 _x(m00, m10, m20);
		vector3 _y(m01, m11, m21);
		vector3 _z;
		_x = _x.normalized();
		_z = vector3(_x % _y).normalized();
		_y = vector3(_z % _x).normalized();
		m00 = _x.x;		m10 = _x.y;		m20 = _x.z;
		m01 = _y.x;		m11 = _y.y;		m21 = _y.z;
		m02 = _z.x;		m12 = _z.y;		m22 = _z.z;
	}
	matrix34 orthonormalized() { matrix34 _mat = *this; _mat.orthonormalize(); return _mat; }

	int orthonormal(float _threshold = 0.001f) const
	{
		int a, b, c;
		float _d0 = fabs(column(0) | column(1));
		if (_d0 > _threshold)
			return false;
		float _d1 = fabs(column(0) | column(2));
		if (_d1 > _threshold)
			return false;
		float _d2 = fabs(column(1) | column(2));
		if (_d2 > _threshold)
			return false;

		a = (fabs(1 - (column(0) | column(0)))) < _threshold;
		b = (fabs(1 - (column(1) | column(1)))) < _threshold;
		c = (fabs(1 - (column(2) | column(2)))) < _threshold;
		return a & b & c;
	}

	int orthonormal_rh(float _threshold = 0.001f) const
	{
		int a = column(0).equivalent(column(1) % column(2), _threshold);
		int b = column(1).equivalent(column(2) % column(0), _threshold);
		int c = column(2).equivalent(column(0) % column(1), _threshold);
		return a & b & c;
	}

	void rotation_yaw_pitch_roll(const float _yaw, const float _pitch, const float _roll)
	{
		float sx = sinf(_yaw), cx = cosf(_yaw);
		float sy = sinf(_pitch), cy = cosf(_pitch);
		float sz = sinf(_roll), cz = cosf(_roll);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m01 = sycz * sx - cx * sz;	m02 = sycz * cx + sx * sz;
		m10 = cy * sz;	m11 = sysz * sx + cx * cz;	m12 = sysz * cx - sx * cz;
		m20 = -sy;		m21 = cy * sx;				m22 = cy * cx;
	}

	void rotation_yaw_pitch_roll(const angles& _ang)
	{
		float sx = sinf(_ang.x), cx = cosf(_ang.x);
		float sy = sinf(_ang.y), cy = cosf(_ang.y);
		float sz = sinf(_ang.z), cz = cosf(_ang.z);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m01 = sycz * sx - cx * sz;	m02 = sycz * cx + sx * sz;
		m10 = cy * sz;	m11 = sysz * sx + cx * cz;	m12 = sysz * cx - sx * cz;
		m20 = -sy;		m21 = cy * sx;				m22 = cy * cx;
	}

	void rotation_quaternion(const quaternion& _quat)
	{
		vector3 _vec = vector3(_quat.x, _quat.y, _quat.z) * 2.0f;
		double xx = 1.0f - _vec.x * _quat.x;	double yy = _vec.y * _quat.y;		double xw = _vec.x * _quat.w;
		double xy = _vec.y * _quat.x;			double yz = _vec.z * _quat.y;		double yw = _vec.y * _quat.w;
		double xz = _vec.z * _quat.x;			double zz = _vec.z * _quat.z;		double zw = _vec.z * _quat.w;

		m00 = 1.0f - yy - zz;		m01 = xy - zw;			m02 = xz + yw;
		m10 = xy + zw;				m11 = xx - zz;			m12 = yz - xw;
		m20 = xz - yw;				m21 = yz + xw;			m22 = xx - yy;
	}
};

struct matrix44
{
	float m00, m01, m02, m03;
	float m10, m11, m12, m13;
	float m20, m21, m22, m23;
	float m30, m31, m32, m33;

	matrix44()
	{
		m00 = 1; m01 = 0; m02 = 0; m03 = 0;
		m10 = 0; m11 = 1; m12 = 0; m13 = 0;
		m20 = 0; m21 = 0; m22 = 1; m23 = 0;
		m30 = 0; m31 = 0; m32 = 0; m33 = 1;
	}
	matrix44(const float _yaw, const float _pitch, const float _roll) { rotation_yaw_pitch_roll(_yaw, _pitch, _roll); }
	matrix44(const angles& _ypr) { rotation_yaw_pitch_roll(_ypr); }
	matrix44(const quaternion& _quat) { rotation_quaternion(_quat); }
	matrix44(const matrix33& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;
		m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;
		m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;
	}
	matrix44(const matrix34& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m10;		m02 = _mat.m20;		m03 = 0;
		m10 = _mat.m01;		m11 = _mat.m11;		m12 = _mat.m21;		m13 = 0;
		m20 = _mat.m02;		m21 = _mat.m12;		m22 = _mat.m22;		m23 = 0;
		m30 = _mat.m03;		m31 = _mat.m13;		m32 = _mat.m23;		m33 = 1;
	}
	matrix44(const matrix44& _mat)
	{
		m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;		m03 = _mat.m03;
		m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;		m13 = _mat.m13;
		m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;		m23 = _mat.m23;
		m30 = _mat.m30;		m31 = _mat.m31;		m32 = _mat.m32;		m33 = _mat.m33;
	}

	float* data() { return &m00; }
	const float* data() const { return &m00; }

	static matrix44 identificated()
	{
		matrix44 m;
		m.m00 = 1; m.m01 = 0; m.m02 = 0; m.m03 = 0;
		m.m10 = 0; m.m11 = 1; m.m12 = 0; m.m13 = 0;
		m.m20 = 0; m.m21 = 0; m.m22 = 1; m.m23 = 0;
		m.m30 = 0; m.m31 = 0; m.m32 = 0; m.m33 = 1;
		return m;
	}

	void identify()
	{
		m00 = 1; m01 = 0; m02 = 0; m03 = 0;
		m10 = 0; m11 = 1; m12 = 0; m13 = 0;
		m20 = 0; m21 = 0; m22 = 1; m23 = 0;
		m30 = 0; m31 = 0; m32 = 0; m33 = 1;
	}

	vector3 row(int _index) const { return *(vector3*)(&m00 + 4 * _index); }
	vector4 vec4(int _index) const { return *(vector4*)(&m00 + 4 * _index); }

	float& operator[](int _index) { return ((float*)(this))[_index]; }
	const float& operator[](int _index) const { return ((float*)(this))[_index]; }

	matrix44 operator*(const matrix44& _mat) const
	{
		matrix44 _result;
		_result.m00 = m00 * _mat.m00 + m01 * _mat.m10 + m02 * _mat.m20 + m03 * _mat.m30;
		_result.m10 = m10 * _mat.m00 + m11 * _mat.m10 + m12 * _mat.m20 + m13 * _mat.m30;
		_result.m20 = m20 * _mat.m00 + m21 * _mat.m10 + m22 * _mat.m20 + m23 * _mat.m30;
		_result.m30 = m30 * _mat.m00 + m31 * _mat.m10 + m32 * _mat.m20 + m33 * _mat.m30;
		_result.m01 = m00 * _mat.m01 + m01 * _mat.m11 + m02 * _mat.m21 + m03 * _mat.m31;
		_result.m11 = m10 * _mat.m01 + m11 * _mat.m11 + m12 * _mat.m21 + m13 * _mat.m31;
		_result.m21 = m20 * _mat.m01 + m21 * _mat.m11 + m22 * _mat.m21 + m23 * _mat.m31;
		_result.m31 = m30 * _mat.m01 + m31 * _mat.m11 + m32 * _mat.m21 + m33 * _mat.m31;
		_result.m02 = m00 * _mat.m02 + m01 * _mat.m12 + m02 * _mat.m22 + m03 * _mat.m32;
		_result.m12 = m10 * _mat.m02 + m11 * _mat.m12 + m12 * _mat.m22 + m13 * _mat.m32;
		_result.m22 = m20 * _mat.m02 + m21 * _mat.m12 + m22 * _mat.m22 + m23 * _mat.m32;
		_result.m32 = m30 * _mat.m02 + m31 * _mat.m12 + m32 * _mat.m22 + m33 * _mat.m32;
		_result.m03 = m00 * _mat.m03 + m01 * _mat.m13 + m02 * _mat.m23 + m03 * _mat.m33;
		_result.m13 = m10 * _mat.m03 + m11 * _mat.m13 + m12 * _mat.m23 + m13 * _mat.m33;
		_result.m23 = m20 * _mat.m03 + m21 * _mat.m13 + m22 * _mat.m23 + m23 * _mat.m33;
		_result.m33 = m30 * _mat.m03 + m31 * _mat.m13 + m32 * _mat.m23 + m33 * _mat.m33;
		return _result;
	}

	void transpose()
	{
		matrix44 _mat = *this;
		m00 = _mat.m00;	m01 = _mat.m10;	m02 = _mat.m20;	m03 = _mat.m30;
		m10 = _mat.m01;	m11 = _mat.m11;	m12 = _mat.m21;	m13 = _mat.m31;
		m20 = _mat.m02;	m21 = _mat.m12;	m22 = _mat.m22;	m23 = _mat.m32;
		m30 = _mat.m03;	m31 = _mat.m13;	m32 = _mat.m23;	m33 = _mat.m33;
	}

	void transpose(const matrix44& _src)
	{
		m00 = _src.m00;	m01 = _src.m10;	m02 = _src.m20;	m03 = _src.m30;
		m10 = _src.m01;	m11 = _src.m11;	m12 = _src.m21;	m13 = _src.m31;
		m20 = _src.m02;	m21 = _src.m12;	m22 = _src.m22;	m23 = _src.m32;
		m30 = _src.m03;	m31 = _src.m13;	m32 = _src.m23;	m33 = _src.m33;
	}

	matrix44 transposed() const
	{
		matrix44 _mat = *this;
		_mat.transpose();
		return _mat;
	}

	matrix44 transposed(const matrix44& _src) const
	{
		matrix44 _mat = _src;
		_mat.transpose();
		return _mat;
	}

	void invert(const matrix44& _mat)
	{
		float tmp[12];

		/* calculate pairs for first 8 elements (cofactors) */
		tmp[0] = _mat.m22 * _mat.m33;
		tmp[1] = _mat.m32 * _mat.m23;
		tmp[2] = _mat.m12 * _mat.m33;
		tmp[3] = _mat.m32 * _mat.m13;
		tmp[4] = _mat.m12 * _mat.m23;
		tmp[5] = _mat.m22 * _mat.m13;
		tmp[6] = _mat.m02 * _mat.m33;
		tmp[7] = _mat.m32 * _mat.m03;
		tmp[8] = _mat.m02 * _mat.m23;
		tmp[9] = _mat.m22 * _mat.m03;
		tmp[10] = _mat.m02 * _mat.m13;
		tmp[11] = _mat.m12 * _mat.m03;

		/* calculate first 8 elements (cofactors) */
		m00 = tmp[0] * _mat.m11 + tmp[3] * _mat.m21 + tmp[4] * _mat.m31;
		m00 -= tmp[1] * _mat.m11 + tmp[2] * _mat.m21 + tmp[5] * _mat.m31;
		m01 = tmp[1] * _mat.m01 + tmp[6] * _mat.m21 + tmp[9] * _mat.m31;
		m01 -= tmp[0] * _mat.m01 + tmp[7] * _mat.m21 + tmp[8] * _mat.m31;
		m02 = tmp[2] * _mat.m01 + tmp[7] * _mat.m11 + tmp[10] * _mat.m31;
		m02 -= tmp[3] * _mat.m01 + tmp[6] * _mat.m11 + tmp[11] * _mat.m31;
		m03 = tmp[5] * _mat.m01 + tmp[8] * _mat.m11 + tmp[11] * _mat.m21;
		m03 -= tmp[4] * _mat.m01 + tmp[9] * _mat.m11 + tmp[10] * _mat.m21;
		m10 = tmp[1] * _mat.m10 + tmp[2] * _mat.m20 + tmp[5] * _mat.m30;
		m10 -= tmp[0] * _mat.m10 + tmp[3] * _mat.m20 + tmp[4] * _mat.m30;
		m11 = tmp[0] * _mat.m00 + tmp[7] * _mat.m20 + tmp[8] * _mat.m30;
		m11 -= tmp[1] * _mat.m00 + tmp[6] * _mat.m20 + tmp[9] * _mat.m30;
		m12 = tmp[3] * _mat.m00 + tmp[6] * _mat.m10 + tmp[11] * _mat.m30;
		m12 -= tmp[2] * _mat.m00 + tmp[7] * _mat.m10 + tmp[10] * _mat.m30;
		m13 = tmp[4] * _mat.m00 + tmp[9] * _mat.m10 + tmp[10] * _mat.m20;
		m13 -= tmp[5] * _mat.m00 + tmp[8] * _mat.m10 + tmp[11] * _mat.m20;

		/* calculate pairs for second 8 elements (cofactors) */
		tmp[0] = _mat.m20 * _mat.m31;
		tmp[1] = _mat.m30 * _mat.m21;
		tmp[2] = _mat.m10 * _mat.m31;
		tmp[3] = _mat.m30 * _mat.m11;
		tmp[4] = _mat.m10 * _mat.m21;
		tmp[5] = _mat.m20 * _mat.m11;
		tmp[6] = _mat.m00 * _mat.m31;
		tmp[7] = _mat.m30 * _mat.m01;
		tmp[8] = _mat.m00 * _mat.m21;
		tmp[9] = _mat.m20 * _mat.m01;
		tmp[10] = _mat.m00 * _mat.m11;
		tmp[11] = _mat.m10 * _mat.m01;

		/* calculate second 8 elements (cofactors) */
		m20 = tmp[0] * _mat.m13 + tmp[3] * _mat.m23 + tmp[4] * _mat.m33;
		m20 -= tmp[1] * _mat.m13 + tmp[2] * _mat.m23 + tmp[5] * _mat.m33;
		m21 = tmp[1] * _mat.m03 + tmp[6] * _mat.m23 + tmp[9] * _mat.m33;
		m21 -= tmp[0] * _mat.m03 + tmp[7] * _mat.m23 + tmp[8] * _mat.m33;
		m22 = tmp[2] * _mat.m03 + tmp[7] * _mat.m13 + tmp[10] * _mat.m33;
		m22 -= tmp[3] * _mat.m03 + tmp[6] * _mat.m13 + tmp[11] * _mat.m33;
		m23 = tmp[5] * _mat.m03 + tmp[8] * _mat.m13 + tmp[11] * _mat.m23;
		m23 -= tmp[4] * _mat.m03 + tmp[9] * _mat.m13 + tmp[10] * _mat.m23;
		m30 = tmp[2] * _mat.m22 + tmp[5] * _mat.m32 + tmp[1] * _mat.m12;
		m30 -= tmp[4] * _mat.m32 + tmp[0] * _mat.m12 + tmp[3] * _mat.m22;
		m31 = tmp[8] * _mat.m32 + tmp[0] * _mat.m02 + tmp[7] * _mat.m22;
		m31 -= tmp[6] * _mat.m22 + tmp[9] * _mat.m32 + tmp[1] * _mat.m02;
		m32 = tmp[6] * _mat.m12 + tmp[11] * _mat.m32 + tmp[3] * _mat.m02;
		m32 -= tmp[10] * _mat.m32 + tmp[2] * _mat.m02 + tmp[7] * _mat.m12;
		m33 = tmp[10] * _mat.m22 + tmp[4] * _mat.m02 + tmp[9] * _mat.m12;
		m33 -= tmp[8] * _mat.m12 + tmp[11] * _mat.m22 + tmp[5] * _mat.m02;

		//devide the cofactor-matrix by the determinat
		float _invDet = 1.0f / (_mat.m00 * m00 + _mat.m10 * m01 + _mat.m20 * m02 + _mat.m30 * m03);
		m00 *= _invDet; m01 *= _invDet; m02 *= _invDet; m03 *= _invDet;
		m10 *= _invDet; m11 *= _invDet; m12 *= _invDet; m13 *= _invDet;
		m20 *= _invDet; m21 *= _invDet; m22 *= _invDet; m23 *= _invDet;
		m30 *= _invDet; m31 *= _invDet; m32 *= _invDet; m33 *= _invDet;
	}

	void invert() { invert(*this); }
	matrix44 inverted(const matrix44& _mat) const { matrix44 _inv = *this; _inv.invert(_mat); return _inv; }
	matrix44 inverted() const { matrix44 _inv = *this; _inv.invert(); return _inv; }

	vector4 transform_point(const vector3& _vec) const
	{ 
		return vector4(
			_vec.x * m00 + _vec.y * m10 + _vec.z * m20 + m30,
			_vec.x * m01 + _vec.y * m11 + _vec.z * m21 + m31,
			_vec.x * m02 + _vec.y * m12 + _vec.z * m22 + m32,
			_vec.x * m03 + _vec.y * m13 + _vec.z * m23 + m33
		);
	}

	float determinant() const { return (m00 * m11 * m22) + (m01 * m12 * m20) + (m02 * m10 * m21) - (m02 * m11 * m20) - (m00 * m12 * m21) - (m01 * m10 * m22); }

	vector3 translation() const { return vector3(m30, m31, m32); }
	void translate(const vector3& _vec) { m30 = _vec.x; m31 = _vec.y; m32 = _vec.z; }

	vector3 direction() const { return vector3(m10, m11, m12); }

	void rotation_yaw_pitch_roll(const float _yaw, const float _pitch, const float _roll)
	{
		float sx = sinf(_yaw), cx = cosf(_yaw);
		float sy = sinf(_pitch), cy = cosf(_pitch);
		float sz = sinf(_roll), cz = cosf(_roll);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m10 = sycz * sx - cx * sz;	m20 = sycz * cx + sx * sz;
		m01 = cy * sz;	m11 = sysz * sx + cx * cz;	m21 = sysz * cx - sx * cz;
		m02 = -sy;		m12 = cy * sx;				m22 = cy * cx;
	}

	void rotation_yaw_pitch_roll(const angles& _ang)
	{
		float sx = sinf(_ang.x), cx = cosf(_ang.x);
		float sy = sinf(_ang.y), cy = cosf(_ang.y);
		float sz = sinf(_ang.z), cz = cosf(_ang.z);

		float sycz = (sy * cz), sysz = (sy * sz);
		m00 = cy * cz;	m10 = sycz * sx - cx * sz;	m20 = sycz * cx + sx * sz;
		m01 = cy * sz;	m11 = sysz * sx + cx * cz;	m21 = sysz * cx - sx * cz;
		m02 = -sy;		m12 = cy * sx;				m22 = cy * cx;
	}

	void rotation_quaternion(const quaternion& _quat)
	{
		vector3 _vec = vector3(_quat.x + _quat.x, _quat.y + _quat.y, _quat.z + _quat.z);
		float xx = 1.0f - _vec.x * _quat.x;	float yy = _vec.y * _quat.y;		float xw = _vec.x * _quat.w;
		float xy = _vec.y * _quat.x;		float yz = _vec.z * _quat.y;		float yw = _vec.y * _quat.w;
		float xz = _vec.z * _quat.x;		float zz = _vec.z * _quat.z;		float zw = _vec.z * _quat.w;

		m00 = 1.0f - yy - zz;		m10 = xy - zw;			m20 = xz + yw;
		m01 = xy + zw;				m11 = xx - zz;			m21 = yz - xw;
		m02 = xz - yw;				m12 = yz + xw;			m22 = xx - yy;
	}

	void rotation_view_direction(const vector3& _vec)
	{
		m00 = 1;		m01 = 0;		m02 = 0;
		m10 = 0;		m11 = 0;		m12 = _vec.z;
		m20 = 0;		m21 = -_vec.z;	m22 = 0;
		float _lenght = sqrtf(_vec.x * _vec.x + _vec.y * _vec.y);
		if (_lenght > 0.0001) {
			float _xl = -_vec.x / _lenght; float _yl = _vec.y / _lenght;
			m00 = (_yl);			m01 = (_xl);			m02 = 0;
			m10 = (_vec.x);			m11 = (_vec.y);			m12 = (_vec.z);
			m20 = (_xl * _vec.z);	m21 = (-_vec.z * _yl);	m22 = (_lenght);
		}
	}
};

struct color
{
	float r, g, b, a;

	color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
	color(float _r, float _g, float _b, float _a = 1.0f) { r = _r; g = _g; b = _b; a = _a; }
	color(const vector3& _col, float _a = 1.0f) { r = _col.x; g = _col.y; b = _col.z; a = _a; };
	color(const vector4& _col) { r = _col.x; g = _col.y; b = _col.z; a = _col.w; };
	color(const color& _col) { r = _col.r; g = _col.g; b = _col.b; a = _col.a; };

	color operator=(const vector3& _col) { r = _col.x; g = _col.y; b = _col.z; a = 1.0f; return *this; }
	color operator=(const vector4& _col) { r = _col.x; g = _col.y; b = _col.z; a = _col.w; return *this; }
	color operator=(const color& _col) { r = _col.r; g = _col.g; b = _col.b; a = _col.a; return *this; }

	bool operator==(const color& _col) const { return (r == _col.r) && (g == _col.g) && (b == _col.b) && (a == _col.a); }
	bool operator!=(const color& _col) const { return (r != _col.r) || (g != _col.g) || (b != _col.b) || (a != _col.a); }

	void operator+(float _val) { r += _val; g += _val; b += _val; a += _val; }
	void operator+(const color& _col) { r += _col.r; g += _col.g; b += _col.b; a += _col.a; }
	void operator-(float _val) { r -= _val; g -= _val; b -= _val; a -= _val; }
	void operator-(const color& _col) { r -= _col.r; g -= _col.g; b -= _col.b; a -= _col.a; }
	void operator*(float _val) { r *= _val; g *= _val; b *= _val; a *= _val; }
	void operator*(const color& _col) { r *= _col.r; g *= _col.g; b *= _col.b; a *= _col.a; }
	void operator/(float _val) { _val = 1.0f / _val; r *= _val; g *= _val; b *= _val; a *= _val; }
	void operator/(const color& _col) { r /= _col.r; g /= _col.g; b /= _col.b; a /= _col.a; }

	color operator+=(float _val) { return color(r + _val, g + _val, b + _val, a + _val); }
	color operator+=(const color& _col) { return color(r + _col.r, g + _col.r, b + _col.r, a + _col.r); }
	color operator-=(float _val) { return color(r - _val, g - _val, b - _val, a - _val); }
	color operator-=(const color& _col) { return color(r - _col.r, g - _col.r, b - _col.r, a - _col.r); }
	color operator*=(float _val) { return color(r * _val, g * _val, b * _val, a * _val); }
	color operator*=(const color& _col) { return color(r * _col.r, g * _col.r, b * _col.r, a * _col.r); }
	color operator/=(float _val) { _val = 1.0f / _val; return color(r * _val, g * _val, b * _val, a * _val); }
	color operator/=(const color& _col) { return color(r / _col.r, g / _col.r, b / _col.r, a / _col.r); }

	float& operator[](int _index) { return ((float*)this)[_index]; }
	float  operator[](int _index) const { ((float*)this)[_index]; }

	color operator-() const { return color(-r, -g, -b, -a); }

	float luminance() const { return r * 0.30f + g * 0.59f + b * 0.11f; }

	float maximum() const { return fmax(r, fmax(b, g)); }

	void clamp(float _min = 0.0f, float _max = 1.0f) { r = ::clamp(r, _min, _max); g = ::clamp(g, _min, _max); b = ::clamp(b, _min, _max); a = ::clamp(a, _min, _max); }
	color clamped() const { color _col = *this; _col.clamp(); return _col; }

	void normalize()
	{
		float _max = maximum();
		if (_max == 0.0f)
			return;

		*this /= _max;
	}

	color normalized() const { color _col = *this; _col.normalize(); return _col; }
};

struct plane
{
	vector3 n;
	float d;

	plane() : n(0.0f), d(0.0f) { }
	plane(const plane& _pl) { n = _pl.n; d = _pl.d; }
	plane(const vector3& _normal, const float _dist) { n = _normal; d = _dist; }

	plane operator=(const plane& _pl) { n = _pl.n; d = _pl.d; return *this; }

	plane operator -() const { return plane(-n, -d); }

	void setup(const vector3& _normal, const vector3& _point) { n = _normal; d = -(_point | _normal); }
	void setup(const vector3& _vec0, const vector3& _vec1, const vector3& _vec2)
	{ 
		n = vector3(vector3(_vec1 - _vec0) % vector3(_vec2 - _vec0)).normalized();
		d = -(n | _vec0);
	}

	float operator|(const vector3& _point) const { return (n | _point) + d; }

	plane operator+(const plane& _pl) const { return plane(n + _pl.n, d + _pl.d); }
	plane operator-(const plane& _pl) const { return plane(n - _pl.n, d - _pl.d); }

	void operator+=(const plane& _pl) { n += _pl.n, d += _pl.d; }
	void operator-=(const plane& _pl) { n -= _pl.n, d -= _pl.d; }

	plane operator+(const float _val) const { return plane(n + _val, d + _val); }
	plane operator-(const float _val) const { return plane(n - _val, d - _val); }
	void operator+=(const float _val) { n += _val, d += _val; }
	void operator-=(const float _val) { n -= _val, d -= _val; }

	bool operator==(const plane& _pl)
	{
		if (fabsf(n.x - _pl.n.x) > 0.0001f) return (false);
		if (fabsf(n.y - _pl.n.y) > 0.0001f) return (false);
		if (fabsf(n.z - _pl.n.z) > 0.0001f) return (false);
		if (fabsf(d - _pl.d) < 0.01f) return(true);
		return (false);
	}

	vector3 mirrored_vector(const vector3& _i) { return n * (2 * (n | _i)) - _i; }
	vector3 mirrored_position(const vector3& _i) { return _i - n * (2 * ((n | _i) + d)); }

	static plane create_plane(const vector3& _normal, const vector3& _point)
	{ 
		plane _pl; 
		_pl.setup(_normal, _point);
		return _pl; 
	}
	static plane create_plane(const vector3& _vec0, const vector3& _vec1, const vector3& _vec2)
	{ 
		plane _pl; 
		_pl.setup(_vec0, _vec1, _vec2);
		return _pl;
	}
};

struct aabb
{
	vector3 aa;
	vector3 bb;

	aabb() { aa = bb = 0.0f; }
	aabb(float _radius) { bb = vector3(_radius); aa = -bb; }
	aabb(const vector3& _vec) { aa = bb = _vec; }
	aabb(const vector3& _vec, float _radius) { aa = _vec - _radius; bb = _vec + _radius; }
	aabb(const vector3& vaa, const vector3& vbb) { aa = vaa; bb = vbb; }

	vector3 center() const { return (aa + bb) * 0.5f; }

	bool intersected(const aabb& _box) const
	{ 
		if ((aa.x > _box.bb.x) || (_box.aa.x > bb.x)) return false;
		if ((aa.y > _box.bb.y) || (_box.aa.y > bb.y)) return false;
		if ((aa.z > _box.bb.z) || (_box.aa.z > bb.z)) return false;
		return true;
	}
};

inline static vector3 operator*(const matrix33& m, const vector3& _point)
{
	vector3 _vec;
	_vec.x = m.m00 * _point.x + m.m01 * _point.y + m.m02 * _point.z;
	_vec.y = m.m10 * _point.x + m.m11 * _point.y + m.m12 * _point.z;
	_vec.z = m.m20 * _point.x + m.m21 * _point.y + m.m22 * _point.z;
	return _vec;
}

inline static vector3 operator*(const matrix34& _mat, const vector3& _point)
{
	vector3 _vec;
	_vec.x = _mat.m00 * _point.x + _mat.m01 * _point.y + _mat.m02 * _point.z + _mat.m03;
	_vec.y = _mat.m10 * _point.x + _mat.m11 * _point.y + _mat.m12 * _point.z + _mat.m13;
	_vec.z = _mat.m20 * _point.x + _mat.m21 * _point.y + _mat.m22 * _point.z + _mat.m23;
	return _vec;
}

MATH_ATTRIBUTE void fill_matrix(float* _mat, const float* _val, size_t _size)
{
	if (_mat && _val && _size > NULL) {
		for (int i = 0; i < _size - 1; i++)
			_mat[i] = _val[i];
	}
}

#include <typeinfo>

template <typename _ty> MATH_ATTRIBUTE std::string string(const _ty& _v)
{
	std::string _fmt;
	float* _data = (float*)&_v;
	size_t _size = sizeof(_ty) / sizeof(float);
	for (int _i = 0; _i < _size; _i++)
		_fmt += std::to_string(_data[_i]) + ((_i < _size - 1) ? ", " : "");
	return _fmt;
}

template <typename _ty> MATH_ATTRIBUTE std::string string_t(const _ty& _v)
{
	std::string _fmt = typeid(_ty).name() + std::string(": ");
	float* _data = (float*)&_v;
	size_t _size = sizeof(_ty) / sizeof(float);
	for (int _i = 0; _i < _size; _i++)
		_fmt += std::to_string(_data[_i]) + ((_i < _size - 1) ? ", " : "");
	return _fmt;
}

template <typename _ty> MATH_ATTRIBUTE const char* c_str(const _ty& _v) { return string(_v).c_str(); }
template <typename _ty> MATH_ATTRIBUTE const char* c_str_t(const _ty& _v) { return string_t(_v).c_str(); }