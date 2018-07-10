//
//  Compositor.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include <cinder/gl/scoped.h>

#include "core/Compositor.hpp"
#include "core/util/GlslProgLoader.hpp"

namespace core {
    
#pragma mark - BaseCompositor
    
    BaseCompositor::BaseCompositor() {}
    BaseCompositor::~BaseCompositor() {}

    
#pragma mark - FboCompositor
    
    /*
     gl::FboRef _fbo;
     gl::GlslProgRef _shader;
     gl::BatchRef _batch;
     GLenum _srcRgb, _dstRgb, _srcAlpha, _dstAlpha;
     */
    
    FboCompositor::FboCompositor():
            _shader(core::util::loadGlslAsset("core/shaders/fbo_compositor.glsl")),
            _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader)),
            _srcRgb(GL_SRC_ALPHA),
            _dstRgb(GL_ONE_MINUS_SRC_ALPHA),
            _srcAlpha(GL_ONE),
            _dstAlpha(GL_ZERO)
    {
    }

    FboCompositor::FboCompositor(std::string shaderAssetPath):
            _shader(core::util::loadGlslAsset(shaderAssetPath)),
            _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader)),
            _srcRgb(GL_SRC_ALPHA),
            _dstRgb(GL_ONE_MINUS_SRC_ALPHA),
            _srcAlpha(GL_ONE),
            _dstAlpha(GL_ZERO)
    {
    }

    FboCompositor::~FboCompositor()
    {}
    
    void FboCompositor::setBlend(GLenum srcRgb, GLenum dstRgb, GLenum srcAlpha, GLenum dstAlpha) {
        _srcRgb = srcRgb;
        _dstRgb = dstRgb;
        _srcAlpha = srcAlpha;
        _dstAlpha = dstAlpha;
    }
    
    void FboCompositor::composite(int width, int height) {
        
        gl::ScopedViewport sv(0,0,width,height);

        gl::ScopedMatrices sm;
        gl::setMatricesWindow( width, height, true );
        
        gl::scale(vec3(width,height,1));
        
        _fbo->getColorTexture()->bind(0);
        _shader->uniform("ColorTex", 0);

        gl::ScopedBlend sb(_srcRgb, _dstRgb, _srcAlpha, _dstAlpha);
        _batch->draw();
    }

#pragma mark - ViewportCompositor
    
    ViewportCompositor::ViewportCompositor(BaseViewportRef viewport):
            _viewport(viewport)
    {}
    
    ViewportCompositor::ViewportCompositor(BaseViewportRef viewport, std::string shaderAssetPath):
            core::FboCompositor(shaderAssetPath),
            _viewport(viewport)
    {}

    void ViewportCompositor::composite(int width, int height) {
        setFbo(_viewport->getFbo());
        FboCompositor::composite(width,height);
    }
}
