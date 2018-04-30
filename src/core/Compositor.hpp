//
//  Compositor.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Compositor_hpp
#define Compositor_hpp

#include "Common.hpp"
#include "Viewport.hpp"

#include <cinder/Area.h>
#include <cinder/gl/scoped.h>
#include <cinder/gl/Fbo.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Batch.h>

using namespace ci;
using namespace std;

namespace core {

    SMART_PTR(BaseCompositor);

    /**
     Compositor is a mechanism to composite the output of one or more Fbos to screen (or some other target).
     */
    class BaseCompositor {
    public:
        
        BaseCompositor();
        virtual ~BaseCompositor();

        /**
         Composite input(s) to a target, with a given width & height
         */
        virtual void composite(int width, int height) = 0;
        
        /**
         Standard time update
         */
        virtual void update(const time_state &time){}
        
    };
    
    SMART_PTR(FboCompositor);

    class FboCompositor : public BaseCompositor {
    public:

        // loads default shader asset (core/shaders/fbo_compositor.glsl)
        FboCompositor();

        // path to a .glsl shader asset
        FboCompositor(std::string shaderAssetPath);

        ~FboCompositor();
        
        void setFbo(const gl::FboRef &fbo) { _fbo = fbo; }
        const gl::FboRef &getFbo() const { return _fbo; }
        
        void composite(int width, int height) override;
    
    protected:
        
        gl::FboRef _fbo;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;

    };
    
    class ViewportCompositor : public FboCompositor {
    public:
        
        ViewportCompositor(BaseViewportRef viewport);
        ViewportCompositor(BaseViewportRef viewport, std::string shaderAssetPath);
        
        void composite(int width, int height) override;

    protected:
        
        BaseViewportRef _viewport;
    };
    
}

#endif /* Compositor_hpp */
