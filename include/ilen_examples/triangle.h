#pragma once

#if !ILEN_ONLY_ECS

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>


namespace ilen::game{
    class Triangle : public GameObject {
        public:
            void Init() override;
            void Update() override;
        private:
            float advance = 0.0f;
            bool isRight = true;
    };

    class FalseCamera : public GameObject {
        public:
            void Init() override;
    };

    class Object3D: public GameObject {
        public:
            void Init() override;
            void Update() override;
    };
}

#endif //!ILEN_ONLY_ECS