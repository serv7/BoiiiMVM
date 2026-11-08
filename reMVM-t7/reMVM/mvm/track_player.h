#ifndef _H__TRACK_PLAYER_H
#define _H__TRACK_PLAYER_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace mvm
{
	struct node
	{
		class value
		{
		public:
			value(size_t _size)						{ m_size = _size; m_data = new float[m_size]; }
			value(const float* _data, size_t _size)	{ m_size = _size; m_data = new float[m_size]; for (int i = 0; i < m_size; i++) m_data[i] = _data[i]; }

			const float* data()						const { return m_data; }
			size_t size()							const { return m_size; }

			void operator=(const value& _val)		{ for (int i = 0; i < m_size; i++) m_data[i] = _val.m_data[i]; }
			void operator=(const float& _val)		{ for (int i = 0; i < m_size; i++) m_data[i] = _val; }
			bool operator==(const value& _val)		{ for (int i = 0; i < m_size; i++) { if (m_data[i] != _val.data()[i]) return false; } return true; }

			float operator[](int _index)			const { return m_data[_index]; }

			value operator+(const value& _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] + _val[i]; return result; }
			value operator+(const float _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] + _val; return result; }
			value operator-(const value& _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] - _val[i]; return result; }
			value operator-(const float _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] - _val; return result; }
			value operator*(const value& _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] * _val[i]; return result; }
			value operator*(const float _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] * _val; return result; }
			value operator/(const value& _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] / _val[i]; return result; }
			value operator/(const float _val)		const { value result = value(m_size); for (int i = 0; i < m_size; i++) result.m_data[i] = m_data[i] / _val; return result; }

			void operator+=(const value& _val)		{ *this = *this + _val; }
			void operator+=(float _val)				{ *this = *this + _val; }
			void operator-=(const value& _val)		{ *this = *this - _val; }
			void operator-=(float _val)				{ *this = *this - _val; }
			void operator*=(const value& _val)		{ *this = *this * _val; }
			void operator*=(float _val)				{ *this = *this * _val; }
			void operator/=(const value& _val)		{ *this = *this / _val; }
			void operator/=(float _val)				{ *this = *this / _val; }

		private:
			float* m_data;
			size_t m_size;

		};

		float time;
		value val;
	};

	class track_player
	{
	public:
		track_player(size_t);
		~track_player();

		node::value value(float);

		void playback(bool);
		bool playing();

		void insert(float, const float*);
		void remove(float);
		void remove();
		void replace(float, const node::value&);
		void clear();

		mvm::node node() { return m_nodes.back(); }
		mvm::node node(int _index) { return m_nodes[_index]; }
		const std::vector<mvm::node>& buffer() { return m_nodes; }

		float nearest(float);
		float time_of(int _index) { return node(_index).time; }

		size_t size() { return m_nodes.size(); }

		void relocate(float, float);
		
	private:
		bool m_playing;

		size_t m_size;

		int m_node;
		std::vector<mvm::node> m_nodes;

		node::value interpolate(const node::value&, const node::value&, const node::value&, const node::value&, float);
	};
}
#endif