#pragma once
#include <algorithm>
#include <functional>
#include <utility>

#include "Registery.hpp"
#include "../Modules/NetworkSync.hpp"
#include "../Modules/Transform.hpp"
#include "Protocol.hpp"

class   NetworkSendSystem : public ECS::System
{
    public:
        using   SendFunction    =   std::function<void(const Packet&)>;
        static inline std::vector<Packet>  queue{};

        void    setSender(SendFunction send)
        {
            _send = std::move(send);
        }

        void    update(ECS::Registery& registery, float dt) override
        {
            if (!_send)
                return;
            registery.view<Transform, NetworkSync>(getEntities()).each(
                [&](Entity, Transform& transform, NetworkSync& sync)
                {
                    sync.elapsed += dt;
                    if (sync.hasSent && sync.sendRate > 0.f && sync.elapsed < 1.f / sync.sendRate)
                        return;

                    const auto      position = positionOf(transform);
                    const uint16_t  x = to16Bit(position.first);
                    const uint16_t  y = to16Bit(position.second);
                    if (sync.hasSent && x == sync.lastX && y == sync.lastY)
                    {
                        sync.elapsed = sync.sendRate > 0.f ? 1.f / sync.sendRate : 0.f;
                        return;
                    }

                    _send(makePosition({sync.id, x, y, sync.name}));
                    sync.elapsed = 0.f;
                    sync.hasSent = true;
                    sync.lastX = x;
                    sync.lastY = y;
                });
            for (auto& p: this->queue)
            {
                _send(p);
            }
            queue.clear();
        }

    private:
        static std::pair<float, float>  positionOf(const Transform& transform)
        {
            return {transform.position.x, transform.position.y};
        }

        static uint16_t to16Bit(float value)
        {
            return static_cast<uint16_t>(std::clamp(value, 0.f, 65535.f) + 0.5f);
        }

        SendFunction    _send;
};
