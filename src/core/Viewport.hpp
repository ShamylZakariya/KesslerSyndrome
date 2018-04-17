//
//  Viewport.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#ifndef Viewport_hpp
#define Viewport_hpp

#include <cinder/Area.h>
#include <chipmunk/chipmunk.h>
#include <cinder/gl/scoped.h>

#include "Common.hpp"
#include "ChipmunkHelpers.hpp"
#include "MathHelpers.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

using namespace ci;
using namespace std;


namespace core {

    SMART_PTR(IViewport);
    SMART_PTR(Viewport);
    SMART_PTR(ScreenViewport);

    class IViewport {
    public:

        virtual ~IViewport() {
        }

        virtual ivec2 getSize() const = 0;

        virtual int getWidth() const = 0;

        virtual int getHeight() const = 0;

        virtual Rectd getBounds() const {
            return Rectd(0, 0, getWidth(), getHeight());
        }

        virtual dvec2 getCenter() const = 0;

        virtual double getScale() const = 0;

        virtual double getReciprocalScale() const {
            return 1.0 / getScale();
        };

        virtual dmat4 getViewMatrix() const = 0;

        virtual dmat4 getInverseViewMatrix() const = 0;

        virtual dmat4 getProjectionMatrix() const = 0;

        virtual dmat4 getInverseProjectionMatrix() const = 0;

        virtual dmat4 getViewProjectionMatrix() const = 0;

        virtual dmat4 getInverseViewProjectionMatrix() const = 0;

        /**
         get the current viewport frustum in world coordinates
         */
        virtual cpBB getFrustum() const = 0;


        /**
         convert a point in world coordinate system to screen
         */
        virtual dvec2 worldToScreen(const dvec2 &world) const {
            return getViewProjectionMatrix() * world;
        }

        /**
         convert a direction in world coordinate system to screen
         */
        virtual dvec2 worldToScreenDir(const dvec2 &world) const {
            return dvec2(getViewProjectionMatrix() * dvec4(world.x, world.y, 0.0, 0.0));
        }

        /**
         convert a point in screen coordinate system to world.
         */
        virtual dvec2 screenToWorld(const dvec2 &screen) const {
            return getInverseViewProjectionMatrix() * screen;
        }

        /**
         convert a direction in screen coordinate system to world.
         */
        virtual dvec2 screenToWorldDir(const dvec2 &screen) const {
            return dvec2(getInverseViewProjectionMatrix() * dvec4(screen.x, screen.y, 0.0, 0.0));
        }


    };

    class Viewport : public IViewport {
    public:

        signals::signal<void(const Viewport &)> onMotion;
        signals::signal<void(const Viewport &)> onBoundsChanged;

        struct look {
            dvec2 world;
            dvec2 up;
            double scale;

            look() :
                    world(0, 0),
                    up(0, 1),
                    scale(1) {
            }

            look(dvec2 w, dvec2 u, double s) :
                    world(w),
                    up(u),
                    scale(s) {
            }

            look(const look &c) :
                    world(c.world),
                    up(c.up),
                    scale(c.scale) {
            }
        };


    public:

        Viewport();

        virtual ~Viewport() {
        }

        /**
         manage the current viewport
         */

        void setSize(int width, int height);

        ivec2 getSize() const override {
            return ivec2(_width, _height);
        }

        int getWidth() const override {
            return _width;
        }

        int getHeight() const override {
            return _height;
        }

        dvec2 getCenter() const override {
            return dvec2(_width / 2.0, _height / 2.0);
        }

        /**
            Set the scale - this effectively zooms in and out of whatever's at the center of the viewport
         */
        void setScale(double z) {
            look l = _look;
            l.scale = z;
            setLook(l);
        }

        /**
            get the current scale
         */
        double getScale() const override {
            return _look.scale;
        }

        /**
            get the reciprocal of current scale
         */
        double getReciprocalScale() const override {
            return 1.0 / _look.scale;
        }

        /**
         center viewport on a position in the world, with a given up-vector
         */
        void setLook(const look &l);

        /**
         center viewport on a position in the world, with a given up-vector and scale
         */
        void setLook(const dvec2 &world, const dvec2 &up, double scale) {
            setLook(look(world, up, scale));
        }

        /**
         center viewport on a position in the world, with a given up-vector, maintaining current scale
         */
        void setLook(const dvec2 &world, const dvec2 &up) {
            setLook(look(world, up, _look.scale));
        }

        /**
         center viewport on a position in the world, maintaining current up and scale
         */
        void setLook(const dvec2 &world) {
            setLook(look(world, _look.up, _look.scale));
        }

        look getLook() const {
            return _look;
        }

        dmat4 getViewMatrix() const override {
            return _viewMatrix;
        }

        dmat4 getInverseViewMatrix() const override {
            return _inverseViewMatrix;
        }

        dmat4 getProjectionMatrix() const override {
            return _projectionMatrix;
        }

        dmat4 getInverseProjectionMatrix() const override {
            return _inverseProjectionMatrix;
        }

        dmat4 getViewProjectionMatrix() const override {
            return _viewProjectionMatrix;
        }

        dmat4 getInverseViewProjectionMatrix() const override {
            return _inverseViewProjectionMatrix;
        }

        /**
         convert a point in world coordinate system to screen
         */
        dvec2 worldToScreen(const dvec2 &world) const override {
            return _viewProjectionMatrix * world;
        }

        /**
         convert a direction in world coordinate system to screen
         */
        dvec2 worldToScreenDir(const dvec2 &world) const override {
            return dvec2(_viewProjectionMatrix * dvec4(world.x, world.y, 0.0, 0.0));
        }

        /**
         convert a point in screen coordinate system to world.
         */
        dvec2 screenToWorld(const dvec2 &screen) const override {
            return _inverseViewProjectionMatrix * screen;
        }

        /**
         convert a direction in screen coordinate system to world.
         */
        dvec2 screenToWorldDir(const dvec2 &screen) const override {
            return dvec2(_inverseViewProjectionMatrix * dvec4(screen.x, screen.y, 0.0, 0.0));
        }

        /**
         get the current viewport frustum in world coordinates
         */
        cpBB getFrustum() const override;

        /**
         apply the camera for rendering. sets gl::viewport, projection and view matrices
         */
        void set();

    private:
        
        friend class ViewportController;

        void _updateMatrices();

    private:

        int _width, _height;
        look _look;
        dmat4 _viewMatrix, _inverseViewMatrix, _projectionMatrix, _inverseProjectionMatrix, _viewProjectionMatrix, _inverseViewProjectionMatrix;

    };
    
    /**
     ScreenViewport
     A special viewport for rendering UI, with transforms configured for 1 unit -> 1px
     */
    class ScreenViewport : public IViewport {
    public:
        ScreenViewport() {
        }
        
        virtual ~ScreenViewport() {
        }
        
        void setSize(int width, int height) {
            _width = width;
            _height = height;
        }
        
        ivec2 getSize() const override {
            return ivec2(_width, _height);
        }
        
        int getWidth() const override {
            return _width;
        }
        
        int getHeight() const override {
            return _height;
        }
        
        dvec2 getCenter() const override {
            return dvec2(_width / 2.0, _height / 2.0);
        }
        
        virtual double getScale() const override {
            return 1.0;
        };
        
        virtual double getReciprocalScale() const override {
            return 1.0;
        };
        
        virtual dmat4 getViewMatrix() const override {
            return mat4();
        };
        
        virtual dmat4 getInverseViewMatrix() const override {
            return mat4();
        };
        
        virtual dmat4 getProjectionMatrix() const override {
            return mat4();
        };
        
        virtual dmat4 getInverseProjectionMatrix() const override {
            return mat4();
        };
        
        virtual dmat4 getViewProjectionMatrix() const override {
            return mat4();
        };
        
        virtual dmat4 getInverseViewProjectionMatrix() const override {
            return mat4();
        };
        
        virtual cpBB getFrustum() const override {
            return cpBBNew(0, 0, _width, _height);
        };
        
    private:
        
        int _width, _height;
        
    };



} // namespace core


#endif /* Viewport_hpp */
