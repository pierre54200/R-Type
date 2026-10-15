#include <iostream>
#include <string>

#include "Client/Client.hpp"

int main(int ac, const char *const *av) {
    try {
        std::string ip = ac > 1 ? av[1] : "127.0.0.1";
        int port = ac > 2 ? std::stoi(av[2]) : 4242;
        std::string name = ac > 3 ? av[3] : "joueur";

        Client client(ip, port, name);
        client.run();
    } catch (const std::exception &e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 84;
    }
    return 0;
}
