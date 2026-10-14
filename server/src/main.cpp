#include <iostream>
#include <string>

#include "Server.hpp"

int main(int ac, const char *const *av) {
    try {
        int port = ac > 1 ? std::stoi(av[1]) : 4242;
        Server server(port);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 84;
    }
    return 0;
}
