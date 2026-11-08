#pragma once
#include <assets/math.h>
#include <unordered_map>

namespace gc
{
	struct transform
	{
		vector3 pos;
		quaternion rot;

		transform(const vector3 _pos, const quaternion& _rot) { pos = _pos; rot = _rot; }
		transform(float* _data)
		{
			for (int i = 0; i < sizeof(transform) - 1; i++)
				*(float*)&this[i] = _data[i];
		}

		float* data() { return &pos.x; }
	};

	struct matrix
	{
		float m00, m01, m02, m03;
		float m10, m11, m12, m13;
		float m20, m21, m22, m23;
		float m30, m31, m32, m33;

		matrix() {}
		matrix(const float* _mat) { *this = _mat; }
		matrix(const matrix44& _mat) { *this = _mat; }
		~matrix() {}

		matrix& operator=(const float* _mat) { fill_matrix(&m00, _mat, 16); return *this; }
		matrix& operator=(const matrix44& _mat)
		{
			m00 = _mat.m00; m01 = _mat.m01; m02 = _mat.m03; m03 = _mat.m02;
			m10 = _mat.m10; m11 = _mat.m11; m12 = _mat.m13; m13 = _mat.m12;
			m20 = _mat.m20; m21 = _mat.m21; m22 = _mat.m23; m23 = _mat.m22;
			m30 = _mat.m30; m31 = _mat.m31; m32 = _mat.m33; m33 = _mat.m32;
			return *this;
		}

		operator matrix44() const
		{
			matrix44 _mat;
			_mat.m00 = m00; _mat.m01 = m01; _mat.m02 = m03; _mat.m03 = m02;
			_mat.m10 = m10; _mat.m11 = m11; _mat.m12 = m13; _mat.m13 = m12;
			_mat.m20 = m20; _mat.m21 = m21; _mat.m22 = m23; _mat.m23 = m22;
			_mat.m30 = m30; _mat.m31 = m31; _mat.m32 = m33; _mat.m33 = m32;
			return _mat;
		}

		vector3 translation() const { return vector3(m30, m31, m33); }

		vector3 forward_x() const { return vector3(m00, m01, m03); }
		vector3 forward_y() const { return vector3(m10, m11, m13); }
		vector3 forward_z() const { return vector3(m30, m31, m33); }
	};

	struct ref_def
	{
		char pad_0000[12]; //0x0000
		int width; //0x000C
		int height; //0x0010
		char pad_0014[100]; //0x0014
		float fovX; //0x0078
		float fovY; //0x007C
		char pad_0080[8]; //0x0080
		vector3 origin; //0x0088
		char pad_0094[16]; //0x0094
		vector3 axis[3]; //0x00A4

		static size_t size_of() { return 0x10A8; }
	};

	struct client_globals
	{
		int Clientnum;
		char pad0x0000[0x131CEC];
		ref_def* refdef;//131CF0
		char pad0x0001[0x1A6C98 + 0x200];
		vector3 view_angle;
	};

	struct user_cmd
	{
		char pad[0x10];
		int angle[3];
	};

	struct client_info
	{
		int32_t infovalid; //0x0000
		char pad_0004[4]; //0x0004
		int32_t number; //0x0008
		char name[32]; //0x000C
		int32_t team; //0x002C
		char pad_0030[196]; //0x0030
		int32_t health; //0x00F4
		char pad_00F8[4060]; //0x00F8

		static size_t size_of() { return 0x10D4; }
	};

	enum entity_type : __int32 {
		ET_GENERAL = 0,
		ET_PLAYER = 1,
		ET_PLAYER_CORPSE = 2,
		ET_ITEM = 3,
		ET_MISSILE = 4,
		ET_PLAYER_INVISIBLE = 5,
		ET_SCRIPTMOVER = 6,
		ET_SOUND_BLEND = 7,
		ET_FX = 8,
		ET_LOOP_FX = 9,
		ET_PRIMARY_LIGHT = 10,
		ET_LENSFLARE = 11,
		ET_REFLECTION_PROBE = 12,
		ET_HELICOPTER = 13,
		ET_PLANE = 14,
		ET_VEHICLE = 15,
		ET_VEHICLE_SPAWNER = 16,
		ET_VEHICLE_CORPSE = 17,
		ET_ACTOR = 18,
		ET_ACTOR_SPAWNER = 19,
		ET_ACTOR_CORPSE = 20,
		ET_STREAMER_HINT = 21,
		ET_ZBARRIER = 22,
		ET_TRIGGER = 23
	};

	struct entity_state
	{
		int number; //0x0000
		char pad_0004[12]; //0x0004
		int eType; //0x0010
		char pad_0014[396]; //0x0014
		int eType2; //0x01A0
		char pad_01A4[20]; //0x01A4
		int weapon; //0x01B8
		char pad_01BC[7876]; //0x01BC

		static size_t size_of() { return 0x1008; }
	};

	struct entity
	{
		char pad_0000[24];
		int dead;
		char pad_001C[36];
		vector3 origin;
		char pad_004C[940];
		entity_state next_state;
		char pad_1400[19612];

		static size_t size_of() { return 0x609C; }
	};

	class player_state
	{
	public:
		int clientnum; //0x0000
		char pad_0004[44]; //0x0004
		vector3 origin; //0x0030
		char pad_003C[148]; //0x003C
		angles delta_angle; //0x00D0
		char pad_00DC[572]; //0x00DC
		angles viewangle; //0x0318
		char pad_0324[5500]; //0x0324

		static size_t size_of() { return 0x18A0; }
	};

	enum dof_type { dof_off, dof_on, dof_far };

	struct camera
	{
		short pad_0x0000;
		short dof_type;
		float focal_length;
		float focus_distance;
		float fStop;
		char pad_0x0001[0x30];
		vector3 scale;
		float hud_pos_scalar;
		vector3 pos; // ...D78
		quaternion rot;
		matrix33 rotation_matrix;
		vector4 pad_0x0002;
		float temperature;
		float saturation;
		vector3 color_scale;
		float screen_brightness;

		matrix44 transform()
		{
			matrix44 _transform;
			_transform.translate(pos);
			_transform.rotation_quaternion(rot);
			return _transform;
		}
	};

	struct light
	{
		int type;
		float intensity;
		vector3 color;
		float range;
		float instansityScale;
		vector3 position;
		vector3 lookAt;
		vector3 u;
	};

	struct fog
	{
		float bDist;
		float hDist;
		float bHeight;
		float hHeight;
		float skyHalfHeightOffset;
		char pad_0x0000[0x40];
		vector3 fCol;
		float fDensity;
		vector3 shCol;
		float shbDist;
		float shfDist;
		float shStrength;
		float shSpread;
		float Brightness;
		float Opacity;
		float pbrAmount;
		float LitFogDensity;
		float WorldFogSkySize;
	};

	enum theater_mode
	{
		basic = 0,
		director = 1,
		dolly = 2,
		object_link = 3,
		lighter = 4
	};

	struct theater_camera
	{
		vector3 pos;
		angles rot;
	};

	struct dolly_marker
	{
		int tick;
		char pad_0x0000[0x4];
		vector3 pos;
		vector3 dir;
		char pad_0x0001[0x66];
	};
}