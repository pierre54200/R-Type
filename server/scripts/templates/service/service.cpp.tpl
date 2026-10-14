#include "{{header}}"

{{ClassName}}& {{ClassName}}::instance() {
    static {{ClassName}} inst;
    return inst;
}
