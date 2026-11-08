#include "track_player.h"

mvm::track_player::track_player(size_t _size) : m_playing(false), m_size(_size), m_node(0)
{

}

mvm::track_player::~track_player()
{

}

mvm::node::value mvm::track_player::value(float _time)
{
	mvm::node::value _val = mvm::node::value(m_size);
	if (m_nodes.size() < 2)
		return _val;

	bool _forward, _backward;
	while ((_forward = _time >= m_nodes[m_node + 1].time) || (_backward = _time < m_nodes[m_node].time)) {
		if (_forward) {
			if (m_node + 1 < m_nodes.size() - 1)
				m_node++;
			else return m_nodes[m_nodes.size() - 1].val;
		}
		else if (_backward) {
			if (m_node - 1 >= NULL)
				m_node--;
			else return m_nodes[NULL].val;
		}
	}

	std::vector<mvm::node>::iterator n0, n1, n2, n3;
	n1 = m_nodes.begin() + m_node;
	n2 = n1 + 1;
	n0 = m_node > NULL ? n1 - 1 : n1;
	n3 = m_node + 1 < m_nodes.size() - 1 ? n2 + 1 : n2;

	float _alpha = (_time - n1->time) / (n2->time - n1->time);
	_val = interpolate(n0->val, n1->val, n2->val, n3->val, _alpha);

	return _val;
}

void mvm::track_player::playback(bool _state)
{
	if (m_nodes.size() < 2)
		return;
	m_playing = _state;
}

bool mvm::track_player::playing()
{
	return m_playing;
}

void mvm::track_player::insert(float _time, const float* _data)
{
	if (m_playing)
		return;

	mvm::node _node = { _time, node::value(_data, m_size) };
	if (!m_nodes.empty()) {
		const auto& _it = std::lower_bound(m_nodes.begin(), m_nodes.end(), _node, [](const mvm::node& _rhs, const mvm::node& _lhs) { return _rhs.time < _lhs.time; });
		if (_it != m_nodes.end() && _it->time == _time)
			return;
		m_nodes.insert(_it, _node);
	}
	else m_nodes.push_back(_node);
}

void mvm::track_player::remove(float _time)
{
	if (m_nodes.empty() || m_playing)
		return;

	const auto& _it = std::find_if(m_nodes.begin(), m_nodes.end(), [&](const mvm::node& _node) { return _node.time == _time; });
	if (_it != m_nodes.end())
		m_nodes.erase(_it);

	if (m_nodes.size() < 2 && m_playing)
		m_playing = false;
}

void mvm::track_player::remove()
{
	if (m_nodes.empty() || m_playing)
		return;

	m_nodes.pop_back();
	if (m_nodes.size() < 2 && m_playing)
		m_playing = false;
}

void mvm::track_player::replace(float _time, const node::value& _val)
{
	const auto& _it = std::find_if(m_nodes.begin(), m_nodes.end(), [&](const mvm::node& _node) { return _node.time == _time; });
	if (_it != m_nodes.end())
		_it._Ptr->val = _val;
}

void mvm::track_player::clear()
{
	if (m_playing)
		return;

	m_nodes.clear(); 
	m_node = 0;
}

float mvm::track_player::nearest(float _time)
{
	if (m_nodes.size() > 1) {
		if (_time < m_nodes[NULL].time)
			return m_nodes[NULL].time;

		if (_time > m_nodes[m_nodes.size() - 1].time)
			return m_nodes[m_nodes.size() - 1].time;

		for (int i = 0; i < m_nodes.size() - 2; i++) {
			const auto& _n0 = m_nodes.begin() + i;
			const auto& _n1 = _n0 + 1;

			if (_time > _n1->time)
				continue;

			const float _delta = _n1->time - _n0->time;
			return (_time > (_n0->time + _delta / 2.0f)) ? _n1->time : _n0->time;
		}
	}

	return m_nodes.size() > NULL ? m_nodes[m_nodes.size() - 1].time : -1.0f;
}

void mvm::track_player::relocate(float _currTime, float _newTime)
{
	if (m_nodes.empty())
		return;

	const auto& _it = std::find_if(m_nodes.begin(), m_nodes.end(), [&](const mvm::node& _node) { return _node.time == _currTime; });
	const auto& _insIt = std::lower_bound(m_nodes.begin(), m_nodes.end(), *_it._Ptr, [](const mvm::node& _rhs, const mvm::node& _lhs) { return _rhs.time < _lhs.time; });
	if (_it != m_nodes.end() && _insIt != m_nodes.end() && _insIt != _it) {
		mvm::node::value _val = _it->val;
		m_nodes.erase(_it);
		m_nodes.insert(_insIt, { _newTime, _val });
	}
}

mvm::node::value mvm::track_player::interpolate(const node::value& _y0, const node::value& _y1, const node::value& _y2, const node::value& _y3, float _alpha)
{
	node::value _v0 = _y0 * (-0.5) + _y1 * 1.5 - _y2 * 1.5 + _y3 * 0.5;
	node::value _v1 = _y0 - _y1 * (2.5) + _y2 * 2.0 - _y3 * 0.5;
	node::value _v2 = _y0 * (-0.5) + _y2 * 0.5;
	node::value _v3 = _y1;
	return _v0 * _alpha * _alpha * _alpha + _v1 * _alpha * _alpha + _v2 * _alpha + _v3;
}