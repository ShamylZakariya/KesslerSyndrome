//
//  RenderState.h
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef RenderState_h
#define RenderState_h

#include "TimeState.hpp"
#include "Viewport.hpp"

namespace core {

#pragma mark - RenderMode

    namespace RenderMode {

        enum mode {
            GAME,
            DEVELOPMENT,
            COUNT
        };

        inline std::string toString(RenderMode::mode mode) {
            switch (mode) {
                case GAME:
                    return "RenderMode::GAME";
                    break;
                case DEVELOPMENT:
                    return "RenderMode::DEVELOPMENT";
                    break;
                case COUNT:
                    return "RenderMode::COUNT";
                    break;
            }

            return "RenderMode::Unknown";
        }

    }

#pragma mark - render_state

    /**
     @struct render_state
     This is passed to objects which are rendering, and is intended to be a
     repository for per-frame information needed to draw the scene.
     */
    struct render_state {

        BaseViewportRef viewport;
        RenderMode::mode mode;
        size_t frame, pass;
        seconds_t time, deltaT;

        /**
         create a render_state
         @param vp The current viewport
         @param m The current RenderMode::mode
         @param f The current frame
         @param p The current pass
         @param t The current time

         Note that pass is optional. It's reserved for special situations where a composite Object
         draws multiple child game objects, and might need to render them multiple times, with them
         rendering effects after solid geometry, etc. Pass will generally be zero.
         */
        render_state(RenderMode::mode m, size_t f, size_t p, seconds_t t, seconds_t dt) :
                mode(m),
                frame(f),
                pass(p),
                time(t),
                deltaT(dt) {
        }

    };

}

inline ostream &operator<<(ostream &os, core::RenderMode::mode mode) {
    return os << core::RenderMode::toString(mode);
}

#endif /* RenderState_h */
