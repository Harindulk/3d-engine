#pragma once
#include <string>
#include <iostream>
#include <mutex>

namespace aurora {

enum class LogLevel { Trace=0, Debug, Info, Warn, Error, Critical, Off };

class Logger {
public:
    static void setLevel(LogLevel lvl) { instance().level_ = lvl; }
    static LogLevel level() { return instance().level_; }

    template<typename... Args>
    static void log(LogLevel lvl, const char* tag, Args&&... args) {
        if (lvl < instance().level_) return;
        std::lock_guard<std::mutex> lock(instance().mtx_);
        std::ostream& os = (lvl >= LogLevel::Error ? std::cerr : std::cout);
        os << prefix(lvl) << "[" << tag << "] ";
        (os << ... << args) << '\n';
    }

    template<typename... Args> static void trace(const char* tag, Args&&... a){ log(LogLevel::Trace, tag, std::forward<Args>(a)...);}    
    template<typename... Args> static void debug(const char* tag, Args&&... a){ log(LogLevel::Debug, tag, std::forward<Args>(a)...);}    
    template<typename... Args> static void info(const char* tag, Args&&... a){ log(LogLevel::Info, tag, std::forward<Args>(a)...);}      
    template<typename... Args> static void warn(const char* tag, Args&&... a){ log(LogLevel::Warn, tag, std::forward<Args>(a)...);}      
    template<typename... Args> static void error(const char* tag, Args&&... a){ log(LogLevel::Error, tag, std::forward<Args>(a)...);}    
    template<typename... Args> static void critical(const char* tag, Args&&... a){ log(LogLevel::Critical, tag, std::forward<Args>(a)...);} 

private:
    static Logger& instance(){ static Logger inst; return inst; }
    static const char* prefix(LogLevel lvl) {
        switch(lvl){
            case LogLevel::Trace: return "[TRACE]"; case LogLevel::Debug: return "[DEBUG]"; case LogLevel::Info: return "[INFO ]"; case LogLevel::Warn: return "[WARN ]"; case LogLevel::Error: return "[ERROR]"; case LogLevel::Critical: return "[FATAL]"; default: return ""; }
    }
    LogLevel level_ = LogLevel::Info;
    std::mutex mtx_;
};

} // namespace aurora
