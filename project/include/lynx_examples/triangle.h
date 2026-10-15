#pragma once

#if !LYNX_ONLY_ECS

#include <lynx/headers/gameobject.h>
#include <lynx/headers/core.h>


namespace lynx::game{
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


LYNX_REGISTER_SCRIPT(lynx::game::Triangle);
LYNX_REGISTER_SCRIPT(lynx::game::Object3D);


#endif //!ILEN_ONLY_ECS