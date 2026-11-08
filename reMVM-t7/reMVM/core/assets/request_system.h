#pragma once
#include <any>
#include <vector>

namespace utils
{
	struct base_request
	{
		int _type;
		std::any _data;
	};

	class request_store
	{
	public:
		request_store() { }
		~request_store() { }

		virtual void create_request(int _type, const std::any& _data = std::any()) { m_requests.push_back({ _type, _data }); }
		virtual std::vector<base_request>& requests() { return m_requests; }

		virtual void handle_requests() = 0;

	private:
		std::vector<base_request> m_requests;

	};
}