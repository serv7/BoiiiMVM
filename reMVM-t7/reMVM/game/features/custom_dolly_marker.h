#pragma once

namespace gc
{
	struct custom_dolly_marker
	{
		custom_dolly_marker() {}
		custom_dolly_marker(const custom_dolly_marker& _marker) { *this = _marker; }
		custom_dolly_marker(const float* _data) { memcpy((void*)&_fov, _data, sizeof(custom_dolly_marker)); }

		float _fov, _focalLength, _focusDistance, _fStop;

		void operator=(const float* _data) { memcpy((void*)&_fov, _data, sizeof(custom_dolly_marker)); }
		bool operator==(const custom_dolly_marker& _marker)
		{
			for (int i = 0; i < sizeof(custom_dolly_marker) - 1; i++) {
				if (data()[i] != _marker.c_data()[i])
					return false;
			}
			return true;
		}

		float& operator[](int _index) { return ((float*)this)[_index]; }
		const float operator[](int _index) const { return ((float*)this)[_index]; }

		float* data() { return &_fov; }
		const float* c_data() const { return &_fov; }

		static size_t size_of() { return sizeof(custom_dolly_marker) / sizeof(float); }
	};
}