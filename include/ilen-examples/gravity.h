#pragma once

#if !ILEN_ONLY_ECS

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>
#include <ilen-public/API/transform.h>


namespace ilen::game{
    class Cube : public GameObject {
        public:
            void Init() override;
            //void Update() override;
    };

    class Ground : public GameObject {
        public:
            void Init() override;
    };

    class GravityCamera : public GameObject {
        public:
            void Init() override;
    };
}

#endif //!ILEN_ONLY_ECS