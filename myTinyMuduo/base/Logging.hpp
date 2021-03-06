//
// Created by john on 4/16/19.
//

#ifndef MY_TINYMUDUO_BASE_LOGGING_H
#define MY_TINYMUDUO_BASE_LOGGING_H

#include "base/LogStream.hpp"
#include "base/Timestamp.hpp"
namespace muduo{
    class Logger{
    public:
        enum LogLevel{
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };

        class SourceFile{
        public:
            template <int N>
            SourceFile(const char (&arr)[N])
                : data_(arr),
                  size_(N-1)
            {
                const char* slash = strrchr(data_,'/');
                if(slash){
                    data_ = slash+1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }
            //提取文件名
            explicit SourceFile(const char* filename)
                : data_(filename)
            {
                const char* slash = strrchr(filename,'/');
                if(slash){
                    data_ = slash+1;
                }
                size_ = static_cast<int>(strlen(data_));
            }

            const char* data_;
            int size_;
        };
        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char* func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream& stream(){return impl_.stream_;}

        static LogLevel logLevel(){return g_logLevel;}
        static void setLogLevel(LogLevel level){g_logLevel = level;}
        static std::string getFLogname(){return fbasename_;}
        static void setFLogname(std::string s){fbasename_ = s;} 

    private:
        class Impl{
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level,int old_errno,const SourceFile& file,int line);
            void formatTime();
            void finish();

            Timestamp   time_;
            LogStream   stream_;
            LogLevel    level_;
            int         line_;
            SourceFile  basename_;
        };
        Impl impl_;
    public:
        static std::string fbasename_;
        static LogLevel g_logLevel;
    };
//__FILE__当前文件名（包含目录）， __LINE__当前行号
//创建一个匿名Logger对象，Logger对象内部的impl对象含有LogStream，将日志内容写到
//LogStream的buffer_中，在Logger的析构函数中将LogStream中的buffer_内容输出
//匿名对象Logger，在1行log命令后会立即销毁
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) muduo::Logger(__FILE__, __LINE__,muduo::Logger::INFO).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

}
#endif //MY_TINYMUDUO_BASE_LOGGING_H
