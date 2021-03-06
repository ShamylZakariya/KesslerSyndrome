//
//  ChipmunkHelpers.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef ChipmunkHelpers_hpp
#define ChipmunkHelpers_hpp

#include <cinder/app/App.h>
#include <chipmunk/chipmunk.h>

#include "core/Common.hpp"
#include "core/MathHelpers.hpp"

inline cpVect cpv(const dvec2 &v) {
    return ::cpv(v.x, v.y);
}

inline dvec2 v2(const cpVect &v) {
    return dvec2(v.x, v.y);
}

#pragma mark -
#pragma mark Chipmunk Helper Functions

// a default invalid cpBB ready to be passed to cpBBExpand() calls
static const cpBB cpBBInvalid = {
        +std::numeric_limits<cpFloat>::max(),
        +std::numeric_limits<cpFloat>::max(),
        -std::numeric_limits<cpFloat>::max(),
        -std::numeric_limits<cpFloat>::max()
};

static const cpBB cpBBZero = {
        0.0, 0.0, 0.0, 0.0
};

/**
	create an aabb which fits a circle given position and radius.
 */

template<class V>
inline void cpBBNewCircle(cpBB &bb, const V &position, cpFloat radius) {
    bb.l = position.x - radius;
    bb.b = position.y - radius;
    bb.r = position.x + radius;
    bb.t = position.y + radius;
}

/**
	create an aabb which contains a line segment
 */

template<class V>
inline cpBB cpBBNewLineSegment(const V &a, const V &b) {
    return cpBBNew(cpfmin(a.x, b.x), cpfmin(a.y, b.y), cpfmax(a.x, b.x), cpfmax(a.y, b.y));
}

/**
	create an aabb which contains a line segment with an outset
 */

template<class V>
inline cpBB cpBBNewLineSegment(const V &a, const V &b, cpFloat outset) {
    return cpBBNew(cpfmin(a.x, b.x) - outset, cpfmin(a.y, b.y) - outset, cpfmax(a.x, b.x) + outset, cpfmax(a.y, b.y) + outset);
}

template<class V>
inline cpBB cpBBNewTriangle(const V &a, const V &b, const V &c, cpFloat outset) {
    return cpBBNew(cpfmin(a.x, cpfmin(b.x, c.x)) - outset, cpfmin(a.y, cpfmin(b.y, c.y)) - outset, cpfmax(a.x, cpfmax(b.x, c.x)) + outset, cpfmax(a.y, cpfmax(b.y, c.y)) + outset);
}

/**
	create an 'infinitely' large aabb which will pass all viewport intersection tests
 */
inline void cpBBInfinite(cpBB &bb) {
    bb.l = -std::numeric_limits<cpFloat>::max();
    bb.b = -std::numeric_limits<cpFloat>::max();
    bb.r = +std::numeric_limits<cpFloat>::max();
    bb.t = +std::numeric_limits<cpFloat>::max();
}

static const cpBB cpBBInfinity = {
        -std::numeric_limits<cpFloat>::max(),
        -std::numeric_limits<cpFloat>::max(),
        +std::numeric_limits<cpFloat>::max(),
        +std::numeric_limits<cpFloat>::max()
};

inline bool cpBBIsValid(cpBB bb) {
    return bb.r > bb.l && bb.t > bb.b;
}

template<class T, glm::precision P>
cpBB cpBBExpand(const cpBB &bb, const glm::tvec2<T, P> &v) {
    return cpBBNew(cpfmin(bb.l, v.x),
            cpfmin(bb.b, v.y),
            cpfmax(bb.r, v.x),
            cpfmax(bb.t, v.y));
}


template<class V>
cpBB cpBBExpand(const cpBB &bb, const V &p, cpFloat radius) {
    return cpBBNew(cpfmin(bb.l, p.x - radius),
            cpfmin(bb.b, p.y - radius),
            cpfmax(bb.r, p.x + radius),
            cpfmax(bb.t, p.y + radius));
}

inline cpBB cpBBExpand(const cpBB &bb, const cpBB &bb2) {
    return cpBBNew(cpfmin(bb.l, bb2.l),
            cpfmin(bb.b, bb2.b),
            cpfmax(bb.r, bb2.r),
            cpfmax(bb.t, bb2.t));
}

inline cpBB cpBBExpand(const cpBB &bb, cpFloat amt) {
    return cpBBNew(bb.l - amt, bb.b - amt, bb.r + amt, bb.t + amt);
}

/**
 Return true iff the two BBs, if expanded by `f` intersect. 
 */
static inline cpBool cpBBIntersects(cpBB a, cpBB b, cpFloat f) {
    return ((a.l - f) <= (b.r + f) && (b.l - f) <= (a.r + f) && (a.b - f) <= (b.t + f) && (b.b - f) <= (a.t + f));
}


/**
	If two bounding boxes intersect, returns true and writes their overlapping regions into @a intersection.
	Otherwise returns false.
 */
bool cpBBIntersection(const cpBB &bb1, const cpBB &bb2, cpBB &intersection);

inline cpTransform cpTransformNew(const dmat4 &M) {
    dvec4 c0 = M[0];
    dvec4 c1 = M[1];
    dvec4 c3 = M[3];
    cpTransform t = {c0.x, c0.y, c1.x, c1.y, c3.x, c3.y};
    return t;
}

/**
	create a new cpBB containing the provided cpBB after it has had the transform applied to it
 */
inline cpBB cpBBTransform(const cpBB &bb, const dmat4 transform) {
    return cpTransformbBB(cpTransformNew(transform), bb);
}

inline cpFloat cpBBRadius(const cpBB &bb) {
    return (std::max(bb.r - bb.l, bb.t - bb.b) * 0.5) * 1.414213562;
}

inline cpFloat cpBBWidth(const cpBB &bb) {
    return bb.r - bb.l;
}

inline cpFloat cpBBHeight(const cpBB &bb) {
    return bb.t - bb.b;
}

inline cpVect cpBBSize(const cpBB &bb) {
    return cpv(bb.r - bb.l, bb.t - bb.b);
}

template<class V>
bool cpBBContains(const cpBB &bb, const V &v) {
    return (bb.l < v.x && bb.r > v.x && bb.b < v.y && bb.t > v.y);
}

template<class V, class T>
bool cpBBContainsCircle(const cpBB &bb, const V &v, T radius) {
    return ((bb.l < (v.x - radius)) && (bb.r > (v.x + radius)) && (bb.b < (v.y - radius)) && (bb.t > (v.y + radius)));
}


inline bool operator==(const cpBB &a, const cpBB &b) {
    return a.l == b.l && a.b == b.b && a.r == b.r && a.t == b.t;
}

inline std::ostream &operator<<(std::ostream &os, const cpBB &bb) {
    return os << "[left: " << bb.l << " top: " << bb.t << " right: " << bb.r << " bottom: " << bb.b << "]";
}

#pragma mark - containers

typedef std::set<cpShape *> cpShapeSet;
typedef std::vector<cpShape *> cpShapeVec;

typedef std::set<cpBody *> cpBodySet;
typedef std::vector<cpBody *> cpBodyVec;

typedef std::set<cpConstraint *> cpConstraintSet;
typedef std::vector<cpConstraint *> cpConstraintVec;

#pragma mark - destroy macros

inline void cpShapeCleanupAndFree(cpShape **shape) {
    if (*shape) {
        if (cpShapeGetSpace(*shape)) {
            cpSpaceRemoveShape(cpShapeGetSpace(*shape), *shape);
        }

        cpShapeSetUserData(*shape, nullptr);
        cpShapeFree(*shape);
        *shape = NULL;
    }
}

inline void cpBodyCleanupAndFree(cpBody **body) {
    if (*body) {
        if (cpBodyGetSpace(*body)) {
            cpSpaceRemoveBody(cpBodyGetSpace(*body), *body);
        }

        cpBodySetUserData(*body, nullptr);
        cpBodyFree(*body);
        *body = NULL;
    }
}

inline void cpConstraintCleanupAndFree(cpConstraint **constraint) {
    if (*constraint) {
        if (cpConstraintGetSpace(*constraint)) {
            cpSpaceRemoveConstraint(cpConstraintGetSpace(*constraint), *constraint);
        }

        cpConstraintSetUserData(*constraint, nullptr);
        cpConstraintFree(*constraint);
        *constraint = NULL;
    }
}

inline void cpCleanupAndFree(cpShape *&shape) {
    cpShapeCleanupAndFree(&shape);
}

inline void cpCleanupAndFree(cpBody *&body) {
    cpBodyCleanupAndFree(&body);
}

inline void cpCleanupAndFree(cpConstraint *&constraint) {
    cpConstraintCleanupAndFree(&constraint);
}

template<typename T>
void cpCleanupAndFree(set<T> &s) {
    for (auto it = ::begin(s), end = ::end(s); it != end; ++it) {
        T obj = *it;
        cpCleanupAndFree(obj);
    }

    s.clear();
}

template<typename T>
void cpCleanupAndFree(vector<T> &s) {
    for (auto it = ::begin(s), end = ::end(s); it != end; ++it) {
        T obj = *it;
        cpCleanupAndFree(obj);
    }

    s.clear();
}

inline bool
cpShapeFilterReject(cpShapeFilter a, cpShapeFilter b) {
    // Reject the collision if:
    return (
            // They are in the same non-zero group.
            (a.group != 0 && a.group == b.group) ||
                    // One of the category/mask combinations fails.
                            (a.categories & b.mask) == 0 ||
                    (b.categories & a.mask) == 0
    );
}

//
// extend std with default_delete for chipmunk types - this is for unique_ptr usage
//

namespace std {
    template<>
    class default_delete<cpShape> {
    public:
        void operator()(cpShape *shape) {
            if (shape) {
                if (cpShapeGetSpace(shape)) {
                    cpSpaceRemoveShape(cpShapeGetSpace(shape), shape);
                }

                cpShapeSetUserData(shape, nullptr);
                cpShapeFree(shape);
            }
        }
    };

    template<>
    class default_delete<cpBody> {
    public:
        void operator()(cpBody *body) {
            if (body) {
                if (cpBodyGetSpace(body)) {
                    cpSpaceRemoveBody(cpBodyGetSpace(body), body);
                }

                cpBodySetUserData(body, nullptr);
                cpBodyFree(body);
            }
        }
    };

    template<>
    class default_delete<cpConstraint> {
    public:
        void operator()(cpConstraint *constraint) {
            if (constraint) {
                if (cpConstraintGetSpace(constraint)) {
                    cpSpaceRemoveConstraint(cpConstraintGetSpace(constraint), constraint);
                }

                cpConstraintSetUserData(constraint, nullptr);
                cpConstraintFree(constraint);
            }
        }
    };
}


typedef std::unique_ptr<cpShape> cpShapeUniquePtr;
typedef std::unique_ptr<cpBody> cpBodyUniquePtr;
typedef std::unique_ptr<cpConstraint> cpConstraintUniquePtr;

#endif /* ChipmunkHelpers_hpp */
