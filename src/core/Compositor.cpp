//
//  Compositor.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Compositor.hpp"
#include "GlslProgLoader.hpp"

#include <cinder/gl/scoped.h>

namespace core {
    
#pragma mark - BaseCompositor
    
    BaseCompositor::BaseCompositor() {}
    BaseCompositor::~BaseCompositor() {}

    
#pragma mark - FboCompositor
    
    /*
     gl::FboRef _fbo;
     gl::GlslProgRef _shader;
     gl::BatchRef _batch;
     */
    
    FboCompositor::FboCompositor():
            _shader(core::util::loadGlslAsset("kessler/shaders/fbo_compositor.glsl")),
            _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader))
    {
    }

    FboCompositor::FboCompositor(std::string shaderAssetPath):
    _shader(core::util::loadGlslAsset(shaderAssetPath)),
    _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader))
    {
    }

    FboCompositor::~FboCompositor()
    {}
    
    void FboCompositor::composite(int width, int height) {
        gl::ScopedViewport sv(0,0,width,height);

        gl::ScopedMatrices sm;
        gl::setMatricesWindow( width, height, true );
        
        gl::scale(vec3(width,height,1));
        
        _fbo->getColorTexture()->bind(0);
        _shader->uniform("ColorTex", 0);
        _batch->draw();
    }

    
}
