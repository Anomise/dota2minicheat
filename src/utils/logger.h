#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdio>
#include <cstdarg>
#include <chrono>

struct LogEntry {
    std::string message;
    float       timestamp;
    int         level; // 0=info, 1=warning, 2=error, 3=success
};

class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void Log(int level, const char* fmt, ...) {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        // Console output
        const char* prefix = "";
        switch (level) {
            case 0: prefix = "[INFO]"; break;
            case 1: prefix = "[WARN]"; break;
            case 2: prefix = "[ERROR]"; break;
            case 3: prefix = "[OK]"; break;
        }
        printf("%s %s\n", prefix, buf);

        // Store for menu
        std::lock_guard<std::mutex> lock(mtx);

        float time = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - startTime).count();

        entries.push_back({std::string(buf), time, level});

        // Keep max 500 entries
        if (entries.size() > 500)
            entries.erase(entries.begin());
    }

    void Info(const char* fmt, ...)    { char buf[1024]; va_list a; va_start(a,fmt); vsnprintf(buf,sizeof(buf),fmt,a); va_end(a); Log(0, "%s", buf); }
    void Warn(const char* fmt, ...)    { char buf[1024]; va_list a; va_start(a,fmt); vsnprintf(buf,sizeof(buf),fmt,a); va_end(a); Log(1, "%s", buf); }
    void Error(const char* fmt, ...)   { char buf[1024]; va_list a; va_start(a,fmt); vsnprintf(buf,sizeof(buf),fmt,a); va_end(a); Log(2, "%s", buf); }
    void Success(const char* fmt, ...) { char buf[1024]; va_list a; va_start(a,fmt); vsnprintf(buf,sizeof(buf),fmt,a); va_end(a); Log(3, "%s", buf); }

    std::vector<LogEntry> GetEntries() {
        std::lock_guard<std::mutex> lock(mtx);
        return entries;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mtx);
        entries.clear();
    }

    int GetCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return (int)entries.size();
    }

private:
    Logger() : startTime(std::chrono::steady_clock::now()) {}
    std::vector<LogEntry> entries;
    std::mutex mtx;
    std::chrono::steady_clock::time_point startTime;
};

// Shorthand macros
#define LOG_INFO(fmt, ...)    Logger::Get().Info(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    Logger::Get().Warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   Logger::Get().Error(fmt, ##__VA_ARGS__)
#define LOG_SUCCESS(fmt, ...) Logger::Get().Success(fmt, ##__VA_ARGS__)
