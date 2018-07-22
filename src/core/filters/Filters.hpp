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
