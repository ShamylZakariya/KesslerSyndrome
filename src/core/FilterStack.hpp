//
//  FilterStack.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef FilterStack_hpp
#define FilterStack_hpp

#include <cinder/Area.h>
#include <cinder/gl/scoped.h>
#include <cinder/gl/Fbo.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Batch.h>

#include "core/Common.hpp"
#include "core/MathHelpers.hpp"
#include "core/TimeState.hpp"
#include "core/RenderState.hpp"

namespace core {
 
    SMART_PTR(Filter);
    SMART_PTR(FilterStack);
    
    /**
     FboRelay is a helper for implementing "ping pong" style shader sequences where
     a shared reads from one Fbo, writing to another, and then the next shader reads
     from the previous' destination, writing to the previous's source. And so on.
     
     Usage will look something like so:
     
        FboRelay relay(input, tempFboBufferOfSameSizeAsInput)
     
         for shader in shaders {
            writeTo(relay.getDst())
            readFrom(relay.getSrc())
     
            executeShader(shader)
            relay.next()
         }
     
         result = relay.getSrc()
     
     */
    class FboRelay {
    public:
        FboRelay(const gl::FboRef &src, const gl::FboRef &dst):
        _a(src),
        _b(dst)
        {}
        
        inline const gl::FboRef &getSrc() const { return _a; }
        inline const gl::FboRef &getDst() const { return _b; }
        
        void next() {
            std::swap(_a, _b);
        }
        
    private:
        gl::FboRef _a, _b;
    };
    
    /**
     Filter represents a single convolution to be applied to an input Fbo.
     Implementations should at minimum override _render(), but more complex Filters, such as separable convolutions requiring 2 or more passes
     may choose to override _execute() to gain finer control.
     Filters should not be used directly, rather, one should create a FilterStack and add Filters to it.
     */
    class Filter {
    public:
        
        Filter();
        virtual ~Filter();
        
        /**
         Set the "strength" of this filter, in range [0,1]. This may be a simple blending operation, but it could also
         be interpreted by the filter in a custom manner, e.g., a gaussian blur might use this value to modulate the
         radius of the blur kernel.
         Note: A filter with alpha of 0 is skipped; Expensive effects which you don't always want running can have their
         alpha set to zero when not in use.
         */
        void setAlpha(double a);

        /// Get the current strength of the filter.
        double getAlpha() const { return _alpha; }
        
        void setClearsColorBuffer(bool clears) { _clearsColorBuffer = clears; }
        bool clearsColorBuffer() const { return _clearsColorBuffer; }
        
        void setClearColor(ColorA clearColor) { _clearColor = clearColor; }
        ColorA getClearColor() const { return _clearColor; }
        
    protected:

        friend class FilterStack;
        
        /// Called by FilterStack when it's resized; Note: FilterStack directly sets _size before calling this.
        virtual void _resize( const ivec2 &newSize ) {}
        
        /// Called via setAlpha() to respond to changes to alpha
        virtual void _alphaChanged(double oldAlpha, double newAlpha) {}

        /// Perform time-based updates
        virtual void _update(const time_state &time) {}

        /// Creates a Batch suitable for drawing a screen-aligned quad with the viewport and scaling set up by _execute()
        virtual gl::BatchRef _createBlitter(const gl::GlslProgRef &shader) const;

        /// Called by FilterStack to prepare render state; you should most-likely implement _render() and leave this alone
        virtual void _execute(const render_state &state, FboRelay &relay);
        
        /// Perform your filtered render using input as your source texture data. Destination Fbo is already bound by _execute().
        virtual void _render( const render_state &state, const gl::FboRef &input ){}

    protected:
        
        /// Current width/height of the FilterStack
        ivec2 _size;
        double _alpha;
        bool _clearsColorBuffer;
        ColorA _clearColor;
        
    };
    
    /**
     FilterStack is a stack of Filter objects, performing a series of convolutions on an input Fbo to produce an output Fbo.
     The Filters are executed in order of first to last. Adding a Filter causes it to be executed after any already in the stack
     */
    class FilterStack {
    public:
        
        FilterStack();
        
        virtual ~FilterStack();
        
        /// Assign an Fbo format to use when creating backing stores
        void setFboFormat(gl::Fbo::Format fboFormat);
        
        /// push a filter to end of stack, will be executed after any already in stack
        void push(const FilterRef &filter);
        
        /// push a set of filters to end of stack; they'll be executed after existing filters, and will be executed in order of definition
        void push(const initializer_list<FilterRef> &filters);
        
        /// pop the last filter in the stack
        void pop();

        /// remove a specific filter from the stack
        void remove(const FilterRef &filter);
        
        /// remove all filters from the stack
        void clear();
        
        /// return true if the filter stack is empty
        bool isEmpty() const { return _filters.empty(); }
        
        /// Get all the Filter instances used by this stack
        const vector<FilterRef> getFilters() const { return _filters; }
        
        /// Convenience function to get the first filter in the stack of a particular type, e.g., getFilter<ToneMapFilter>()
        template<typename T>
        shared_ptr<T> getFilter() {
            for (const auto &f : _filters) {
                shared_ptr<T> typedF = dynamic_pointer_cast<T>(f);
                if (typedF) {
                    return typedF;
                }
            }
            return nullptr;
        }
        
        /**
         Execute the filters in order against the contents of `input, returning an Fbo containing the result.
         The returned Fbo belongs to FilterStack and will be overwritten in subsequent calls to execute(), so
         draw it immediately, or make a copy somehow if you intend to hold on to the results.
        */
        virtual gl::FboRef execute(const render_state &state, const gl::FboRef &input);
        
        /**
         Assign a shader to composite result to screen, when executeToScreen finishes executing its filters.
         compositeShader requires the following uniforms:
         - uniform sampler2D ColorTex;
         
         If none is assigned, executeToScreen will simply use the passthrough shader (core/filters/passthrough.glsl)
         */
        virtual void setScreenCompositeShader(const gl::GlslProgRef &compositeShader) {
            _compositeShader = compositeShader;
        }
        
        /**
         Get the composite shader. If none has been assigned, executeToScreen will use the default passthrough shader.
         */
        gl::GlslProgRef getScreenCompositeShader() const { return _compositeShader; }
        
        /**
         Execute the filters in order against the contents of input, rendering result to screen using the screenCompositeShader
         if assigned, otherwise using the default passthrough shader (core/filters/passthrough.glsl).
         Custom composite operations may be handled by overriding FilterStack::performScreenComposite where you may bind
         custom uniforms, extra textures, etc
         */
        virtual void executeToScreen(const render_state &state, const gl::FboRef &input);
        
        /**
         Dispatch time-based updates to all filters in the stack
         */
        virtual void update(const time_state &time);
        
        /**
         Convenience method to capture render output into an Fbo.
         As an example usage, a set of drawing commands might be performed, captured, and have a sequence of filters executed against them and then composited to screen:
         @code MyThing::draw(const render_state &state) {
            auto result = filterStack->capture(state, [this](const render_state &state){
                // perform drawing commands
            });
            filterStack->executeToScreen(state, result);
         }
         */
        virtual const gl::FboRef &capture(const render_state &state, std::function<void(const render_state&)> renderFunc, ColorA clearColor = ColorA(0,0,0,0), const gl::Fbo::Format &fmt = gl::Fbo::Format().disableDepth());
        
        /// return the capture buffer Fbo used by capture(); this will have the last value captured.
        gl::FboRef getCaptureFbo() const { return _captureBuffer; }
        
    protected:
        
        /**
         Performs final rendering operation for FilterStack::executeToScreen. Default implementation
         binds the color buffer to texture 0 as "ColorTex". Custom implementations may want to
         bind other textures, set custom uniforms for a custom screenCompositeShader, etc.
         */
        virtual void performScreenComposite(const render_state &state, const gl::GlslProgRef &shader, const gl::FboRef &color);
        
    protected:
        
        gl::FboRef _buffer, _captureBuffer;
        gl::BatchRef _blitter;
        vector<FilterRef> _filters;
        ivec2 _size;
        gl::GlslProgRef _compositeShader, _passthroughCompositeShader;
        
    };
    
}

#endif /* FilterStack_hpp */
