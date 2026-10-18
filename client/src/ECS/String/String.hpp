#pragma once

#include <iostream>
#include <vector>

class String: public std::string
{
    public:
        using std::string::string;
        String(std::string);

        std::vector<String> split(char);
        std::vector<String> split(String);
        std::vector<String> split();
        std::string         toStdString();
        bool                contains(char);
        bool                contains(String);
};