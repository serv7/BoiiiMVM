#include "logs.h"
#include <filesystem>

void mvm::logs::initialize(const std::string& _title, const std::string& _file)
{
#if _DEBUG
    AllocConsole();
    SetConsoleTitleA(_title.c_str());
    freopen_s(&m_pfstdout, "CONOUT$", "w", stdout);
    freopen_s(&m_pfstdin, "CONIN$", "r", stdin);
    m_hstdin = GetStdHandle(STD_INPUT_HANDLE);
    m_hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    (system)("chcp 65001 > nul");
#endif
    
    if (std::filesystem::exists(_file))
        std::filesystem::remove(_file);
    m_logFile = _file;

    m_initalized = true;
}

void mvm::logs::shutdown()
{
#if _DEBUG
    FreeConsole();
#endif
    m_initalized = false;
    m_logFile.clear();
}

#include "../../mvm.h"
void mvm::logs::print_log(const std::string& _fmt)
{
    std::ofstream _fs(m_logFile, std::ios::binary | std::ios::out | (std::filesystem::exists(m_logFile) ? std::ios::app : NULL));
    if (_fs.is_open()) {
        _fs.write(_fmt.data(), static_cast<std::streamsize>(_fmt.size()));
        _fs.close();
    }

#if not MVM_RELEASE
    if (auto _uiWrapper = mvm::_core->ui_wrapper(); _uiWrapper)
        _uiWrapper->push_console_line(_fmt);
#endif
}