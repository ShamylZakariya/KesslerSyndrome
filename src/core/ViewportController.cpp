//
//  ViewportController.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#include "ViewportController.hpp"

namespace core {

    /*
     ViewportRef _viewport;
     bool _disregardViewportMotion;
     
     trauma_config _traumaConfig;
     double _traumaLevel, _traumaBaselineLevel;
     vector<ci::Perlin> _traumaPerlinNoiseGenerators;
     */
    
    ViewportController::ViewportController(ViewportRef viewport) :
            _disregardViewportMotion(false),
            _traumaLevel(0),
            _traumaBaselineLevel(0)
    {
        _setViewport(viewport);
        _setup();
    }

    ViewportController::ViewportController(ViewportRef viewport, const tracking_config &tracking, const trauma_config &trauma) :
    Tracker(tracking),
    _viewport(viewport),
    _disregardViewportMotion(false),
    _traumaConfig(trauma),
    _traumaLevel(0),
    _traumaBaselineLevel(0)
    {
        _setViewport(viewport);
        _setup();
    }

    void ViewportController::update(const time_state &time) {
        Component::update(time);

        _disregardViewportMotion = true;
        
        // perform tracking
        Viewport::look look = Tracker::apply(_viewport->getLook(), time);
                
        // apply trauma shake
        const double shake = getCurrentTraumaShake();
        if (shake > 0) {
            double t = time.time * _traumaConfig.shakeFrequency;
            double scale = 1 / look.scale;
            const double dx = _traumaPerlinNoiseGenerators[0].noise(t) * shake * _traumaConfig.shakeTranslation.x * scale;
            const double dy = _traumaPerlinNoiseGenerators[1].noise(t) * shake * _traumaConfig.shakeTranslation.y * scale;
            const double dr = _traumaPerlinNoiseGenerators[2].noise(t) * shake * _traumaConfig.shakeRotation * scale;
            look.world.x += dx;
            look.world.y += dy;
            look.up = rotate(look.up, dr);
        }

        _viewport->setLook(look);

        _disregardViewportMotion = false;
        
        // now decay trauma and trauma baseline
        _traumaLevel = saturate(_traumaLevel - _traumaConfig.traumaDecayRate * time.deltaT);
        _traumaBaselineLevel = saturate(_traumaBaselineLevel - _traumaConfig.traumaBaselineDecayRate * time.deltaT);
    }
    
    void ViewportController::scaleAboutScreenPoint(double scale, dvec2 aboutScreenPoint) {
        _disregardViewportMotion = true;
        
        // determine where aboutScreenPoint is in worldSpace right now
        dvec2 aboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);

        // assign new scale
        auto look = _viewport->getLook();
        look.scale = scale;
        _viewport->setLook(look);

        // determine where that point will be, post scale
        dvec2 postScaleAboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);
        dvec2 delta = postScaleAboutScreenPointWorld - aboutScreenPointWorld;
        
        look.world -= delta;
        _viewport->setLook(look);

        // now sync our target to our viewport
        _target = look;
        _disregardViewportMotion = false;
    }
    
    void ViewportController::addTrauma(double trauma) {
        _traumaLevel = saturate(_traumaLevel + trauma);
    }
    
    void ViewportController::setTraumaBaseline(double tb) {
        _traumaBaselineLevel = saturate(tb);
    }


#pragma mark - Private
    
    void ViewportController::_setup() {
        // uniquely-seeded perlin noise for shake x, shake y, and shake rotation
        for (int i = 0; i < 3; i++) {
            _traumaPerlinNoiseGenerators.emplace_back(ci::Perlin(4,i));
        }
    }
    
    void ViewportController::_setViewport(ViewportRef vp) {
        if (_viewport) {
            _viewport->onMotion.disconnect(this);
            _viewport->onBoundsChanged.disconnect(this);
        }
        _viewport = vp;
        if (_viewport) {
            _target = _viewport->getLook();
            _viewport->onMotion.connect(this, &ViewportController::_onViewportPropertyChanged);
            _viewport->onBoundsChanged.connect(this, &ViewportController::_onViewportPropertyChanged);
        } else {
            _target = { dvec2(0, 0), dvec2(0, 1), 1 };
        }
    }

    void ViewportController::_onViewportPropertyChanged(const Viewport &vp) {
        if (!_disregardViewportMotion) {
            _target = vp.getLook();
        }
    }


} // end namespace core
