//Este punto de entrada es válido para muchos sistemas operativos, pero no para iOS
#include <exception>
#include <utility>
#include <game/runtime_options.h>

#include <lynx/core/exceptions.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/core/lifetime.h>

int main(int argc, char* argv[]) {
    auto bootstrap = game::generated::MakeProjectBootstrap();
    auto& application = lynx::AppRuntime::getInstance();

    try {
        application.init(std::move(bootstrap));

        while (application.isRunning()) {
            application.update();
        }
    }catch(const lynx::core::BasicException& exception) {
        LYNX_FATAL_ERROR_P(exception.prefix(),exception.what());
    }catch(const std::exception& exception) {
        LYNX_FATAL_ERROR_P("GAME", exception.what());
    }

    application.shutdown();
    return 0;
}
