#include "EntityManager.hpp"
#include "./ECS/Modules/DespawnByRange.hpp"
#include "./ECS/Modules/FrontEnd.hpp"
#include "./ECS/Modules/Health.hpp"
#include "./ECS/Modules/HitBox.hpp"
#include "./ECS/Modules/NetworkSync.hpp"
#include "./ECS/Modules/RemotePlayer.hpp"
#include "./ECS/Modules/Transform.hpp"
#include "./ECS/Modules/Window.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdint>
#include <iostream>
#include <string>

#include "Network/NetworkClient.hpp"
#include "./ECS/System/ClearDisplaySystem.hpp"
#include "./ECS/System/CollisionSystem.hpp"
#include "./ECS/System/DespawnSystem.hpp"
#include "./ECS/System/DisplaySystem.hpp"
#include "./ECS/System/EventSystem.hpp"
#include "./ECS/System/HealthBarDisplaySystem.hpp"
#include "./ECS/System/HurtSystem.hpp"
#include "./ECS/System/MovementSystem.hpp"
#include "./ECS/System/NetworkReceiveSystem.hpp"
#include "./ECS/System/NetworkSendSystem.hpp"
#include "./ECS/System/PlayerControlSystem.hpp"
#include "./ECS/System/UpdateDisplaySystem.hpp"

// Chaque joueur distant a sa couleur (le joueur local est rouge)
static sf::Color    colorFor(uint8_t id)
{
    static const sf::Color  palette[] = {sf::Color::Cyan, sf::Color::Magenta, sf::Color::White, sf::Color::Yellow};
    return palette[id % 4];
}

static int  run(const std::string& ip, int port, const std::string& playerName, const std::string& lobby)
{
    NetworkClient   network(ip, port, playerName);

    network.send({PacketType::LobbyCreate, lobby});
    network.send({PacketType::LobbyJoin, lobby});

    ECS::Registery reg;

    reg.registerModule<Transform>()
        .registerModule<RigidBody>()
        .registerModule<FrontEnd>()
        .registerModule<Window<sf::RenderWindow>>()
        .registerModule<EventQueue>()
        .registerModule<PlayerControl<sf::Keyboard::Key>>()
        .registerModule<HitBox>()
        .registerModule<Health>().registerModule<DespawnByRange>()
        .registerModule<NetworkSync>()
        .registerModule<RemotePlayer>();

    reg.registerSystem<EventSystem>(reg.signatureOf<Window<sf::RenderWindow>, EventQueue>());
    auto    networkReceive = reg.registerSystem<NetworkReceiveSystem>(reg.signatureOf<RemotePlayer>());
    networkReceive->setLocalName(playerName);
    networkReceive->setReceiver([&network]() { return network.receive(); });
    networkReceive->setSpawner([](ECS::Registery& registery, const PlayerPosition& position)
    {
        Entity  remote = registery.createEntity();
        registery.addModule(remote, Transform{ {static_cast<float>(position.x), static_cast<float>(position.y), 0.f}, {50.f, 50.f, 0.f} })
            .addModule(remote, FrontEnd{ true, false, 0, colorFor(position.id) });
        return remote;
    });
    reg.registerSystem<PlayerControlSystem>(reg.signatureOf<PlayerControl<sf::Keyboard::Key>, RigidBody, Transform>());
    reg.registerSystem<MovementSystem>(reg.signatureOf<Transform, RigidBody>());
    reg.registerSystem<CollisionSystem>(reg.signatureOf<Transform, RigidBody>());
    auto    networkSend = reg.registerSystem<NetworkSendSystem>(reg.signatureOf<Transform, NetworkSync>());
    networkSend->setSender([&network](const Packet& packet) { network.send(packet); });
    reg.registerSystem<HurtSystem>(reg.signatureOf<HitBox, Health, Transform>());
    reg.registerSystem<DespawnSystem>(reg.signatureOf<Transform, DespawnByRange>());
    reg.registerSystem<ClearDisplaySystem>(reg.signatureOf<>());
    reg.registerSystem<HealtBarDisplaySystem>(reg.signatureOf<Transform, Health, FrontEnd>());
    reg.registerSystem<DisplaySystem>(reg.signatureOf<Transform, FrontEnd>());
    reg.registerSystem<UpdateDisplaySystem>(reg.signatureOf<>());


    Entity screen = reg.createEntity();

    reg.addModule(screen, Window<sf::RenderWindow>{
            std::make_shared<sf::RenderWindow>(sf::VideoMode(600, 800), "Game"),
            sf::Color::Black,
            false})
        .addModule(screen, EventQueue{});

    reg.getModule<Window<sf::RenderWindow>>(screen).window->setFramerateLimit(60);

    EventSystem::screen             =   screen;
    DisplaySystem::screen           =   screen;
    ClearDisplaySystem::screen      =   screen;
    UpdateDisplaySystem::screen     =   screen;
    HealtBarDisplaySystem::screen   =   screen;

    Entity square = reg.createEntity();

    reg.addModule(square, Transform{ {100.f, 100.f, 0.f}, {50.f, 50.f, 0.f} })
        .addModule(square, RigidBody{ {0.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{1}, false })
        .addModule(square, FrontEnd{ true, false, 0, sf::Color::Red })
        .addModule(square, PlayerControl<sf::Keyboard::Key>{})
        .addModule(square, Health{100.f, false, 100.f})
        .addModule(square, HitBox{{0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, std::set<uint32_t>{1}})
        .addModule(square, NetworkSync{0, playerName});

    auto&   controls = reg.getModule<PlayerControl<sf::Keyboard::Key>>(square);
    controls.registerKey("up", sf::Keyboard::Key::W).
        registerKey("down", sf::Keyboard::Key::S).
        registerKey("right", sf::Keyboard::Key::D).
        registerKey("left", sf::Keyboard::Key::A).
        registerKey("shoot", sf::Keyboard::Key::Space);

    Entity wall = reg.createEntity();

    reg.addModule(wall, Transform{ {300.f, 300.f, 0.f}, {100.f, 100.f, 0.f} })
        .addModule(wall, RigidBody{ {0.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{1}, true })
        .addModule(wall, FrontEnd{ true, false, 0, sf::Color::Blue });

    Entity hurt = reg.createEntity();

    reg.addModule(hurt, Transform{ {100.f, 500.f, 0.f}, {100.f, 100.f, 0.f}})
        .addModule(hurt, FrontEnd{ true, false, 0, sf::Color::Yellow})
        .addModule(hurt, HitBox{  {0.f, 0.f, 0.f}, {100.f, 100.f, 0.f}, std::set<uint32_t>{1}})
        .addModule(hurt, Health{1.f, true, 0.f});

    Entity  enemy = reg.createEntity();

    reg.addModule(enemy, Transform { {500.f, 100.f, 0.f}, {50.f, 50.f, 0.f} })
        .addModule(enemy, FrontEnd{ true, false, 1, sf::Color::Red})
        .addModule(enemy, Health{100.f, false, 100.f})
        .addModule(enemy, HitBox{
            .topLeft           = {0.f, 0.f, 0.f},
            .size              = {50.f, 50.f, 0.f},
            .mask              = std::set<uint32_t>{2},
            .hitCooldownValue  = 0.25f,
        });

    reg.run();

    return 0;
}

int main(int ac, const char *const *av)
{
    try {
        const std::string   ip      =   ac > 1 ? av[1] : "127.0.0.1";
        const int           port    =   ac > 2 ? std::stoi(av[2]) : 4242;
        const std::string   name    =   ac > 3 ? av[3] : "Player";
        const std::string   lobby   =   ac > 4 ? av[4] : "default";
        return run(ip, port, name, lobby);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 84;
    }
}
