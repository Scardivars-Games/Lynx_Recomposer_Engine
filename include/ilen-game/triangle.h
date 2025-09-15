#pragma once

#include <ilen/headers/gameobject.h>
#include <ilen/headers/core.h>

using namespace ilen::game;

class Triangle : public GameObject {
    public:
        void Init() override;
        void Update() override;
};