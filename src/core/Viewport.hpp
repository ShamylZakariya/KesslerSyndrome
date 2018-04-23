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
#include <cinder/gl/Fbo.h>

#include "Common.hpp"
#include "ChipmunkHelpers.hpp"
#include "MathHelpers.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

using namespace ci;
using namespace std;


namespace core {

    SMART_PTR(BaseViewport);
    SMART_PTR(Viewport);
    SMART_PTR(ScreenViewport);
    
    // prototype - defined in ViewportController.hpp
    SMART_PTR(ViewportController);

    class BaseViewport {
    public:
        
        BaseViewport();

        virtual ~BaseViewport();
        
        virtual void setSize(int width, int height) = 0;

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

        virtual void setFboFormat(const gl::Fbo::Format &format) { _fboFormat = format; }
        const gl::Fbo::Format &getFboFormat() const { return _fboFormat; }
        
        // if this Viewport should render into an Fbo instead of directly to screen,
        // the Viewport should return an Fbo instance here.
        virtual gl::FboRef getFbo() const { return nullptr; }
        
        /**
         apply the camera for rendering. sets gl::viewport, projection and view matrices
         */
        virtual void set() = 0;

    private:

        gl::Fbo::Format _fboFormat;

    };

    class Viewport : public BaseViewport {
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

        void setSize(int width, int height) override;

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
         By default, calling setLook centers the given world position directly at the center of the viewport.
         You can bias this away from the center by setting the lookCenterOffset in pixel coordinates.
         A default value of (0,0) centers look targets in the viewport. A value of (-100, -200) would
         cause the look target to be positioned 100px left of center.x, 200px up from center.y
         */
        void setLookCenterOffset(dvec2 offset);
        
        dvec2 getLookCenterOffset() const { return _lookCenterOffset; }
        
        /**
         Reset the look center offset such that look targets are positioned directly at the
         center of the viewport.
         */
        void resetLookCenterOffset() { setLookCenterOffset(dvec2(0,0)); }

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
         get the backing Fbo
        */
        gl::FboRef getFbo() const override {
            return _fbo;
        }

        /**
         apply the camera for rendering. sets gl::viewport, projection and view matrices
         */
        void set() override;

    private:
        
        friend class ViewportController;

        void _updateMatrices();

    private:

        int _width, _height;
        dvec2 _lookCenterOffset;
        look _look;
        dmat4 _viewMatrix, _inverseViewMatrix, _projectionMatrix, _inverseProjectionMatrix, _viewProjectionMatrix, _inverseViewProjectionMatrix;
        gl::FboRef _fbo;
        
    };
    
    /**
     ScreenViewport
     A special viewport for rendering UI, with transforms configured for 1 unit -> 1px
     */
    class ScreenViewport : public BaseViewport {
    public:
        ScreenViewport();
        
        ~ScreenViewport();
        
        void setSize(int width, int height) override;
        
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
        
        double getScale() const override {
            return 1.0;
        };
        
        double getReciprocalScale() const override {
            return 1.0;
        };
        
        dmat4 getViewMatrix() const override {
            return mat4();
        };
        
        dmat4 getInverseViewMatrix() const override {
            return mat4();
        };
        
        dmat4 getProjectionMatrix() const override {
            return mat4();
        };
        
        dmat4 getInverseProjectionMatrix() const override {
            return mat4();
        };
        
        dmat4 getViewProjectionMatrix() const override {
            return mat4();
        };
        
        dmat4 getInverseViewProjectionMatrix() const override {
            return mat4();
        };
        
        cpBB getFrustum() const override {
            return cpBBNew(0, 0, _width, _height);
        };
        
        gl::FboRef getFbo() const override {
            return _fbo;
        }

        // this camera is an identity matrix so we don't have anything to do here
        void set() override {}
        
    private:
        
        int _width, _height;
        gl::FboRef _fbo;
        
    };



} // namespace core


#endif /* Viewport_hpp */
