#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <lynx/core/bitflags.h>

namespace lynx::resources {
    using AssetID = std::uint32_t;
    inline constexpr AssetID InvalidAssetID = std::numeric_limits<AssetID>::max();
}

namespace lynx::core {
    struct GraphicsOptions {
        LynxGraphicsOptionsFlags flags = 0;
        std::uint32_t instanceBufferSizeInMB = 0;
        std::uint32_t generalBufferSizeInMB = 0;
        std::uint32_t actualFPS = 0;
    };

    struct PhysicsOptions {
        double updatedPerSecond = 0.0f;

        float gravity = 0.0f;
        float linearMovementResistance = 0.0f;
        float velocityZeroThreshold = 0.0f;

        bool solveRealisticCollision = false;
        bool showDebugArrows = false;

        std::array<std::uint32_t, 32> collisionMatrix{};
    };

    struct ECSOptions {
        std::uint32_t initialEntityCapacity = 0;
    };

    using namespace resources;
    struct PredefinedAssetsID {
        AssetID errorShader = InvalidAssetID;
        AssetID errorMaterial = InvalidAssetID;
        AssetID errorTexture = InvalidAssetID;

        AssetID quadMesh = InvalidAssetID;
        AssetID postProcessPresentShader = InvalidAssetID;
        AssetID postProcessCalculateShader = InvalidAssetID;
        AssetID defaultTexture = InvalidAssetID;
        AssetID defaultUnlitOpaqueMaterial = InvalidAssetID;
        AssetID defaultUnlitTransparentMaterial = InvalidAssetID;
    };

    struct RuntimeConfiguration {
        GraphicsOptions graphics;
        PhysicsOptions physics;
        ECSOptions ecs;
        PredefinedAssetsID predefinedAssets;
    };
}