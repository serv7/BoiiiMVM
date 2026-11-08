#ifndef _H__ANIMATED_VALUE_H
#define _H__ANIMATED_VALUE_H

template <typename _ty>
class animated_value
{
public:
	animated_value() : m_start(), m_end(), m_value() {}
	animated_value(_ty _val)
	{
		m_start = _val;
		m_end = _val;
		m_value = _val;
	}

	animated_value(const animated_value<_ty>& _animVal)
	{
		m_start = _animVal.m_start;
		m_end = _animVal.m_end;
		m_value = _animVal.m_value;
	}

	animated_value(const _ty& _start, const _ty& _end, const _ty& _val = _ty())
	{
		m_start = _start;
		m_end = _end;
		m_value = _val;
	}

	~animated_value() {}

private:
	_ty m_start, m_end, m_value;

public:
	void process(float _alpha, bool _forward) { m_value = linear(m_value, _forward ? m_end : m_start, _alpha); }

	_ty start() const { return m_start; }
	_ty end() const { return m_end; }

	_ty value() const { return m_value; }

private:
	_ty linear(const _ty& _y0, const _ty& _y1, float _alpha) { return _y0 + (_y1 - _y0) * _alpha; }
};
#endif