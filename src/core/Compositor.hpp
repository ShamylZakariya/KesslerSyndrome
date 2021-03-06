//
//  Compositor.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Compositor_hpp
#define Compositor_hpp

#include <cinder/Area.h>
#include <cinder/gl/scoped.h>
#include <cinder/gl/Fbo.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Batch.h>

#include "core/Common.hpp"
#include "core/Viewport.hpp"
#include "core/FilterStack.hpp"

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
        virtual void composite(const render_state &state, int width, int height) = 0;
        
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
        
        void setBlend(GLenum srcRgb, GLenum dstRgb, GLenum srcAlpha, GLenum dstAlpha);

        GLenum getBlendSrcRgb() const { return _srcRgb; }
        GLenum getBlendDstRgb() const { return _dstRgb; }
        GLenum getBlendSrcAlpha() const { return _srcAlpha; }
        GLenum getBlendDstAlpha() const { return _dstAlpha; }
        
        void composite(const render_state &state, int width, int height) override;
        void update(const time_state &time) override;
        
        const FilterStackRef getFilterStack() const { return _filterStack; }
    
    protected:
        
        gl::FboRef _fbo;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;
        GLenum _srcRgb, _dstRgb, _srcAlpha, _dstAlpha;
        FilterStackRef _filterStack;

    };
    
    SMART_PTR(ViewportCompositor);

    class ViewportCompositor : public FboCompositor {
    public:
        
        ViewportCompositor(BaseViewportRef viewport);
        ViewportCompositor(BaseViewportRef viewport, std::string shaderAssetPath);
        
        void composite(const render_state &state, int width, int height) override;

    protected:
        
        BaseViewportRef _viewport;
    };
    
}

#endif /* Compositor_hpp */
