//
//  Bezier.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 6/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Bezier_hpp
#define Bezier_hpp

#include "MathHelpers.hpp"

namespace core {
    namespace util {

        /**
         De Casteljau’s algorithm for bezier segment subdivision
         t: [0,1] time
         p0: start point
         p1: first control point
         p2: second control point
         p3: end point
         */
        template<class T, class V> /* T is number type, V is vector type (vec2, vec3f, etc) */
        V bezier(T t, V p0, V p1, V p2, V p3) {
            const V a = lrp(t, p0, p1);
            const V b = lrp(t, p2, p3);
            const V c = lrp(t, a, b);
            return c;
        }
        
    }
}

#include <stdio.h>

#endif /* Bezier_hpp */
