//
//  Filters.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 7/22/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Filters_hpp
#define Filters_hpp

#include "core/FilterStack.hpp"

namespace core {
    namespace filters {
        
        /**
         SimpleGlslFilter
         Base class for simple single-pass filters which simply use a single shader to convolve an image.
         Implementations can likely just override _bindUniforms(), (calling inherited!) to set needed uniforms.
         
         See ColorshiftFilter and PixelateFilter for reference.
         
         Your filter shaders can assume the following uniforms will be accessible:
         - uniform float Alpha; // [0,1] the amount to apply this filter
         - uniform sampler2D ColorTex; // the input texture data
         - uniform vec2 ColorTexSize; // the size of the input texture data
         - uniform vec2 ColorTexSizeInverse; // inverse of the size of input texture data
         
         */
        class SimpleGlslFilter : public Filter {
        public:

            SimpleGlslFilter(const gl::GlslProgRef &shader);
            
        protected:
            
            void _resize( const ivec2 &newSize ) override;
            
            virtual void _bindUniforms(const render_state &state, const gl::FboRef &input);
            
            void _render( const render_state &state, const gl::FboRef &input ) override;
            
        protected:
            
            gl::GlslProgRef _shader;
            gl::BatchRef _blitter;
            
        };
        
        /**
         ColorshiftFilter
         ColorshiftFilter is a simple filter implementing a linear transform like y=mx+b, where
         the output color is the input color scaled by "multiplier", and then added to "offset".
         */
        class ColorshiftFilter : public SimpleGlslFilter {
        public:
            ColorshiftFilter(ColorA offset = ColorA(0,0,0,0), ColorA multiplier = ColorA(1,1,1,1));
            
            void setColorOffset(ColorA offset) { _colorOffset = offset; }
            ColorA getColorOffset() const { return _colorOffset; }
            
            void setColorMultiplier(ColorA offset) { _colorMultiplier = offset; }
            ColorA getColorMultiplier() const { return _colorMultiplier; }
            
        protected:
            
            void _bindUniforms(const render_state &state, const gl::FboRef &input) override;
            
        private:
            
            ColorA _colorOffset, _colorMultiplier;
            
        };
        
        /**
         PixelateFilter
         PixelateFilter implements a pixelated output, similar to the SNES mode-7 effects where the screen
         would go blocky to effect some kind of scene transition.
         */
        class PixelateFilter : public SimpleGlslFilter {
        public:
            
            PixelateFilter(int pixelSize);
            
            void setPixelSize(int ps) { _pixelSize = max<int>(ps, 1); }
            int getPixelSize() const { return _pixelSize; }
            
        protected:
            
            void _bindUniforms(const render_state &state, const gl::FboRef &input) override;
            
        protected:
            
            int _pixelSize;
            
        };
        
        
        /**
         BlurFilter
         Implements a box blur using a two-pass convolution. BlurFilter requires multi-pass rendering and should be viewed
         as a template for how to implement other multipass filters using FboRelay.
         */
        class BlurFilter : public Filter {
        public:
            
            BlurFilter(int radius);
            
            void setRadius(int r);
            
            int getRadius() const { return _radius; }
            
        protected:
            
            void _resize( const ivec2 &newSize ) override;
            
            void _invalidate();
            
            void _create();
            
            void _execute(const render_state &state, FboRelay &relay) override;
            
            int _createKernel( int radius, vector<vec2> &kernel );
            
        protected:
            
            int _radius;
            string _hPassAsset, _vPassAsset;
            gl::GlslProgRef _hPass, _vPass;
            gl::BatchRef _hBlitter, _vBlitter;
            vector<vec2> _kernel;
            
        };
        
    }
}

#endif /* Filters_hpp */
