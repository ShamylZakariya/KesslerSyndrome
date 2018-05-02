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

#pragma mark - render_state

    /**
     @struct render_state
     This is passed to objects which are rendering, and is intended to be a
     repository for per-frame information needed to draw the scene.
     */
    struct render_state {

        BaseViewportRef viewport;
        size_t frame;
        seconds_t time;
        seconds_t deltaT;
        size_t gizmoMask;

        /**
         create a render_state
         */
        render_state(size_t frame, seconds_t time, seconds_t deltaTime, size_t gizmoMask) :
                frame(frame),
                time(time),
                deltaT(deltaTime),
                gizmoMask(gizmoMask){
        }
        
        // test to see if all bits in `bits are enabled in gizmoMask
        inline bool testAllGizmoBits(size_t bits) const { return (gizmoMask & bits) == bits; }
        
        // test if any bits in `bits are enabled in gizmoMask
        inline bool testGizmoBit(size_t bits) const { return gizmoMask & bits; }


    };
    
    namespace Gizmos {
        static const size_t ANCHORS         = 1 << 0;
        static const size_t LABELS          = 1 << 1;
        static const size_t AABBS           = 1 << 2;
        static const size_t WIREFRAME       = 1 << 3;
        static const size_t PHYSICS         = 1 << 4;
    }

}

#endif /* RenderState_h */
