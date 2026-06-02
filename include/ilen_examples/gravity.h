#pragma once

#if !ILEN_ONLY_ECS

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>


namespace ilen::game{
    class Cube final : public GameObject {
        public:
            void Init() override;
            void Update() override;
    };

    class PointerPos final: public GameObject {
        void Init() override;
        void Update() override;
    };

    class Ground final : public GameObject {
        public:
            void Init() override;
    };

    class GravityCamera final : public GameObject {
        public:
            void Init() override;
            void LateUpdate() override;
    };


}

#endif //!ILEN_ONLY_ECS