#pragma once
#include <assets/function.h>
#include <assets/math.h>

#include "../assets/addresses.h"
#include "../assets/structs.h"

#include <track_player.h>
#include <any>

namespace gc
{
	enum light_variable : __int32 
	{ 
		light_variable_type,
		light_variable_intensity,
		light_variable_color,
		light_variable_instansityScale,
		light_variable_position,
		light_variable_look_at,
		light_variable_u
	};

	struct light_node
	{
		vector3 position;
		vector3 lookAt;
		vector3 color;
		float intensity;
		float range;

		light_node(const light& _light)
		{
			position = _light.position;
			lookAt = _light.lookAt;
			color = _light.color;
			intensity = _light.intensity;
			range = _light.range;
		}
		light_node(const light_node& _marker) { *this = _marker; }
		light_node(const float* _data) { memcpy((void*)&position.x, _data, sizeof(light_node)); }

		void operator=(const light& _light)
		{
			position = _light.position;
			lookAt = _light.lookAt;
			color = _light.color;
			intensity = _light.intensity;
			range = _light.range;
		}
		void operator=(const float* _data) { memcpy((void*)&position.x, _data, sizeof(light_node)); }
		bool operator==(const light_node& _node)
		{
			for (int i = 0; i < sizeof(light_node) - 1; i++) {
				if (data()[i] != _node.c_data()[i])
					return false;
			}
			return true;
		}

		float& operator[](int _index) { return ((float*)this)[_index]; }
		const float operator[](int _index) const { return ((float*)this)[_index]; }

		float* data() { return &position.x; }
		const float* c_data() const { return &position.x; }

		static size_t size_of() { return sizeof(light_node) / sizeof(float); }
	};

	struct light_dolly
	{
		int instance;
		std::shared_ptr<mvm::track_player> dolly;
		std::vector<gc::light_node> buffer;

		void update_buffer();

		void insert_marker();
		void remove_marker(int);
		void replace_marker(float);

		int nearest_of(const vector3&);
	};

	class lightman
	{
	public:
		lightman();
		~lightman();

		void initialize();
		void update(float, float);

		int* curr_light();

		gc::light_dolly* light_dolly(int);
		const std::vector<gc::light_dolly>& lights_buffer();

		void push_dolly_mode(bool);
		bool dolly_mode();

		int* remove_method();

		void relocate(int);
		void redirect(int);

		static void push_variable(gc::light&, light_variable, const std::any&);

	private:
		void create();
		void remove(int);

		bool m_initialized;

		int m_lightCount, m_currLight;
		std::vector<gc::light_dolly> m_lights;
		bool m_dollyMode;
		int m_removeMethod;

		float m_frameTime;

		static inline lightman* m_lightmanInstance;
		static inline utils::function m_addLightmanMarker, m_removeLightmanMarker, m_editLightmanMarker;
		static __int64 __stdcall add_lightman_marker();
		static __int64 __stdcall remove_lightman_marker();
		static __int64 __stdcall edit_lightman_marker();

		friend struct light_dolly;
	};
}