//
//  FilterStack.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "core/FilterStack.hpp"


namespace core {
    
#pragma mark - Filter
    
    /*
     ivec2 _size;
     double _alpha;
     */
    
    Filter::Filter():
            _alpha(1)
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

        _render(state, relay.getSrc());
        
        // prepare for next pass
        relay.next();
    }

    
#pragma mark - FilterStack

    /*
     gl::FboRef _buffer;
     vector<FilterRef> _filters;
     ivec2 _size;
     */
    
    FilterStack::FilterStack()
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
    
    void FilterStack::update(const time_state &time) {
        for (const auto &f : _filters) {
            f->_update(time);
        }
    }
    
}
