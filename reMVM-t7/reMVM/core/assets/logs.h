#pragma once
#include <Windows.h>

#include <string>
#include <format>
#include <fstream>
#include <unordered_map>

namespace mvm
{
	enum log_type { lg, ok, wrn, err };

	class logs
	{
	public:
		void initialize(const std::string&, const std::string&);
		void shutdown();

		bool initialized() { return m_initalized; }

		template <class... _types> void print(const std::format_string<_types...>, _types&& ...);
		template <class... _types> void print(log_type, const std::format_string<_types...>, _types&& ...);

		void print_log(const std::string&);

	private:
		bool m_initalized;

		std::string m_logFile;
#if _DEBUG
		HANDLE m_hstdin = nullptr, m_hstdout = nullptr;
		FILE* m_pfstdin = nullptr;
		FILE* m_pfstdout = nullptr;
		FILE* m_pfstlog = nullptr;
#endif
		std::unordered_map<mvm::log_type, const char*> m_logTypeMap = { { log_type::lg, "LOG" }, { log_type::ok, "OK" }, { log_type::wrn, "WARNING" }, { log_type::err, "ERROR" } };
		std::unordered_map<mvm::log_type, unsigned short> m_logTypeColorMap = { 
			{ log_type::lg, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY },
			{ log_type::ok, FOREGROUND_GREEN | FOREGROUND_INTENSITY},
			{ log_type::wrn, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY},
			{ log_type::err, FOREGROUND_RED | FOREGROUND_INTENSITY}
		};
	};
};

#include <iostream>
#include <assets/utils.h>

template<class ..._types> inline void mvm::logs::print(const std::format_string<_types...> _fmt, _types&& ..._args)
{
	if (!m_initalized)
		return;

	std::string _outFmt = std::format("[{}] [{}] ", m_logTypeMap[mvm::lg], utils::date()) + std::vformat(_fmt.get(), std::make_format_args(_args...)) + "\n";
	print_log(_outFmt);
#if _DEBUG
	SetConsoleTextAttribute(m_hstdout, m_logTypeColorMap[mvm::lg]);
	std::cout << _outFmt;
#endif
}

template<class ..._types> inline void mvm::logs::print(mvm::log_type _logType, const std::format_string<_types...> _fmt, _types && ..._args)
{
	if (!m_initalized)
		return;

	std::string _outFmt = std::format("[{}] [{}] ", m_logTypeMap[_logType], utils::date()) + std::vformat(_fmt.get(), std::make_format_args(_args...)) + "\n";
	print_log(_outFmt);
#if _DEBUG
	SetConsoleTextAttribute(m_hstdout, m_logTypeColorMap[_logType]);
	std::cout << _outFmt;
#endif
}