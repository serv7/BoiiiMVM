#pragma once
#include <chrono>
#include <ctime>

template <typename T>
class timer_tpl
{
public:
    void start() { m_start = std::chrono::system_clock::now(); m_running = true; }
    void stop() { m_end = std::chrono::system_clock::now(); m_running = false; }

    bool is_running() { return m_running; }
    double elapsed() { return std::chrono::duration_cast<T>((m_running ? std::chrono::system_clock::now() : m_end) - m_start).count(); }

    void reset() { m_running = false; m_start = std::chrono::system_clock::now(); m_end = std::chrono::system_clock::now(); }

private:
    std::chrono::time_point<std::chrono::system_clock> m_start;
    std::chrono::time_point<std::chrono::system_clock> m_end;
    bool m_running = false;
};

typedef timer_tpl<std::chrono::nanoseconds> timer_ns;
typedef timer_tpl<std::chrono::milliseconds> timer_ms;
typedef timer_tpl<std::chrono::seconds> timer_s;