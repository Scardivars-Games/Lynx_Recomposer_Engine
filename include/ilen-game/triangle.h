#pragma once

#include <ilen/headers/gameobject.h>

using namespace ilen::game;

class Triangle : public GameObject {
    public:
        void Init() override;
        void Update() override;
};