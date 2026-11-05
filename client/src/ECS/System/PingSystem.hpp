#pragma once
#include <functional>
#include <utility>

#include "Protocol.hpp"
#include "Registery.hpp"

class PingSystem : public ECS::System {
public:
  using SendFunction = std::function<void(const Packet &)>;

  void setSender(SendFunction send) { _send = std::move(send); }

  void update(ECS::Registery &, float dt) override {
    if (!_send)
      return;
    _time += dt;
    if (_time >= 1.f) {
      _send({PacketType::Ping, ""});
      _time = 0.f;
    }
  }

private:
  float _time = 0.f;
  SendFunction _send;
};