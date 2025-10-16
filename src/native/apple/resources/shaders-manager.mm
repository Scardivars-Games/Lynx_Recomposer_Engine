//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <ilen/render/gpu-interfaces/shaders.h>
#import <Metal/Metal.h>

namespace  ilen::render {
    class MetalShader: public IShader {
    public:
        MetalShader(id<MTLLibrary> globalLibrary, ilen::graphics::IlenShader shaderData) : _shaderData(shaderData){
            //NSString* vertName = [NSString stringWithUTF8String:shaderData.]
        }
        
        virtual ~MetalShader() { }
        
        void* getNativeVertexFunction() const {
            
        }
        
        void* getNativeFragmentFunction() const {
            
        }
    private:
        ilen::graphics::IlenShader _shaderData;
        __strong id<MTLFunction> _vertexFunction;
        __strong id<MTLFunction> _fragmentFunction;
    };
}


#endif
