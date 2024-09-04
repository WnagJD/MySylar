#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <memory>
#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <list>
#include <map>
#include <cstdarg>
#include <iostream>
#include "util.h"
#include "singleton.h"

//获取root日志器
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

//获取指定名称的日志器
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)


#define SYLAR_LOG_LEVEL(logger , level) \
    if(level <= logger->getLevel()) \
        sylar::LogEventWrap(logger, sylar::LogEvent::ptr(new sylar::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, sylar::GetElapsedMS() - logger->getCreateTime(), \
            sylar::GetThreadId(), sylar::GetFiberId(), time(0), sylar::GetThreadName()))).getLogEvent()->getSS()


#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_ALERT(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ALERT)

#define SYLAR_LOG_CRIT(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::CRIT)

#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

#define SYLAR_LOG_NOTICE(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::NoTICE)

#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)


#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt,...)\
        if(level <=logger->getLevel()) \
            sylar::LogEventWrap(logger, sylar::LogEvent::ptr(new LogEvent(logger->getName(),\
            level, __FILE__, __LINE__, sylar::GetElapsedMS() - logger->getCreateTime(),\
            sylar::GetThreadId(), sylar::GetFiberId(), time(0), sylar::GetThreadName()))).getLogEvent()->printf(fmt, __VA_ARGS__)


#define SYLAR_LOG_FMT_FATAL(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_ALERT(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::ALERT, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_CRIT(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::CRIT, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_ERROR(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_WARN(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_INFO(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger,fmt,...) SYLAR_LOG_FMAT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)





namespace sylar{


class Logger;
class LoggerManager;

//日志级别
class LogLevel
{
    public:
        enum Level
        {
            FATAL = 0,
            ALERT = 100,
            CRIT =200,
            ERROR =300,
            WARN = 400,
            NOTICE = 500,
            INFO = 600,
            DEBUG = 700,
            NOTSET = 800,
        };

    static const char* ToString(LogLevel::Level Level); 
    static LogLevel::Level FromString(const std::string& str);       
};


//日志事件
class LogEvent
{
    public:
        typedef std::shared_ptr<LogEvent> ptr;
    
        LogEvent(const std::string& loggerName, LogLevel::Level level, const char* file, int32_t line,
                    int64_t elapse, uint32_t threadId, uint32_t fiberId, time_t time, const std::string& threadName);

        //获取对应成员变量

        LogLevel::Level getLevel() const{ return m_level;}

        //获取日志内容
        std::string getContent() const{ return m_ss.str();}

        uint32_t getLine() const{ return m_line;}

        std::string getFile() const{ return m_file;}

        uint64_t getElapse() const { return m_elapse;}

        uint32_t getThreadId() const{ return m_threadId;}

        uint32_t getFiberId() const { return m_fiberId;}

        time_t getTime() const{ return m_time;}

        const std::string& getThreadName() const{ return m_threadName;}

        const std::string& getLoggerName() const{ return m_loggerName;}
        
        //获取内容字节流,用于流式写日志
        std::stringstream& getSS() { return m_ss;}

        /**
         * C printf 风格写日志
         * C vprintf 风格写日志
         */

        void printf(const char* fmt,...);

        void vprintf(const char* fmt, va_list ap);


    private:
        //日志级别
        LogLevel::Level m_level;
        //日志内容(给用户使用的)
        std::stringstream m_ss;
        //文件名
        const char* m_file =nullptr;
        //行号
        int32_t m_line =0;
        //从日志器创建开始到现在的耗时
        int64_t m_elapse=0;
        //线程ID
        uint32_t m_threadId=0;
        //协程ID
        uint32_t m_fiberId=0;
        //时间戳 UTC
        time_t m_time;
        //线程名称
        std::string m_threadName;
        //日志器名称
        std::string m_loggerName;

};


//日志格式化
class LogFormatter{
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        /**
        * - %%m 消息
        * - %%p 日志级别
        * - %%c 日志器名称
        * - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
        * - %%r 该日志器创建后的累计运行毫秒数
        * - %%f 文件名
        * - %%l 行号
        * - %%t 线程id
        * - %%F 协程id
        * - %%N 线程名称
        * - %%% 百分号
        * - %%T 制表符
        * - %%n 换行
        * 
        * 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
        * 
        * 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符 
         */
                                                  
        LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

        void init();

        bool isError()const { return m_error;}


        //返回格式化日志字符串
        std::string format(LogEvent::ptr Event);

        //返回格式化日志流
        std::ostream& format(std::ostream& ss, LogEvent::ptr event);


        std::string getPattern() const { return m_pattern;}
    

    public:
        class FormatItem{
            public:
                typedef std::shared_ptr<FormatItem> ptr;
                virtual ~FormatItem(){};

                virtual void format(std::ostream & os, LogEvent::ptr event) =0;
        };

    private:
        //日志格式模版
        std::string m_pattern;
        //解析后的格式项数组
        std::vector<FormatItem::ptr> m_items;
        //是否解析出错
        bool m_error = false;
};

//日志输出器
class LogAppender{
    friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr;


        //默认日志格式
        // LogAppender(LogFormatter::ptr default_formatter);
    

        virtual ~LogAppender(){}

        //设置日志格式
        void setFormatter(LogFormatter::ptr val);

        //获取日志格式
        LogFormatter::ptr getFormatter();

        //写日志
        virtual void log(LogEvent::ptr event) =0;

        //设置日志级别
        void setLevel(LogLevel::Level val){ m_level = val;}

        //将日志输出器配置转换为YAML string
        virtual std::string toYamlString() = 0;



    protected:
        //日志级别
        LogLevel::Level m_level = LogLevel::DEBUG;
        //日志格式器
        LogFormatter::ptr m_formatter;
        //默认的日志格式器
        // LogFormatter::ptr m_defaultFormatter;
        //是否有自己的formatter
        bool m_hasFormatter = false;

};


//输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        // StdoutLogAppender();

        void log(LogEvent::ptr event);

        //将日志输出器配置转换为YAML string
        std::string toYamlString() override;


};


//输出到文件的Appender
class FileLogAppender : public LogAppender{
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string& filename);

        void log(LogEvent::ptr event);

        //重新打开日志文件 返回true
        bool reopen();

        //将日志输出器配置转换为YAML string
        std::string toYamlString() override;

    
    private:
        //文件路径
        std::string m_filename;
        //文件流
        std::ofstream m_filestream;
        //上次打开的时间
        uint64_t m_lastTime =0;
        //文件打开错误标志
        bool m_reopenError = false;
};


//日志器
class Logger : public std::enable_shared_from_this<Logger>
{
    friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;
        
        Logger(const std::string& name ="root");

        const std::string& getName() const { return m_name;}

        const uint64_t& getCreateTime() const { return m_createTime;}

        //设置日志级别
        void setLevel(LogLevel::Level level){ m_level = level;}

        LogLevel::Level getLevel() const { return m_level;}

        //添加appender
        void addAppender(LogAppender::ptr appender);

        //删除appender
        void delAppender(LogAppender::ptr appender);

        //设置formatter
        void setFormatter(const std::string& val);

        //设置formatter
        void setFormatter(LogFormatter::ptr val);


        //清空LogAppender
        void clearAppenders();

        //写日志
        void log(LogEvent::ptr event);

        //将日志器的配置转换为YAML string
        std::string toYamlString();
    

    private:
        //日志器名称
        std::string m_name;
        //日志器级别
        LogLevel::Level m_level;
        //日志输出器集合
        std::list<LogAppender::ptr> m_appenders;
        //日志器创建时间
        uint64_t m_createTime;
        //日志格式
        LogFormatter::ptr m_formatter; 
        //主日志器
        Logger::ptr m_root;
};

//日志事件包装器,方便宏定义,内部包含日志事件和日志器
class LogEventWrap
{
    public:
        LogEventWrap(Logger::ptr logger, LogEvent::ptr event);
        ~LogEventWrap();


        //获取日志事件
        LogEvent::ptr getLogEvent()const{return m_event;}

    private:
        //日志器
        Logger::ptr m_logger;
        //日志事件
        LogEvent::ptr m_event;
 

};

//日志器管理类
class LoggerManager
{
    public:
        LoggerManager();

        //初始化,主要是结合配置模块实现日志模块的初始化
        void init();
        
        //获取指定名称的日志器
        Logger::ptr getLogger(const std::string& name);

        //获取"root"名称的日志器
        Logger::ptr getRoot(){return m_root;}

        //将所有的日志器配置转成YAML String
        std::string toYamlString();

    
    private:
        //root日志器
        Logger::ptr m_root;
        //日志器集合
        std::map<std::string, Logger::ptr> m_loggers;

};


typedef sylar::Singleton<LoggerManager> LoggerMgr;


}

#endif
