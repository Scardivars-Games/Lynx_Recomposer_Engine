#pragma once
#if !LYNX_ONLY_ECS

#include <lynx/headers/gameobject.h>
#include <lynx/headers/core.h>


namespace lynx::game{
    class Cube final : public GameObject {
        public:
            void Init() override;
            void Update() override;
            void OnCollisionStay(const ContactManifold2D& contact) override;
            void OnCollisionExit(const ContactManifold2D& contact) override;
        private:
            float firstXPointerPos = 0;
            bool canJump = false;
            float coyoteTime = 0.0f;
            float defaultCoyoteTime = 0.3f;
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

LYNX_REGISTER_SCRIPT(lynx::game::Cube);
LYNX_REGISTER_SCRIPT(lynx::game::PointerPos);
LYNX_REGISTER_SCRIPT(lynx::game::Ground);
LYNX_REGISTER_SCRIPT(lynx::game::GravityCamera);
LYNX_REGISTER_SCRIPT(lynx::game::PlayerSpawner);
LYNX_REGISTER_SCRIPT(lynx::game::RotateGround);

#endif //!ILEN_ONLY_ECS