#include <iostream>
#include <cstdlib>


int main(void) {
    const char* portEnv = std::getenv("SERVER_PORT");
    std::cout << portEnv << std::endl;
}