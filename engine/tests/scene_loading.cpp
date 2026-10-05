// Headless integration test: real scene reader, serializers, ECS and job system.
// Only resource paths, logging and GPU asset readiness are replaced below.
#include <ilen/resources/scene_manager.h>
#include <ilen/resources/assets_manager.h>
#include <ilen/resources/mesh_manager.h>
#include <ilen/resources/shaders_manager.h>
#include <ilen/resources/materials_manager.h>
#include <ilen/resources/render_pipeline_manager.h>
#include <ilen/resources/texture_manager.h>
#include <ilen/render/renderer.h>
#include <ilen/window/window.h>
#include <ilen/headers/ecs_components.h>
#include <ilen/core/filemanager.h>
#include <cassert>
#include <iostream>

// No window or camera system is initialized by this headless test.
ilen::window::IWindow::~IWindow() = default;
void ilen::ecs::systems::CameraSystem::Init(const ilen::EngineContext*) {}
void ilen::ecs::systems::CameraSystem::Update() {}
static std::filesystem::path resourceRoot;
std::filesystem::path ilen::core::FileManager::fileRoute(const std::string& name, ILEN_FILE_TYPE type) {
    const char* folder = type == ILEN_FILE_TYPE_ASSETS_MANIFEST ? "manifests" :
        type == ILEN_FILE_TYPE_PREFAB ? "prefabs" : "scenes";
    return resourceRoot / folder / name;
}
float ilen::resources::AssetsManager::getAssetLoadStatus() { return 1.0f; }
void ilen::internal::Debug::Warning(const char*, const char*, ...) {}
void ilen::internal::Debug::Error(const char* format, const char* prefix, ...) { va_list args; va_start(args, prefix); vfprintf(stderr, format, args); va_end(args); fputc('\n', stderr); }
void ilen::internal::Debug::ExecuteError(const char*, const char*, ...) { std::abort(); }
void ilen::internal::Debug::FatalError(const char*, const char*) { std::abort(); }

int main(int argc, char** argv) {
    assert(argc == 2);
    resourceRoot = argv[1];
    ilen::EngineContext ctx;
    ctx.p_jobSystem = std::make_unique<ilen::job_system::JobSystem>();
    ctx.p_jobSystem->Initializate();
    ctx.p_commandDispatcher = std::make_unique<ilen::job_system::CommandBufferDispatcher>();
    ctx.p_commandDispatcher->Init(ctx.p_jobSystem->AvailableThreads() + 1);
    ctx.p_coordinator = std::make_unique<ilen::ecs::ECSCoordinator>();
    ctx.p_coordinator->init(&ctx);
    ctx.p_assetsManager = std::make_unique<ilen::resources::AssetsManager>();
    ctx.p_sceneManager = std::make_unique<ilen::resources::scene::SceneManager>(&ctx);
    using namespace ilen::ecs::components;
    using namespace ilen::resources::scene;
    for (int iteration = 0; iteration < 30; ++iteration) {
        auto op = iteration % 2 == 0
            ? ctx.p_sceneManager->LoadSceneAsync("TemporalGroundTest")
            : ctx.p_jobSystem->SubmitJobWithResult([&] {
                return ctx.p_sceneManager->LoadSceneAsync("TemporalGroundTest");
              }).get();
        assert(op);
        if (iteration == 0) {
            auto overlap = ctx.p_sceneManager->LoadSceneAsync("TemporalGroundTest");
            assert(overlap->phase == AsyncLoadingPhase::Failed);
        }
        op->allowSceneActivation = false;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!op->isDone() && op->phase != AsyncLoadingPhase::PendingActivation) {
            ctx.p_sceneManager->Update(0);
            assert(std::chrono::steady_clock::now() < deadline);
            std::this_thread::yield();
        }
        assert(op->phase == AsyncLoadingPhase::PendingActivation);
        for (int i = 0; i < 92; ++i) {
            const auto name = "Tierra_" + std::to_string(i);
            auto entity = ctx.p_coordinator->getEntityByName(name);
            assert(entity != ilen::ecs::NULL_ENTITY);
            auto* meta = ctx.p_coordinator->getComponent<EntityMetadataComponent>(entity);
            assert(meta && meta->name == name && static_cast<unsigned>(meta->tags) == 1);
            assert(ctx.p_coordinator->hasComponent<RenderComponent>(entity));
            assert(ctx.p_coordinator->hasComponent<TransformComponent>(entity));
            assert(ctx.p_coordinator->hasComponent<physics::BoxCollider2D>(entity));
            assert(ctx.p_coordinator->hasComponent<physics::Rigidbody2D>(entity));
        }
        op->allowSceneActivation = true;
        ctx.p_sceneManager->Update(0);
        assert(op->phase == AsyncLoadingPhase::Done);
    }
    ctx.p_jobSystem->StopJobSystem();
    std::cout << "Passed: 30 asynchronous loads, all 92 Ground names and five components intact.\n";
}
