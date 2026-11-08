#pragma once

#include <string>
#include <unordered_map>

#include <d3d11.h>
#include <wrl.h>

namespace mvm
{
	extern class render;
	class shaders_store;

	enum shader_type : __int32
	{
		unknown = 0,
		vertex = 1,
		geometry = 2,
		pixel = 4
	};

	class shader_instance
	{
	public:
		shader_instance() : m_types(unknown) {}
		shader_instance(const std::wstring& _path, int _types) : m_types(_types) {}
		~shader_instance() {}

		const std::wstring& path() const { return m_path; }
		int types() const { return m_types; }

	private:
		std::wstring m_path;

		int m_types;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertex;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometry;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixel;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

		friend class shaders_store;
	};

	class shaders_store
	{
	public:
		shaders_store(mvm::render*);
		~shaders_store();

		void create_shader(const std::string&, const std::wstring&, int);
		void create_shader(const std::string&, int, int, int, int);
		void use_shader(const std::string&);

		bool has_shaders() { return !m_shaders.empty(); }

		void recompile();
		void recompile(const std::string&);

	private:
		mvm::render* m_render;

		std::unordered_map<std::string, shader_instance> m_shaders;

	};
}