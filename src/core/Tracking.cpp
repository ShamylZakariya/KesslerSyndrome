//
//  Tracking.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/27/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Tracking.hpp"

namespace core {
    
    /*
     TrackableWeakRef _trackable;
     tracking_config _config;
     Viewport::look _target;
     */
    
    Tracker::Tracker(){}

    Tracker::Tracker(const TrackableRef &trackableTarget, const tracking_config &config):
            _trackable(trackableTarget),
            _config(config)
    {}

    Tracker::Tracker(const tracking_config &config):
    _config(config)
    {}

    Tracker::~Tracker()
    {}
    
    void Tracker::setTarget(Viewport::look l) {
        _target.world = l.world;

        double len = glm::length(l.up);
        if (len > 1e-7) {
            _target.up = l.up / len;
        } else {
            _target.up = dvec2(0, 1);
        }

        _target.scale = max(l.scale, 1e-7);
    }
    
    Viewport::look Tracker::apply(Viewport::look look, const time_state &time) {
        TrackableRef trackableTarget = getTrackableTarget();
        if (trackableTarget) {
            _target.world = trackableTarget->getTrackingPosition();
            _target.up = trackableTarget->getTrackingUp();
        }
        
        const double rate = 1.0 / time.deltaT;
        
        const dvec2 lookWorldError = _target.world - look.world;
        if (lookWorldError.x > 0) {
            if (_config.positiveX < 1) {
                const double f = std::pow(_config.positiveX, rate);
                look.world.x += f * lookWorldError.x;
            } else {
                look.world.x = _target.world.x;
            }
        } else {
            if (_config.negativeX < 1) {
                const double f = std::pow(_config.negativeX, rate);
                look.world.x += f * lookWorldError.x;
            } else {
                look.world.x = _target.world.x;
            }
        }
        
        if (lookWorldError.y > 0) {
            if (_config.positiveY < 1) {
                const double f = std::pow(_config.positiveY, rate);
                look.world.y += f * lookWorldError.y;
            } else {
                look.world.y = _target.world.y;
            }
        } else {
            if (_config.negativeY < 1) {
                const double f = std::pow(_config.negativeY, rate);
                look.world.y += f * lookWorldError.y;
            } else {
                look.world.y = _target.world.y;
            }
        }
        
        {
            const dvec2 lookUpError = _target.up - look.up;
            const double f = std::pow(_config.up, rate);
            look.up += f * lookUpError;
            look.up = normalize(look.up);
        }
        
        const double scaleError = _target.scale - look.scale;
        if (scaleError > 0) {
            if (_config.positiveScale < 1) {
                const double f = std::pow(_config.positiveScale, rate);
                look.scale += f * scaleError;
            } else {
                look.scale = _target.scale;
            }
        } else if (scaleError < 0) {
            if (_config.negativeScale < 1) {
                const double f = std::pow(_config.negativeScale, rate);
                look.scale += f * scaleError;
            } else {
                look.scale = _target.scale;
            }
        }
        
        return look;
    }
    
}
