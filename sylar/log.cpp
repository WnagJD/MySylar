#include <utility> // for std::pair
#include "log.h"
#include "config.h"



namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) case LogLevel::name: return #name;
    XX(FATAL);
    XX(ALERT);
    XX(CRIT);
    XX(ERROR);
    XX(WARN);
    XX(NOTICE);
    XX(INFO);
    XX(DEBUG);
#undef XX
    default:
        return "NOTSET";
    }
    return "NOTSET";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) if(str == #v) { return LogLevel::level; }
    XX(FATAL, fatal);
    XX(ALERT, alert);
    XX(CRIT, crit);
    XX(ERROR, error);
    XX(WARN, warn);
    XX(NOTICE, notice);
    XX(INFO, info);
    XX(DEBUG, debug);

    XX(FATAL, FATAL);
    XX(ALERT, ALERT);
    XX(CRIT, CRIT);
    XX(ERROR, ERROR);
    XX(WARN, WARN);
    XX(NOTICE, NOTICE);
    XX(INFO, INFO);
    XX(DEBUG, DEBUG);
#undef XX

    return LogLevel::NOTSET;
}

LogEvent::LogEvent(const std::string& loggerName, LogLevel::Level level, const char* file, int32_t line,
                int64_t elapse, uint32_t threadId, uint32_t fiberId, time_t time, const std::string& threadName)
    : m_level(level)
    , m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(threadId)
    , m_fiberId(fiberId)
    , m_time(time)
    , m_threadName(threadName)
    , m_loggerName(loggerName) {
}

void LogEvent::printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void LogEvent::vprintf(const char *fmt, va_list ap) {
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, ap);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem {
public:
    LoggerNameFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getLoggerName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << "\t";
    }
};

class PercentSignFormatItem : public LogFormatter::FormatItem {
public:
    PercentSignFormatItem(const std::string &str) {}
    void format(std::ostream &os, LogEvent::ptr event) override {
        os << "%";
    }
};

LogFormatter::LogFormatter(const std::string &pattern)
    : m_pattern(pattern) {
    init();
}

/**
 * 简单的状态机判断，提取pattern中的常规字符和模式字符
 * 
 * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
 * 
 * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
 * 
 * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
 * 
 * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
 * 
 * @see LogFormatter::LogFormatter
 */
void LogFormatter::init() {
    // 按顺序存储解析到的pattern项
    // 每个pattern包括一个整数类型和一个字符串，类型为0表示该pattern是常规字符串，为1表示该pattern需要转义
    // 日期格式单独用下面的dataformat存储
    std::vector<std::pair<int, std::string>> patterns;
    // 临时存储常规字符串
    std::string tmp;
    // 日期格式字符串，默认把位于%d后面的大括号对里的全部字符都当作格式字符，不校验格式是否合法
    std::string dateformat;
    // 是否解析出错
    bool error = false;
    
    // 是否正在解析常规字符，初始时为true (表示前一个阶段是在解析什么内容)
    bool parsing_string = true;
    // 是否正在解析模板字符，%后面的是模板字符
    // bool parsing_pattern = false;

    // "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    size_t i = 0;
    while(i < m_pattern.size()) {
        std::string c = std::string(1, m_pattern[i]);
        if(c == "%") {
            if(parsing_string) {
                if(!tmp.empty()) {
                    patterns.push_back(std::make_pair(0, tmp));
                }
                tmp.clear();
                parsing_string = false; // 在解析常规字符时遇到%，表示开始解析模板字符
                // parsing_pattern = true;
                i++;
                continue;
            } else {
                patterns.push_back(std::make_pair(1, c));
                parsing_string = true; // 在解析模板字符时遇到%，表示这里是一个%转义
                // parsing_pattern = false;
                i++;
                continue;
            }
        } else { // not %
            if(parsing_string) { // 持续解析常规字符直到遇到%，解析出的字符串作为一个常规字符串加入patterns
                tmp += c;
                i++;
                continue;
            } else { // 模板字符，直接添加到patterns中，添加完成后，状态变为解析常规字符，%d特殊处理
                patterns.push_back(std::make_pair(1, c));
                parsing_string = true; 
                // parsing_pattern = false;

                // 后面是对%d的特殊处理，如果%d后面直接跟了一对大括号，那么把大括号里面的内容提取出来作为dateformat
                if(c != "d") {
                    i++;
                    continue;
                }
                i++;
                if(i < m_pattern.size() && m_pattern[i] != '{') {
                    continue;
                }
                i++;
                while( i < m_pattern.size() && m_pattern[i] != '}') {
                    dateformat.push_back(m_pattern[i]);
                    i++;
                }
                if(m_pattern[i] != '}') {
                    // %d后面的大括号没有闭合，直接报错
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                    error = true;
                    break;
                }
                i++;
                continue;
            }
        }
    } // end while(i < m_pattern.size())

    if(error) {
        m_error = true;
        return;
    }

    // 模板解析结束之后剩余的常规字符也要算进去
    if(!tmp.empty()) {
        patterns.push_back(std::make_pair(0, tmp));
        tmp.clear();
    }

    // for debug 
    // std::cout << "patterns:" << std::endl;
    // for(auto &v : patterns) {
    //     std::cout << "type = " << v.first << ", value = " << v.second << std::endl;
    // }
    // std::cout << "dataformat = " << dateformat << std::endl;
    
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C)  {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));} }

        XX(m, MessageFormatItem),           // m:消息
        XX(p, LevelFormatItem),             // p:日志级别
        XX(c, LoggerNameFormatItem),        // c:日志器名称
//        XX(d, DateTimeFormatItem),          // d:日期时间
        XX(r, ElapseFormatItem),            // r:累计毫秒数
        XX(f, FileNameFormatItem),          // f:文件名
        XX(l, LineFormatItem),              // l:行号
        XX(t, ThreadIdFormatItem),          // t:编程号
        XX(F, FiberIdFormatItem),           // F:协程号
        XX(N, ThreadNameFormatItem),        // N:线程名称
        XX(%, PercentSignFormatItem),       // %:百分号
        XX(T, TabFormatItem),               // T:制表符
        XX(n, NewLineFormatItem),           // n:换行符
#undef XX
    };

    for(auto &v : patterns) {
        if(v.first == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
        } else if( v.second =="d") {
            m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
        } else {
            auto it = s_format_items.find(v.second);
            if(it == s_format_items.end()) {
                std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " << 
                "unknown format item: " << v.second << std::endl;
                error = true;
                break;
            } else {
                m_items.push_back(it->second(v.second));
            }
        }
    }

    if(error) {
        m_error = true;
        return;
    }
}

std::string LogFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for(auto &i:m_items) {
        i->format(ss, event);
    }
    return ss.str();
}

std::ostream &LogFormatter::format(std::ostream &os, LogEvent::ptr event) {
    for(auto &i:m_items) {
        i->format(os, event);
    }
    return os;
}

// LogAppender::LogAppender(LogFormatter::ptr default_formatter)
//     : m_defaultFormatter(default_formatter) {
// }

void LogAppender::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    if(m_formatter)
    {
        m_hasFormatter = true;
    }
    else{
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    // return m_formatter ? m_formatter : m_defaultFormatter;
    return m_formatter;
}

// StdoutLogAppender::StdoutLogAppender()
//     : LogAppender(LogFormatter::ptr(new LogFormatter)) {
// }

void StdoutLogAppender::log(LogEvent::ptr event) {
    if(event->getLevel() <= m_level)
    if(m_formatter) {
        m_formatter->format(std::cout, event);
    }
    // } else {
    //     m_defaultFormatter->format(std::cout, event);
    // }
}


std::string StdoutLogAppender::toYamlString()
{
    YAML::Node node;
    node["type"]="StdoutLogAppender";
    if(m_level!=LogLevel::NOTSET)
    {
        node["level"] = LogLevel::ToString(m_level);
    }

    if(m_hasFormatter && m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }

    std::stringstream ss;
    ss<<node;
    return ss.str();
}


FileLogAppender::FileLogAppender(const std::string &file)
    {
        m_filename = file;
        reopen();
        if(m_reopenError) {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
}

/**
 * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
 */
void FileLogAppender::log(LogEvent::ptr event) {
    if(event->getLevel()<=m_level)
        {
            uint64_t now = event->getTime();
            if(now >= (m_lastTime + 3)) {
                reopen();
                if(m_reopenError) {
                    std::cout << "reopen file " << m_filename << " error" << std::endl;
                }
                m_lastTime = now;
            }
            if(m_reopenError) {
                return;
            }
            if(m_formatter) {
                if(!m_formatter->format(m_filestream, event)) {
                    std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
                }
            } 
            // else {
            //     if(!m_defaultFormatter->format(m_filestream, event)) {
            //         std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            //     }
            // }
        }
    
}

std::string FileLogAppender::toYamlString()
{
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::NOTSET)
    {
        node["level"] = LogLevel::ToString(m_level);
    }

    if(m_hasFormatter && m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }

    std::stringstream ss;
    ss<<node;
    return ss.str();
}


bool FileLogAppender::reopen() {
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
    m_reopenError = !m_filestream;
    return !m_reopenError;
}

Logger::Logger(const std::string &name)
    : m_name(name)
    , m_level(LogLevel::INFO)
    , m_createTime(GetElapsedMS()) {
        m_formatter.reset(new LogFormatter());
    }

void Logger::addAppender(LogAppender::ptr appender) {
    if(!appender->getFormatter())
    {
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    m_appenders.clear();
}


//设置formatter
void Logger::setFormatter(const std::string& val)
{
    sylar::LogFormatter::ptr new_value(new LogFormatter(val));
    if(new_value->isError())
    {
        std::cout<<"Logger setFormatter name="<<m_name<<" value="<<val
                 <<" invalid formatter"<<std::endl;
        
        return;
    }

    setFormatter(new_value);
}

//设置formatter
void Logger::setFormatter(LogFormatter::ptr val)
{
    m_formatter = val;
    
    for(auto& i : m_appenders)
    {
        if(!i->m_hasFormatter)
        {
            i->m_formatter = m_formatter;
        }
    }
}


//将日志器的配置转换为YAML string
std::string Logger::toYamlString()
{
    YAML::Node node;
    node["name"] = m_name;
    if(m_level!=LogLevel::NOTSET)
    {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto& i : m_appenders)
    {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    
    std::stringstream ss;
    ss<<node;
    return ss.str();
}


/**
 * 调用Logger的所有appenders将日志写一遍，
 * Logger至少要有一个appender，否则没有输出
 */
void Logger::log(LogEvent::ptr event) {
    if(event->getLevel() <= m_level) {
        if(!m_appenders.empty())
        {
            for(auto &i : m_appenders) 
            {
                        i->log(event);
            }
        
        }else if(m_root)
        {
            m_root->log(event);
        }
    }
}

LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
    : m_logger(logger)
    , m_event(event) {
}

/**
 * @note LogEventWrap在析构时写日志
 */
LogEventWrap::~LogEventWrap() {
    m_logger->log(m_event);
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger("root"));
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->getName()] = m_root;
    init();
}

/**
 * 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
 * 需要手动添加Appender
 */
Logger::ptr LoggerManager::getLogger(const std::string &name) {
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;

    return logger;
}

std::string LoggerManager::toYamlString()
{
    YAML::Node node;
    for(auto& i: m_loggers)
    {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();

}



/**
 * @todo 实现从配置文件加载日志配置
 */
void LoggerManager::init() {
}


///////////////////////////////////////////////////////////////////////////////
// 从配置文件中加载日志配置
/**
 * @brief 日志输出器配置结构体定义
 */
struct LogAppenderDefine {
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::NOTSET;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::NOTSET;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;  //设定std::set的find函数通过name来比较(说明是否找到)
    }

    bool isValid() const {
        return !name.empty();
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string>{
    public:
        std::string operator()(const LogDefine& val)
        {
            YAML::Node node;
            node["name"]=val.name;
            if(val.level!=LogLevel::NOTSET)
            {
                node["level"]=LogLevel::ToString(val.level);
            }

            if(!val.formatter.empty())
            {
                node["formatter"]=val.formatter;
            }

            for(auto&i : val.appenders)
            {
                YAML::Node n;
                if(i.type == 1)
                {
                    n["type"] = "FileLogAppender";
                    n["file"] = i.file;
                }else if(i.type == 2)
                {
                    n["type"] = "StdLogAppender";
                }

                if(!i.formatter.empty())
                {
                    n["formatter"] = i.formatter;
                }
                node["appenders"].push_back(n);
            }
           
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
};


sylar::ConfigVar<std::set<LogDefine>>::ptr g_logs_defines =
        sylar::Config::Lookup("logs", std::set<LogDefine>(),"logs config");


struct LogIniter{
    LogIniter()
    {
        g_logs_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"on_logs_changed";
            for(auto& i : new_value)
            {
                auto it = old_value.find(i);
                Logger::ptr logger;
                if(it==old_value.end())
                {
                    //新增logger
                    
                    logger = SYLAR_LOG_NAME(i.name);
                }else{
                    if(!(i == *it))
                    {
                        //修改logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }

                logger->setLevel(i.level);
                if(!i.formatter.empty())
                {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a: i.appenders)
                {
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1) //FileLogAppender
                    {
                        ap.reset(new FileLogAppender(a.file));
                    }else if(a.type == 2)
                    {
                        ap.reset(new StdoutLogAppender());
                    }else{
                        continue;
                    }

                    ap->setLevel(a.level);
                    if(!a.formatter.empty())
                    {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError())
                        {
                            ap->setFormatter(fmt);
                        }else{
                            std::cout<<"log.nmae="<<i.name<<" appender type="<<a.type
                                     <<" formatter="<<a.formatter<<" is invalid"<<std::endl;
                        }
                    }
                    logger->addAppender(ap);

                }
            }

            for(auto&i : old_value)
            {
                auto it = new_value.find(i);
                if(it == new_value.end())
                {
                    //删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel(LogLevel::NOTSET);
                    logger->clearAppenders();
                }
            }
        }
        );
    }
};

static LogIniter __log_init__;

} // end namespace sylar