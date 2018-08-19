#pragma once

//
//  Spline.h
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include <vector>
#include "core/Common.hpp"
#include "core/MathHelpers.hpp"

namespace core { namespace util { namespace spline {
    
    /**
     draw a spline from b to c
     a is the vertex before b, d is the vertex after c. They're used to create control points. They can be NULL, signifying begin or end of path.
     */
    template<class T, glm::precision P>
    void spline_segment(const glm::tvec2<T, P> *a, const glm::tvec2<T, P> &b, const glm::tvec2<T, P> &c, const glm::tvec2<T, P> *d, T tension, size_t count, std::vector< glm::tvec2<T, P> > &result )
    {
        T u = 0,
            inc = static_cast<T>(1) / static_cast<T>(count);
        
        for ( size_t i = 0; i < count; i++, u += inc ) {
            const T
                uSquared = u*u,
                uCubed = uSquared*u,
                c0 = tension * (-uCubed + static_cast<T>(2) * uSquared - u),
                c1 = (static_cast<T>(2) - tension) * uCubed + (tension - static_cast<T>(3)) * uSquared + static_cast<T>(1),
                c2 = (tension - static_cast<T>(2)) * uCubed + (static_cast<T>(3) - static_cast<T>(2) * tension) * uSquared + tension * u,
                c3 = tension * (uCubed - uSquared);
            
            glm::tvec2<T, P>
                vOne( b ),
                vTwo( c ),
                vBegin( a ? *a : (vOne * static_cast<T>(2) - vTwo)),
                vEnd( d ? *d : (vTwo * static_cast<T>(2) - vOne));
            
            result.push_back((vBegin*c0) + (vOne* c1) + (vTwo * c2) + (vEnd * c3));
        }
    }
    
    /**
     generate the spline of the path described by @a vertices with @a tension and a target count of @a count into @a result.
     if @a closed the path is assumed to be closed.
     */
    template<class T, glm::precision P>
    void spline( const std::vector< glm::tvec2<T, P> > &vertices, T tension, size_t count, bool closed, std::vector< glm::tvec2<T, P> > &result ) {
        typedef std::vector< glm::tvec2<T, P> > Vec2Vec;
        typedef typename std::vector< glm::tvec2<T, P> >::const_iterator Vec2VecConstIterator;
        typedef typename std::vector< glm::tvec2<T, P> >::iterator Vec2VecIterator;
        
        result.clear();
        
        if ( vertices.size() < 2 ) return;
        size_t segCount = count / ( vertices.size() - 1 );
        
        if ( closed ) {
            glm::tvec2<T, P>
                *aPtr = closed ? const_cast< glm::tvec2<T, P>* >(&vertices.back()) : NULL,
                *dPtr = NULL;
            
            for( Vec2VecConstIterator
                b(vertices.begin()),
                c(vertices.begin()+1),
                d(vertices.begin()+2),
                end(vertices.end());
                b != end;
                ++b,++c,++d )
            {
                if ( c == end ) {
                    c = vertices.begin();
                    d = vertices.begin()+1;
                }
                
                dPtr = const_cast< glm::tvec2<T, P>* >(&(*d));
                
                spline_segment( aPtr, *b, *c, dPtr, tension, segCount, result);
                aPtr = const_cast< glm::tvec2<T, P>* >(&(*b));
            }
            
            result.push_back(vertices.front());
        } else {
            glm::tvec2<T, P>
                *aPtr = NULL,
                *dPtr = NULL;
            
            for( Vec2VecConstIterator
                b(vertices.begin()),
                c(vertices.begin()+1),
                d(vertices.begin()+2),
                end(vertices.end());
                c != end;
                ++b,++c,++d )
            {
                dPtr = d != end ? const_cast< glm::tvec2<T, P>* >(&(*d)) : NULL;
                
                spline_segment( aPtr, *b, *c, dPtr, tension, segCount, result);
                aPtr = const_cast< glm::tvec2<T, P>* >(&(*b));
            }
            
            result.push_back(vertices.back());
        }
    }
    
}}} // end namespace core::util::spline
