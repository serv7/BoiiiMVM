#include "math.h"

angles::angles(const matrix33& _mat) { rotation_matrix(_mat); }
angles::angles(const matrix34& _mat) { rotation_matrix(_mat); }
angles::angles(const matrix44& _mat) { rotation_matrix(_mat); }

void angles::rotation_quaternion(const quaternion& _quat)
{
	y = asinf(fmax(-1.0f, fmin(1.0f, -(_quat.x * _quat.z - _quat.w * _quat.y) * 2.0f)));
	if (fabsf(fabsf(y) - ((PI * 0.5f)) < 0.01f))
	{
		x = 0.0f;
		z = atan2f(-2.0f * (_quat.x * _quat.y - _quat.w * _quat.z), 1.0f - (_quat.x * _quat.x + _quat.z * _quat.z) * 2.0f);
	}
	else
	{
		x = atan2f((_quat.y * _quat.z + _quat.w * _quat.x) * 2.0f, 1.0f - (_quat.x * _quat.x + _quat.y * _quat.y) * 2.0f);
		z = atan2f((_quat.x * _quat.y + _quat.w * _quat.z) * 2.0f, 1.0f - (_quat.z * _quat.z + _quat.y * _quat.y) * 2.0f);
	}
}

void angles::rotation_matrix(const matrix33& _mat)
{
	y = asinf(fmax(-1.0f, fmin(1.0f, -_mat.m20)));
	if (fabs(fabs(y) - (PI * 0.5f)) < 0.01f)
	{
		x = 0.0f;
		z = atan2f(-_mat.m01, _mat.m11);
	}
	else
	{
		x = atan2f(_mat.m21, _mat.m22);
		z = atan2f(_mat.m10, _mat.m00);
	}
}

void angles::rotation_matrix(const matrix34& _mat)
{
	y = asinf(fmax(-1.0f, fmin(1.0f, -_mat.m20)));
	if (fabs(fabs(y) - (PI * 0.5f)) < 0.01f)
	{
		x = 0.0f;
		z = atan2f(-_mat.m01, _mat.m11);
	}
	else
	{
		x = atan2f(_mat.m21, _mat.m22);
		z = atan2f(_mat.m10, _mat.m00);
	}
}

void angles::rotation_matrix(const matrix44& _mat)
{
	y = asinf(fmax(-1.0f, fmin(1.0f, -_mat.m02)));
	if (fabs(fabs(y) - (PI * 0.5f)) < 0.01f)
	{
		x = 0.0f;
		z = atan2f(-_mat.m10, _mat.m11);
	}
	else
	{
		x = atan2f(_mat.m12, _mat.m22);
		z = atan2f(_mat.m01, _mat.m00);
	}
}

quaternion::quaternion(const matrix33& _mat) { rotation_matrix(_mat); }
quaternion::quaternion(const matrix34& _mat) { rotation_matrix(_mat); }
quaternion::quaternion(const matrix44& _mat) { rotation_matrix(_mat); }

void quaternion::rotation_matrix(const matrix33& _mat)
{
	float s, p, tr = _mat.m00 + _mat.m11 + _mat.m22;
	w = 1, x = 0, y = 0, z = 0;
	if (tr > 0)
	{
		s = sqrtf(tr + 1.0f), p = 0.5f / s, w = s * 0.5f, x = (_mat.m21 - _mat.m12) * p, y = (_mat.m02 - _mat.m20) * p, z = (_mat.m10 - _mat.m01) * p;
	}
	else if ((_mat.m00 >= _mat.m11) && (_mat.m00 >= _mat.m22))
	{
		s = sqrtf(_mat.m00 - _mat.m11 - _mat.m22 + 1.0f), p = 0.5f / s, w = (_mat.m21 - _mat.m12) * p, x = s * 0.5f, y = (_mat.m10 + _mat.m01) * p, z = (_mat.m20 + _mat.m02) * p;
	}
	else if ((_mat.m11 >= _mat.m00) && (_mat.m11 >= _mat.m22))
	{
		s = sqrtf(_mat.m11 - _mat.m22 - _mat.m00 + 1.0f), p = 0.5f / s, w = (_mat.m02 - _mat.m20) * p, x = (_mat.m01 + _mat.m10) * p, y = s * 0.5f, z = (_mat.m21 + _mat.m12) * p;
	}
	else if ((_mat.m22 >= _mat.m00) && (_mat.m22 >= _mat.m11))
	{
		s = sqrtf(_mat.m22 - _mat.m00 - _mat.m11 + 1.0f), p = 0.5f / s, w = (_mat.m10 - _mat.m01) * p, x = (_mat.m02 + _mat.m20) * p, y = (_mat.m12 + _mat.m21) * p, z = s * 0.5f;
	}
}

void quaternion::rotation_matrix(const matrix34& _mat)
{
	float s, p, tr = _mat.m00 + _mat.m11 + _mat.m22;
	w = 1, x = 0, y = 0, z = 0;
	if (tr > 0)
	{
		s = sqrtf(tr + 1.0f), p = 0.5f / s, w = s * 0.5f, x = (_mat.m21 - _mat.m12) * p, y = (_mat.m02 - _mat.m20) * p, z = (_mat.m10 - _mat.m01) * p;
	}
	else if ((_mat.m00 >= _mat.m11) && (_mat.m00 >= _mat.m22))
	{
		s = sqrtf(_mat.m00 - _mat.m11 - _mat.m22 + 1.0f), p = 0.5f / s, w = (_mat.m21 - _mat.m12) * p, x = s * 0.5f, y = (_mat.m10 + _mat.m01) * p, z = (_mat.m20 + _mat.m02) * p;
	}
	else if ((_mat.m11 >= _mat.m00) && (_mat.m11 >= _mat.m22))
	{
		s = sqrtf(_mat.m11 - _mat.m22 - _mat.m00 + 1.0f), p = 0.5f / s, w = (_mat.m02 - _mat.m20) * p, x = (_mat.m01 + _mat.m10) * p, y = s * 0.5f, z = (_mat.m21 + _mat.m12) * p;
	}
	else if ((_mat.m22 >= _mat.m00) && (_mat.m22 >= _mat.m11))
	{
		s = sqrtf(_mat.m22 - _mat.m00 - _mat.m11 + 1.0f), p = 0.5f / s, w = (_mat.m10 - _mat.m01) * p, x = (_mat.m02 + _mat.m20) * p, y = (_mat.m12 + _mat.m21) * p, z = s * 0.5f;
	}
}

void quaternion::rotation_matrix(const matrix44& _mat)
{
	float s, p, tr = _mat.m00 + _mat.m11 + _mat.m22;
	identify();
	if (tr > 0)
	{
		s = sqrtf(tr + 1.0f), p = 0.5f / s, w = s * 0.5f, x = (_mat.m12 - _mat.m21) * p, y = (_mat.m20 - _mat.m02) * p, z = (_mat.m01 - _mat.m10) * p;
	}
	else if ((_mat.m00 >= _mat.m11) && (_mat.m00 >= _mat.m22))
	{
		s = sqrtf(_mat.m00 - _mat.m11 - _mat.m22 + 1.0f), p = 0.5f / s, w = (_mat.m12 - _mat.m21) * p, x = s * 0.5f, y = (_mat.m01 + _mat.m10) * p, z = (_mat.m02 + _mat.m20) * p;
	}
	else if ((_mat.m11 >= _mat.m00) && (_mat.m11 >= _mat.m22))
	{
		s = sqrtf(_mat.m11 - _mat.m22 - _mat.m00 + 1.0f), p = 0.5f / s, w = (_mat.m20 - _mat.m02) * p, x = (_mat.m10 + _mat.m01) * p, y = s * 0.5f, z = (_mat.m12 + _mat.m21) * p;
	}
	else if ((_mat.m22 >= _mat.m00) && (_mat.m22 >= _mat.m11))
	{
		s = sqrtf(_mat.m22 - _mat.m00 - _mat.m11 + 1.0f), p = 0.5f / s, w = (_mat.m01 - _mat.m10) * p, x = (_mat.m20 + _mat.m02) * p, y = (_mat.m21 + _mat.m12) * p, z = s * 0.5f;
	}
}

matrix33::matrix33(const matrix34& _mat)
{
	m00 = _mat.m00;		m01 = _mat.m01;		m02 = _mat.m02;
	m10 = _mat.m10;		m11 = _mat.m11;		m12 = _mat.m12;
	m20 = _mat.m20;		m21 = _mat.m21;		m22 = _mat.m22;
}

matrix33::matrix33(const matrix44& _mat)
{
	m00 = _mat.m00;		m01 = _mat.m10;		m02 = _mat.m20;
	m10 = _mat.m01;		m11 = _mat.m11;		m12 = _mat.m21;
	m20 = _mat.m02;		m21 = _mat.m12;		m22 = _mat.m22;
}

matrix34::matrix34(const matrix44& _mat)
{
	m00 = _mat.m00;		m01 = _mat.m10;		m02 = _mat.m20;		m03 = _mat.m30;
	m10 = _mat.m01;		m11 = _mat.m11;		m12 = _mat.m21;		m13 = _mat.m31;
	m20 = _mat.m02;		m21 = _mat.m12;		m22 = _mat.m22;		m23 = _mat.m32;
}

vector3::vector3(const matrix34& _mat)
{
	vector3 _vec = _mat.translation();
	x = _vec.x; y = _vec.y; z = _vec.z;
}

vector3::vector3(const matrix44& _mat)
{
	vector3 _vec = _mat.translation();
	x = _vec.x; y = _vec.y; z = _vec.z;
}

vector3 vector3::transform_point(const matrix33& m, const vector3& p)
{
	vector3 tp;
	tp.x = float(m.m00 * p.x + m.m01 * p.y + m.m02 * p.z);
	tp.y = float(m.m10 * p.x + m.m11 * p.y + m.m12 * p.z);
	tp.z = float(m.m20 * p.x + m.m21 * p.y + m.m22 * p.z);
	return tp;
}

vector4::vector4(const color& _col) { x = _col.r; y = _col.g; z = _col.b; w = _col.a; }