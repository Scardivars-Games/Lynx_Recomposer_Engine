#pragma once

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>
#include <ilen-public/API/transform.h>


namespace ilen::game{
    class Triangle : public GameObject {
        public:
            void Init() override;
            void Update() override;
    };

    class FalseCamera : public GameObject {
        public:
            void Init() override;
    };
}

