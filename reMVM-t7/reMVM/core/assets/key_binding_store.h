#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace mvm
{
	struct bind_instance
	{
		bind_instance() : name(""), key(-1) { }
		bind_instance(const std::string& _name, int _key) { name = _name; key = _key; }

		bool valid() { return (!name.empty() && key >= NULL); }

		std::string name;
		int key;
	};

	class key_binding_store
	{
	public:
		key_binding_store(const std::string&);
		~key_binding_store();

		std::vector<bind_instance>& instances() { return m_instances; }
		bool empty() { return m_instances.empty(); }
		size_t size() { return m_instances.size(); }

		void bind(const std::string&, int, bool _override = false);
		bind_instance* find(const int);
		bind_instance* find(const std::string&);
		void override_key(const std::string&, int);

		void save();
		void load();

		void begin_capturing(const std::string&);
		void complete_capturing();
		void capture_key(int);

		bool capturing() { return m_capturing; }
		const std::string& capturing_target() { return m_capturingTarget; }
		int captured_key() { return m_capturedKey; }

	private:
		std::filesystem::path m_file;
		std::vector<bind_instance> m_instances;

		bool m_capturing;
		std::string m_capturingTarget;
		int m_capturedKey;

	};
}