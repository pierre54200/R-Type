#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Utils/MenuUI.hpp"

// Signature: reg.signatureOf<>()
// Re-requests the lobby list every 2 s while the browser scene is active
class LobbyBrowserSystem : public ECS::System
{
public:
    void update(ECS::Registery&, float dt) override
    {
        if (!MenuUI::browserActive)
        {
            _elapsed = 0.f;
            return;
        }
        _elapsed += dt;
        if (_elapsed >= 2.f)
        {
            _elapsed = 0.f;
            LobbyBrowser::requestList();
        }
    }

private:
    float _elapsed = 0.f;
};