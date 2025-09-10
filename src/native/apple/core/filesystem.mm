#if defined (__APPLE__)
#include <ilen/core/filemanager.h>
#include <ilen/core/application.h>
#include <ilen/core/logs.h>
#import <Foundation/Foundation.h>
using namespace ilen::core;

std::filesystem::path FileManager::s_applicationResourcesPath;
std::filesystem::path FileManager::s_applicationWrittablePath;

void FileManager::Init(){
    NSString *resourcesPath = [[NSBundle mainBundle] resourcePath];
    s_applicationResourcesPath = resourcesPath ? [resourcesPath UTF8String] : "";

    #if ILEN_IOS
    s_applicationResourcesPath /= "assets";
    #endif

    NSArray *urls = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *base = [urls firstObject];

    if(base){
        NSString *appName = [NSString stringWithUTF8String:ilen::application::Application::getApplicationName().c_str()];
        NSString *saveDir = [base stringByAppendingPathComponent:appName];
        [[NSFileManager defaultManager] createDirectoryAtPath:saveDir withIntermediateDirectories:YES attributes:Nil error:Nil];
        s_applicationWrittablePath = [saveDir UTF8String];
    }

    Debug::Log("Resources path: " + s_applicationResourcesPath.string(), DEBUG_PREFIX);
    Debug::Log("Writtable path: " + s_applicationWrittablePath.string(), DEBUG_PREFIX);

    isInited = true;
}

#endif //__APPLE__
