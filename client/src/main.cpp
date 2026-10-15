#include <iostream>
#include <string>

#include <unistd.h>

#include "Client/Client.hpp"

// Lit une ligne directement sur l'entree standard (sans le buffer de std::cin,
// pour que le client recoive bien tout ce qui est tape ensuite)
static bool readLine(std::string &line) {
    line.clear();
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n')
            return true;
        line += c;
    }
    return !line.empty();
}

int main(int ac, const char *const *av) {
    try {
        std::string ip = ac > 1 ? av[1] : "127.0.0.1";
        int port = ac > 2 ? std::stoi(av[2]) : 4242;

        // Demande le pseudo avant de se connecter
        std::string name;
        while (name.empty()) {
            std::cout << "Pseudo : " << std::flush;
            if (!readLine(name))
                return 0;
        }

        Client client(ip, port, name);
        client.run();
    } catch (const std::exception &e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 84;
    }
    return 0;
}
