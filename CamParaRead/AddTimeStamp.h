#pragma once
#include <iostream>
#include <cassert>
#include <chrono>
#include <sstream>
#include <iomanip>

// From: https://stackoverflow.com/users/3099147/cpp-progger
class AddTimeStamp :
    public std::streambuf
{
public:
    AddTimeStamp(std::basic_ios< char >& out)
        : out_(out)
        , sink_()
        , newline_(true)
    {
        sink_ = out_.rdbuf(this);
        assert(sink_);
    }
    ~AddTimeStamp()
    {
        out_.rdbuf(sink_);
    }
protected:
    int_type overflow(int_type m = traits_type::eof())
    {
        if (traits_type::eq_int_type(m, traits_type::eof()))
            return sink_->pubsync() == -1 ? m : traits_type::not_eof(m);
        if (newline_)
        {   // --   add timestamp here
            std::ostream str(sink_);
            if (!(str << getTime())) // add perhaps a seperator " "
                return traits_type::eof(); // Error
        }
        newline_ = traits_type::to_char_type(m) == '\n';
        return sink_->sputc(m);
    }
private:
    std::string getTime()
    {
        const auto now = std::chrono::system_clock::now();
        const auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream output_stream;

        struct tm time_info;
        const auto errno_value = localtime_s(&time_info, &in_time_t);
        if (errno_value != 0)
        {
            throw std::runtime_error("localtime_s() failed: " + std::to_string(errno_value));
        }

        output_stream << std::put_time(&time_info, "%Y-%m-%d.%H:%M:%S");
        return output_stream.str() + std::string(" ");

    }
    AddTimeStamp(const AddTimeStamp&);
    AddTimeStamp& operator=(const AddTimeStamp&); // not copyable
    // --   Members
    std::basic_ios< char >& out_;
    std::streambuf* sink_;
    bool newline_;
};

