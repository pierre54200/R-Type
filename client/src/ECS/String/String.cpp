#include "String.hpp"

String::String(std::string string)
    : std::string(string.c_str())
{}

std::vector<String> String::split(char sep)
{
    std::vector<String> ret;
    size_t              start = 0;
    size_t              end;

    while((end = this->find(sep, start)) != std::string::npos)
    {
        ret.push_back(String(this->substr(start, end - start)));
        start = end +1;
    }

    ret.push_back(this->substr(start));
    return ret;
}

std::vector<String> String::split(String sep)
{
    std::vector<String> ret;
    size_t              start = 0;
    size_t              end;

    while((end = this->find(sep, start)) != std::string::npos)
    {
        ret.push_back(String(this->substr(start, end - start)));
        start = end + sep.size();
    }

    ret.push_back(this->substr(start));
    return ret;
}

std::vector<String> String::split()
{
    std::vector<String> ret;
    size_t              start = 0;
    size_t              end;

    while((end = this->find(' ', start)) != std::string::npos)
    {
        ret.push_back(String(this->substr(start, end - start)));
        start = end +1;
    }

    ret.push_back(this->substr(start));
    return ret;
}

std::string         String::toStdString()
{
    return std::string(this->c_str());
}

bool    String::contains(char pattern)
{
    auto it = this->find(pattern);
    if (it == std::string::npos)
        return false;
    return true;
}

bool    String::contains(String pattern)
{
    auto it = this->find(pattern);
    if (it == std::string::npos)
        return false;
    return true;
}
