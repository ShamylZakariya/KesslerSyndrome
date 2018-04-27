//
//  Tracking.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/27/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Tracking_hpp
#define Tracking_hpp

#include "Object.hpp"
#include "TimeState.hpp"
#include "Viewport.hpp"

namespace core {

    SMART_PTR(Trackable);
    class Trackable {
    public:
        virtual ~Trackable(){}
        virtual dvec2 getPosition() const = 0;
        virtual dvec2 getUp() const { return dvec2(0,1); };
    };

    
    SMART_PTR(Tracker);
    class Tracker {
    public:
        
        struct tracking_config {
            double positiveY;
            double negativeY;
            double positiveX;
            double negativeX;
            double up;
            double positiveScale;
            double negativeScale;
            
            tracking_config():
            positiveY(0.99),
            negativeY(0.99),
            positiveX(0.99),
            negativeX(0.99),
            up(0.99),
            positiveScale(0.99),
            negativeScale(0.99)
            {}
            
            tracking_config(double panFactor, double upFactor, double scaleFactor) :
            positiveY(panFactor),
            negativeY(panFactor),
            positiveX(panFactor),
            negativeX(panFactor),
            up(upFactor),
            positiveScale(scaleFactor),
            negativeScale(scaleFactor)
            {}
            
        };
        
    public:
        
        Tracker();
        Tracker(const TrackableRef &target, const tracking_config &config);
        Tracker(const tracking_config &config);
        virtual ~Tracker();
        
        void setTrackingConfig(const tracking_config &c) { _config = c; }
        const tracking_config &getTrackingConfig() const { return _config; }
        tracking_config &getTrackingConfig() { return _config; }
        
        void setTrackableTarget(const TrackableRef &trackable) { _trackable = trackable; }
        TrackableRef getTrackableTarget() const { return _trackable.lock(); }
        
        virtual void setTarget(Viewport::look l);
        
        void setTarget(const dvec2 world, const dvec2 up, double scale) {
            setTarget(Viewport::look(world, up, scale));
        }
        
        void setTarget(const dvec2 world, const dvec2 up) {
            setTarget(Viewport::look(world, up, _target.scale));
        }
        
        void setTarget(const dvec2 world) {
            setTarget(Viewport::look(world, _target.up, _target.scale));
        }
        
        void setTargetScale(double scale) {
            setTarget(Viewport::look(_target.world, _target.up, scale));
        }
        
        Viewport::look getTarget() const {
            return _target;
        }
        
        virtual Viewport::look update(Viewport::look look, const time_state &time);
        
    protected:
        
        TrackableWeakRef _trackable;
        tracking_config _config;
        Viewport::look _target;
        
    };

}

#endif /* Tracking_hpp */
