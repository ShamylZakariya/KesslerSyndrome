//
//  Filters.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 7/22/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "core/filters/Filters.hpp"

#include "core/util/GlslProgLoader.hpp"
#include "core/Strings.hpp"

namespace core {
    namespace filters {
        
#pragma mark - SimpleGlslFilter

        /*
         gl::GlslProgRef _shader;
         gl::BatchRef _blitter;
         */
        SimpleGlslFilter::SimpleGlslFilter(const gl::GlslProgRef &shader):
                _shader(shader)
        {}
            
        void SimpleGlslFilter::_resize( const ivec2 &newSize ) {
            const vec2 ColorTexSize(newSize);
            int loc = 0;
            if (_shader->findUniform("ColorTexSize", &loc)) {
                _shader->uniform("ColorTexSize", ColorTexSize);
            }
            
            if (_shader->findUniform("ColorTexSizeInverse", &loc)) {
                _shader->uniform("ColorTexSizeInverse", vec2(1/ColorTexSize.x,1/ColorTexSize.y));
            }
        }
            
        void SimpleGlslFilter::_bindUniforms(const render_state &state, const gl::FboRef &input) {
            _shader->uniform("ColorTex", 0);
            _shader->uniform("Alpha", static_cast<float>(getAlpha()));
        }
            
        void SimpleGlslFilter::_render( const render_state &state, const gl::FboRef &input ) {
            if (!_blitter) {
                _blitter = _createBlitter(_shader);
            }
            _bindUniforms(state, input);
            
            gl::ScopedTextureBind stb(input->getColorTexture(), 0);
            _blitter->draw();
        }
            
        
#pragma mark - ColorshiftFilter

        /*
         ColorA _colorOffset, _colorMultiplier;
         */
        ColorshiftFilter::ColorshiftFilter(ColorA offset, ColorA multiplier):
                SimpleGlslFilter(util::loadGlslAsset("core/filters/colorshift.glsl")),
                _colorOffset(offset),
                _colorMultiplier(multiplier)
        {}
        
        void ColorshiftFilter::_bindUniforms(const render_state &state, const gl::FboRef &input) {
            SimpleGlslFilter::_bindUniforms(state, input);
            _shader->uniform("Offset", _colorOffset);
            _shader->uniform("Multiplier", _colorMultiplier);
        }
        
#pragma mark - PixelateFilter
        
       /*
        int _pixelSize;
        */
        PixelateFilter::PixelateFilter(int pixelSize):
                SimpleGlslFilter(util::loadGlslAsset("core/filters/pixelate.glsl")),
                _pixelSize(pixelSize)
        {}
        
        void PixelateFilter::_bindUniforms(const render_state &state, const gl::FboRef &input) {
            SimpleGlslFilter::_bindUniforms(state, input);
            _shader->uniform("PixelSize", static_cast<float>(_pixelSize));
        }

#pragma mark - BlurFilter

        /*
         int _radius;
         string _hPassAsset, _vPassAsset;
         gl::GlslProgRef _hPass, _vPass;
         gl::BatchRef _hBlitter, _vBlitter;
         vector<vec2> _kernel;
         */
        
        BlurFilter::BlurFilter(int radius):
                _hPassAsset("core/filters/blur_h.glsl"),
                _vPassAsset("core/filters/blur_v.glsl"),
                _radius(radius)
        {}
        
        void BlurFilter::setRadius(int r) {
            _radius = r;
            _invalidate();
        }
    
        void BlurFilter::_resize( const ivec2 &newSize ) {
            _invalidate();
        }
        
        void BlurFilter::_invalidate() {
            _hPass.reset();
            _vPass.reset();
            _hBlitter.reset();
            _vBlitter.reset();
        }
        
        void BlurFilter::_create() {
            
            // create the kernel
            const int kSize = _createKernel(_radius, _kernel);
            
            // load shaders, replacing "__SIZE__" with our kernel size
            const map<string,string> substitutions = { { "__SIZE__", str(kSize) } };
            _hPass = util::loadGlslAsset(_hPassAsset, substitutions);
            _vPass = util::loadGlslAsset(_vPassAsset, substitutions);
            
            // create the blitters
            _hBlitter = _createBlitter(_hPass);
            _vBlitter = _createBlitter(_vPass);
            
            // load uniforms
            const vec2 ColorTexSize(_size);
            const vec2 ColorTexSizeInverse(1/ColorTexSize.x, 1/ColorTexSize.y);
            
            _hPass->uniform("ColorTexSizeInverse", ColorTexSizeInverse);
            _hPass->uniform("Kernel", _kernel.data(), kSize);
            
            _vPass->uniform("ColorTexSizeInverse", ColorTexSizeInverse);
            _vPass->uniform("Kernel", _kernel.data(), kSize);
        }
        
        void BlurFilter::_execute(const render_state &state, FboRelay &relay) {
            if (!_hPass) {
                _create();
            }
            
            gl::ScopedMatrices sm;
            gl::setMatricesWindow( _size.x, _size.y, true );
            gl::scale(vec3(_size.x,_size.y,1));
            
            const auto alpha = static_cast<float>(getAlpha());
            
            // hPass
            {
                gl::ScopedFramebuffer sfb(relay.getDst());
                gl::ScopedTextureBind stb(relay.getSrc()->getColorTexture(), 0);
                
                _hPass->uniform("ColorTex", 0);
                _hPass->uniform("Alpha", alpha);
                
                _hBlitter->draw();
            }
            
            relay.next();
            
            // vPass
            {
                gl::ScopedFramebuffer sfb(relay.getDst());
                gl::ScopedTextureBind stb(relay.getSrc()->getColorTexture(), 0);
                _vPass->uniform("ColorTex", 0);
                _vPass->uniform("Alpha", alpha);
                _vBlitter->draw();
            }
            
            relay.next();
            
        }
        
        int BlurFilter::_createKernel( int radius, vector<vec2> &kernel )
        {
            kernel.clear();
            
            for ( int i = -radius; i <= radius; i++ )
            {
                int dist = std::abs( i );
                float mag = 1.0f - ( float(dist) / float(radius) );
                kernel.push_back( vec2( i, sqrt(mag) ));
            }
            
            //
            // normalize
            //
            
            float sum = 0;
            for ( size_t i = 0, N = kernel.size(); i < N; i++ )
            {
                sum += kernel[i].y;
            }
            
            for ( size_t i = 0, N = kernel.size(); i < N; i++ )
            {
                kernel[i].y /= sum;
            }
            
            return static_cast<int>(kernel.size());
        }

    }
}
