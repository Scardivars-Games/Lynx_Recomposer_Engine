#if defined (__APPLE__)
#include <lynx/core/filemanager.h>
#include <lynx/core/application.h>
#include <lynx/headers/lynx_logs.h>
#import <Foundation/Foundation.h>
using namespace lynx::core;

std::filesystem::path FileManager::s_applicationResourcesPath;
std::filesystem::path FileManager::s_applicationWrittablePath;

void FileManager::Init(){
    @autoreleasepool {
        NSString *resourcesPath = [[NSBundle mainBundle] resourcePath];
        s_applicationResourcesPath = resourcesPath ? [resourcesPath UTF8String] : "";

        #if LYNX_IOS || LYNX_TVOS
        s_applicationResourcesPath /= "assets";
        #endif

        NSArray *urls = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *base = [urls firstObject];

        if(base){
            NSString *appName = [NSString stringWithUTF8String:lynx::application::Application::getApplicationName().c_str()];
            NSString *saveDir = [base stringByAppendingPathComponent:appName];
            [[NSFileManager defaultManager] createDirectoryAtPath:saveDir withIntermediateDirectories:YES attributes:Nil error:Nil];
            s_applicationWrittablePath = [saveDir UTF8String];
        }

        LYNX_LOG_P(DEBUG_PREFIX, "Resources path: %s.", s_applicationResourcesPath.string().c_str());
        LYNX_LOG_P(DEBUG_PREFIX, "Writtable path: %s.", s_applicationWrittablePath.string().c_str());

        isInited = true;
    }
}

#endif //__APPLE__
