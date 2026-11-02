#include "ECS/Modules/EnemyStat.hpp"
#include "ECS/Modules/EventQueue.hpp"
#include "ECS/Modules/Function.hpp"
#include "ECS/Modules/PlayerStat.hpp"
#include "ECS/Modules/SceneManager.hpp"
#include "ECS/Modules/SceneTag.hpp"
#include "ECS/Modules/SpawnerSettings.hpp"
#include "ECS/Modules/Targetable.hpp"
#include "ECS/System/ClickSystem.hpp"
#include "ECS/System/PlayerControlSystem.hpp"
#include "ECS/System/SceneSystem.hpp"
#include "ECS/System/ShootSystem.hpp"
#include "ECS/System/SpawnSystem.hpp"
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
#include "./ECS/System/SceneSystem.hpp"
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
#include "./ECS/System/TargetSystem.hpp"
#include "./ECS/System/EnemyLogicSystem.hpp"

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

    reg
        .registerModule<Controls<sf::Keyboard::Key>>()
        .registerModule<DespawnByRange>()
        .registerModule<EnemyStat>()
        .registerModule<EventQueue>()
        .registerModule<FrontEnd>()
        .registerModule<Function>()
        .registerModule<Health>()
        .registerModule<HitBox>()
        .registerModule<NetworkSync>()
        .registerModule<PlayerStat>()
        .registerModule<RemotePlayer>()
        .registerModule<RigidBody>()
        .registerModule<SceneManager>()
        .registerModule<SceneTag>()
        .registerModule<SpawnerSettings>()
        .registerModule<Targetable>()
        .registerModule<Transform>()
        .registerModule<Window<sf::RenderWindow>>();

    reg.registerSystem<SceneSystem>(reg.signatureOf<SceneTag>());
    reg.registerSystem<EventSystem>(reg.signatureOf<Window<sf::RenderWindow>, EventQueue>());
    reg.registerSystem<ClickSystem>(reg.signatureOf<Function, Transform>());
    auto    networkReceive = reg.registerSystem<NetworkReceiveSystem>(reg.signatureOf<RemotePlayer>());
    networkReceive->setLocalName(playerName);
    networkReceive->setReceiver([&network]() { return network.receive(); });
    networkReceive->setSpawner([](ECS::Registery& registery, const PlayerPosition& position)
    {
        Entity  remote = registery.createEntity();
        registery.addModule(remote, Transform{ {static_cast<float>(position.x), static_cast<float>(position.y), 0.f}, {50.f, 50.f, 0.f} })
            .addModule(remote, FrontEnd{ true, false, 0, colorFor(position.id) })
            .addModule(remote, Targetable{.id = 0});
        return remote;
    });
    reg.registerSystem<PlayerControlSystem>(reg.signatureOf<PlayerStat, RigidBody, Transform, SceneTag>());
    reg.registerSystem<TargetSystem>(reg.signatureOf<Targetable, Transform>());
    reg.registerSystem<EnemyLogicSystem>(reg.signatureOf<EnemyStat, Transform, RigidBody>());
    reg.registerSystem<ShootSystem>(reg.signatureOf<Transform, PlayerStat, SceneTag>());
    reg.registerSystem<MovementSystem>(reg.signatureOf<Transform, RigidBody>());
    reg.registerSystem<CollisionSystem>(reg.signatureOf<Transform, RigidBody>());
    auto    networkSend = reg.registerSystem<NetworkSendSystem>(reg.signatureOf<Transform, NetworkSync>());
    networkSend->setSender([&network](const Packet& packet) { network.send(packet); });
    reg.registerSystem<HurtSystem>(reg.signatureOf<HitBox, Health, Transform>());
    reg.registerSystem<SpawnerSystem>(reg.signatureOf<SpawnerSettings>());
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

    Entity sceneMgr = reg.createEntity();
    reg.addModule(sceneMgr, SceneManager{});

    Entity Ctrl = reg.createEntity();

    reg.addModule(Ctrl, Controls<sf::Keyboard::Key>{});
    reg.getModule<Controls<sf::Keyboard::Key>>(Ctrl)
        .registerKey("up", sf::Keyboard::Key::W).
            registerKey("down", sf::Keyboard::Key::S).
            registerKey("right", sf::Keyboard::Key::D).
            registerKey("left", sf::Keyboard::Key::A).
            registerKey("shoot", sf::Keyboard::Key::Space).
            registerKey("attack", sf::Keyboard::Key::F);

    EventSystem::screen             =   screen;
    DisplaySystem::screen           =   screen;
    ClearDisplaySystem::screen      =   screen;
    UpdateDisplaySystem::screen     =   screen;
    HealtBarDisplaySystem::screen   =   screen;
    SceneSystem::manager            =   sceneMgr;
    SpawnerSystem::manager          =   sceneMgr;
    PlayerControlSystem::controls   =   Ctrl;


    auto& sm = reg.getModule<SceneManager>(sceneMgr);

    sm.add("game", SceneDef{
        .onEnter = [playerName](ECS::Registery& reg, SceneId s)
        {
            Entity player = reg.createEntity();

            reg.addModule(player, SceneTag{s})
                .addModule(player, Transform{ {100.f, 100.f, 0.f}, {50.f, 50.f, 0.f} })
                .addModule(player, RigidBody{ {0.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{1}, false })
                .addModule(player, FrontEnd{ true, false, 0, sf::Color::Red })
                .addModule(player, PlayerStat{})
                .addModule(player, Health{100.f, 100.f, 0.f, 0})
                .addModule(player, HitBox{{0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, std::set<uint32_t>{1}})
                .addModule(player, NetworkSync{0, playerName})
                .addModule(player, Targetable{.id = 0});

            auto makeWall = [&reg, s](float x, float y, float w, float h)
            {
                Entity wall = reg.createEntity();

                reg.addModule(wall, SceneTag{s})
                    .addModule(wall, Transform{ {x, y, 0.f}, {w, h, 0.f} })
                    .addModule(wall, RigidBody{ {0.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{1}, true })
                    .addModule(wall, FrontEnd{ true, false, 0, sf::Color::Blue });
            };

            const float t = 20.f; // wall thickness

            makeWall(0.f,        0.f,         600.f, t);      // top
            makeWall(0.f,        800.f - t,   600.f, t);      // bottom
            makeWall(0.f,        0.f,         t,     800.f);  // left
            makeWall(600.f - t,  0.f,         t,     800.f);  // right


            Entity spawner = reg.createEntity();

            SpawnerSettings settings;
            settings.time    = 1.f;                      // seconds between spawns
            settings.spawnClock.setTargetTime(settings.time);

            reg.addModule(spawner, SceneTag{s})
                .addModule(spawner, std::move(settings));
        }
    });

    SceneId menu = sm.add("menu", SceneDef{
        .onEnter = [](ECS::Registery&   reg,    SceneId s)
        {
            Entity playbutton = reg.createEntity();

            reg.addModule(playbutton, SceneTag{s}).addModule(playbutton, Transform{{0.f, 0.f, 0.f}, {200.f, 200.f, 0.f}})
                .addModule(playbutton, Function{[](ECS::Registery&  reg){std::cout << "DOING" << std::endl;reg.getModule<SceneManager>(SceneSystem::manager).switchTo("game");}});
        }
    });

    sm.switchTo(menu);

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
