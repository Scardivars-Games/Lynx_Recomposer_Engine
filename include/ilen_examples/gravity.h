#pragma once

#include "ilen/game_logic/game_object.h"
#include "ilen/game_logic/script_registry.h"
#if !ILEN_ONLY_ECS

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>


namespace ilen::game{
    class Cube final : public GameObject {
        public:
            void Init() override;
            void Update() override;
        private:
            float firstXPointerPos = 0;
    };

    class PointerPos final: public GameObject {
        public:
            void Init() override;
            void Update() override;
    };

    class Ground final : public GameObject {
        public:
            void Init() override;
    };

    class PlayerSpawner final: public GameObject {
        public:
            void Init() override;
    };
    
    class GravityCamera final : public GameObject {
        public:
            void Init() override;
            void LateUpdate() override;
    };

    class RotateGround final : public GameObject {
        //public Init() override;
        public:
            void Update() override;
    };
}

ILEN_REGISTER_SCRIPT(ilen::game::Cube);
ILEN_REGISTER_SCRIPT(ilen::game::PointerPos);
ILEN_REGISTER_SCRIPT(ilen::game::Ground);
ILEN_REGISTER_SCRIPT(ilen::game::GravityCamera);
ILEN_REGISTER_SCRIPT(ilen::game::PlayerSpawner);
ILEN_REGISTER_SCRIPT(ilen::game::RotateGround);

#endif //!ILEN_ONLY_ECS