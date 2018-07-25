//
//  FilterStack.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "core/FilterStack.hpp"

#include "core/util/GlslProgLoader.hpp"

namespace core {
    
#pragma mark - Filter
    
    /*
     ivec2 _size;
     double _alpha;
     bool _clearsColorBuffer;
     ColorA _clearColor;
     */
    
    Filter::Filter():
            _alpha(1),
            _clearsColorBuffer(false),
            _clearColor(0,0,0,0)
    {}
    
    Filter::~Filter()
    {}
    
    void Filter::setAlpha(double a) {
        double oldAlpha = _alpha;
        _alpha = saturate(a);
        _alphaChanged(oldAlpha, _alpha);
    }

    gl::BatchRef Filter::_createBlitter(const gl::GlslProgRef &shader) const {
        return gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), shader);
    }
    
    void Filter::_execute(const render_state &state, FboRelay &relay) {
        gl::ScopedFramebuffer sfb(relay.getDst());
        gl::ScopedViewport sv(0,0,_size.x,_size.y);
        
        gl::ScopedMatrices sm;
        gl::setMatricesWindow( _size.x, _size.y, true );
        gl::scale(vec3(_size.x,_size.y,1));
        
        if (_clearsColorBuffer) {
            gl::clear(_clearColor);
        }

        _render(state, relay.getSrc());
        
        // prepare for next pass
        relay.next();
    }

    
#pragma mark - FilterStack

    /*
     gl::FboRef _buffer, _captureBuffer;
     gl::BatchRef _blitter;
     vector<FilterRef> _filters;
     ivec2 _size;
     gl::GlslProgRef _compositeShader, _passthroughCompositeShader;
     */
    
    FilterStack::FilterStack():
            _passthroughCompositeShader(util::loadGlslAsset("core/filters/passthrough.glsl"))
    {}
    
    FilterStack::~FilterStack()
    {}
    
    void FilterStack::push(const FilterRef &filter) {
        _filters.push_back(filter);
        filter->_size = _size;
        filter->_resize(_size);
    }
    
    void FilterStack::push(const initializer_list<FilterRef> &filters) {
        for (const auto &filter : filters) {
            _filters.push_back(filter);
            filter->_size = _size;
            filter->_resize(_size);
        }
    }
    
    void FilterStack::pop() {
        _filters.pop_back();
    }
    
    void FilterStack::remove(const FilterRef &filter) {
        _filters.erase(std::remove(_filters.begin(), _filters.end(), filter), _filters.end());
    }
    
    void FilterStack::clear() {
        _filters.clear();
    }
    
    gl::FboRef FilterStack::execute(const render_state &state, const gl::FboRef &input) {
        
        //
        // Create _buffer to match size of input; note FboFormat doesn't have an
        // equality operator, so we can't check on this condition. I think it's probably an
        // acceptable compromise; we're unlikely to be switching out fbo formats for the input at runtime
        //

        if (!_buffer || input->getSize() != _size /*|| input->getFormat() != _buffer->getFormat()*/) {
            _size = input->getSize();
            _buffer = gl::Fbo::create(_size.x, _size.y, input->getFormat());
            
            for (const auto &f : _filters) {
                f->_size = _size;
                f->_resize(_size);
            }
        }

        FboRelay relay(input, _buffer);
        
        if (!_filters.empty()) {
            for (const auto &f : _filters) {
                if (f->getAlpha() > ALPHA_EPSILON) {
                    f->_execute(state, relay);
                }
            }
        }
        
        // after next() is called on a relay, src is always the previous pass's dst
        return relay.getSrc();
    }
    
    void FilterStack::executeToScreen(const render_state &state, const gl::FboRef &input) {
        auto result = execute(state, input);
        auto shader = _compositeShader ? _compositeShader : _passthroughCompositeShader;
        
        if (_blitter) {
            if (_blitter->getGlslProg() != shader) {
                _blitter->replaceGlslProg(shader);
            }
        } else {
            _blitter = gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), shader);
        }

        const auto viewportSize = state.viewport->getSize();
        gl::ScopedViewport sv(viewportSize);
        
        gl::ScopedMatrices sm;
        gl::setMatricesWindow( viewportSize.x, viewportSize.y, true );
        gl::scale(vec3(viewportSize.x,viewportSize.y,1));
        
        gl::ScopedTextureBind stb(result->getColorTexture(), 0);
        shader->uniform("ColorTex", 0);
        _blitter->draw();
    }
    
    void FilterStack::update(const time_state &time) {
        for (const auto &f : _filters) {
            f->_update(time);
        }
    }
    
    const gl::FboRef &FilterStack::capture(const render_state &state, std::function<void(const render_state&)> renderFunc, ColorA clearColor, const gl::Fbo::Format &fmt) {
        const auto size = state.viewport->getSize();
        if (!_captureBuffer || _captureBuffer->getSize() != size) {
            _captureBuffer = gl::Fbo::create(size.x, size.y, fmt);
        }
        
        gl::ScopedFramebuffer sfb(_captureBuffer);
        gl::ScopedViewport sv(0,0,size.x,size.y);
        
        gl::clear(clearColor);
        renderFunc(state);
        
        return _captureBuffer;
    }

    
}
