#pragma once

class {{ClassName}} {
public:
    static {{ClassName}}& instance();

    {{ClassName}}(const {{ClassName}}&) = delete;
    {{ClassName}}& operator=(const {{ClassName}}&) = delete;

private:
    {{ClassName}}() = default;
};
