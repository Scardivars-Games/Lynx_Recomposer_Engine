//Este punto de entrada es válido para muchos sistemas operativos, pero no para iOS
#include <TargetConditionals.h>

#if !TARGET_OS_IOS //Si esta app no es de iOS
#include <ilen/headers/core.h>
#include <ilen/headers/visual.h>

using namespace ilen;
using namespace ilen::window;
int main() {
    application::ApplicationMetadata metadata{};
    metadata.applicationName = "ILEN Test";
    metadata.applicationDeveloper = "Scardivar's Games";
    metadata.applicationVersion = "0.0.1";
    metadata.startHeight = 600;
    metadata.startWidth = 800;
    metadata.windowPropertiesFlags = ILEN_WINDOW_CLOSABLE | ILEN_WINDOW_RESIZABLE | ILEN_WINDOW_MAXIMIZABLE | ILEN_WINDOW_WINDOWED;

    AppRuntime app;
    try{
        app.init(metadata);

        while (app.isRunning()) {
            app.update();
        }

        app.shutdown();
    }catch(const core::BasicException& e){
        core::Debug::FatalError(e.what(), e.prefix());
    }

    return 0;
}
#endif
