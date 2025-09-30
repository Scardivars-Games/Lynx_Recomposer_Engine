//Este punto de entrada es válido para muchos sistemas operativos, pero no para iOS
#include <TargetConditionals.h>

#if !TARGET_OS_IOS //Si esta app no es de iOS
#include <ilen/headers/core.h>
#include <ilen/headers/visual.h>
#include <ilen/headers/enumerations.h>

using namespace ilen::enumerations;
using namespace ilen;
using namespace ilen::window;
int main() {
    application::ApplicationMetadata metadata{};
    metadata.applicationName = "ILEN Test";
    metadata.applicationDeveloper = "Scardivar's Games";
    metadata.applicationVersion = "0.0.1";
    metadata.startHeight = 600;
    metadata.startWidth = 800;
    metadata.windowPropertiesFlags = ILEN_WINDOW_PROPERTY_CLOSABLE | ILEN_WINDOW_PROPERTY_RESIZABLE | ILEN_WINDOW_PROPERTY_MAXIMIZABLE | ILEN_WINDOW_PROPERTY_WINDOWED;
    metadata.applicationFramerate = 30;

    AppRuntime& app = AppRuntime::getInstance();
    try{
        app.init(metadata);

        while (app.isRunning()) {
            app.update();
        }

        app.shutdown();
    }catch(const core::BasicException& e){
        ILEN_FATAL_ERROR_P(e.prefix(), e.what());
    }

    return 0;
}
#endif
