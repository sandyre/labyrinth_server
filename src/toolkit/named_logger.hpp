//
//  named_logger.hpp
//  labyrinth_serv_xcode
//
//  Created by Aleksandr Borzikh on 06.07.17.
//  Copyright © 2017 hate-red. All rights reserved.
//

#ifndef named_logger_hpp
#define named_logger_hpp

#include <fstream>
#include <sstream>

#include <string>


namespace Color
{
    static const std::string RED        = "\033[1;31m";
    static const std::string GREEN      = "\033[1;32m";
    static const std::string YELLOW     = "\033[1;33m";
    static const std::string BLUE       = "\033[1;34m";
    static const std::string CYAN       = "\033[0;36m";
    static const std::string MAGENTA    = "\033[1;35m";
    static const std::string WHITE      = "\033[1;37m";
    static const std::string RESET      = "\033[0m";
}

class NamedLogger
{
private:
    class LoggerStream
    {
        friend NamedLogger;

    public:
        enum class Level
        {
            DBG,
            INFO,
            WARNING,
            ERROR
        };

    private:
        LoggerStream(const NamedLogger& parent,
                     Level level);
        LoggerStream(const LoggerStream& other)
        : _logger(other._logger),
          _prefix(other._prefix)
        {

        }

    public:
        template<typename T>
        LoggerStream& operator<<(T value)
        {
            _stream << value;

            return *this;
        }

        ~LoggerStream()
        {
            _stream << Color::RESET;
            _logger.Write(_stream.str());
        }

    private:
        const NamedLogger&  _logger;
        std::string         _prefix;
        std::ostringstream  _stream;
    };
    friend LoggerStream;

public:
    enum Mode
    {
        FILE  = 0x01,
        STDIO = 0x02,
        MIXED = 0x03
    };

public:
    NamedLogger(const std::string& name, Mode mode = STDIO);

    LoggerStream Debug()
    { return LoggerStream(*this, LoggerStream::Level::DBG); }

    LoggerStream Info()
    { return LoggerStream(*this, LoggerStream::Level::INFO); }

    LoggerStream Warning()
    { return LoggerStream(*this, LoggerStream::Level::WARNING); }

    LoggerStream Error()
    { return LoggerStream(*this, LoggerStream::Level::ERROR); }

private:
    void Write(const std::string& str) const;

private:
    const Mode              _mode;
    const std::string       _name;

    mutable std::mutex      _fileMutex;
    mutable std::ofstream   _fileStream;
};

#endif /* named_logger_hpp */
