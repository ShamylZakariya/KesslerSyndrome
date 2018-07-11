//
//  VoronoiSplitView.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/30/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include <cinder/gl/scoped.h>

#include "elements/SplitView/VoronoiSplitView.hpp"
#include "core/util/GlslProgLoader.hpp"
#include "core/util/Easing.hpp"

using namespace core;
namespace elements {
    
    /*
     core::BaseViewportRef _viewportA, _viewportB;
     gl::GlslProgRef _shader;
     gl::BatchRef _batch;
     vec2 _side;
     ColorA _shadowColor;
     float _shadowWidth, _shadowIntensity;
     */
    
    
    VoronoiSplitViewCompositor::VoronoiSplitViewCompositor(const BaseViewportRef &viewportA, const BaseViewportRef &viewportB):
    _viewportA(viewportA),
    _viewportB(viewportB),
    _shader(util::loadGlslAsset("core/elements/shaders/voronoi_split_view_compositor.glsl")),
    _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader)),
    _side(0,1),
    _shadowColor(ColorA(0.1,0.1,0.1,1)),
    _shadowWidth(0.5),
    _shadowIntensity(1)
    {
    }
    
    void VoronoiSplitViewCompositor::composite(int width, int height) {
        gl::ScopedViewport sv(0,0,width,height);
        
        gl::ScopedMatrices sm;
        gl::setMatricesWindow( width, height, true );
        gl::scale(width, height, 1);
        
        gl::ScopedBlendAlpha blender;
        
        _viewportA->getFbo()->getColorTexture()->bind(0);
        _viewportB->getFbo()->getColorTexture()->bind(1);
        _shader->uniform("ColorTex0", 0);
        _shader->uniform("ColorTex1", 1);
        _shader->uniform("Tint0", ColorA(1,1,1,1));
        _shader->uniform("Tint1", ColorA(1,1,1,1));
        _shader->uniform("ShadowWidth", _shadowWidth * _shadowIntensity);
        _shader->uniform("ShadowColor", ColorA(_shadowColor, _shadowColor.a * _shadowIntensity));
        _shader->uniform("Side", _side);
        _batch->draw();
    }
    
    
    /*
     int _width, _height;
     core::TrackableRef _firstTarget, _secondTarget;
     core::TrackerRef _firstTracker, _secondTracker;
     core::ViewportRef _firstViewport, _secondViewport;
     shared_ptr<VoronoiSplitViewCompositor> _vspc;
     double _scale;
     */
    
    
    VoronoiSplitViewComposer::VoronoiSplitViewComposer(const TrackableRef &firstTarget, const TrackableRef &secondTarget,
                                                       const ViewportRef &firstViewport, const ViewportRef &secondViewport,
                                                       const TrackerRef &firstTracker, const TrackerRef &secondTracker):
    _width(0),
    _height(0),
    _firstTarget(firstTarget),
    _secondTarget(secondTarget),
    _firstTracker(firstTracker ? firstTracker : make_shared<Tracker>(Tracker::tracking_config(0.975))),
    _secondTracker(secondTracker ? secondTracker : make_shared<Tracker>(Tracker::tracking_config(0.975))),
    _firstViewport(firstViewport),
    _secondViewport(secondViewport),
    _scale(1)
    {
        _viewports = { _firstViewport, _secondViewport };
        _compositor = _vspc = make_shared<VoronoiSplitViewCompositor>(_viewports[0], _viewports[1]);
        _vspc->setShadowWidth(0.5);
    }
    
    void VoronoiSplitViewComposer::onScenarioResized(int width, int height) {
        _width = width;
        _height = height;
        
        _firstViewport->setSize(width, height);
        _secondViewport->setSize(width, height);
    }
    
    void VoronoiSplitViewComposer::update(const time_state &time) {
        ViewportComposer::update(time);
        
        
        // first compute the split, in world space
        const dvec2 screenCenter(_width/2, _height/2);
        const dvec2 firstTargetPositionWorld = _firstTarget->getTrackingPosition();
        const dvec2 secondTargetPositionWorld = _secondTarget->getTrackingPosition();
        const double worldDistance = length(secondTargetPositionWorld-firstTargetPositionWorld);
        const dvec2 dir = (secondTargetPositionWorld-firstTargetPositionWorld) / worldDistance;
                
        // compute the voronoi mix component. when:
        // voronoiMix == 0, then both characters can be rendered on screen without a split
        // voronoiMix == 1, we have full separation
        const double screenDistance = worldDistance * _scale;
        const double maxEllipticalDistScreen = length(dvec2(dir.x * _width/2, dir.y * _height/2));
        const double minEllipticalDistScreen = maxEllipticalDistScreen * 0.5;
        const double voronoiMix = saturate(((screenDistance / 2)  - minEllipticalDistScreen) / (maxEllipticalDistScreen - minEllipticalDistScreen));
        const double curvedVoronoiMix = util::easing::quad_ease_in_out(voronoiMix);
        
        // compute the tracking targets. when:
        // voronoiMix == 0, viewports both track the midpoint of the two targets
        // voronoiMix == 1, viewports track the targets directly
        const dvec2 targetMidpointWorld = (firstTargetPositionWorld + secondTargetPositionWorld) * 0.5;
        const dvec2 firstLookWorld = lrp(curvedVoronoiMix, targetMidpointWorld, firstTargetPositionWorld);
        const dvec2 secondLookWorld = lrp(curvedVoronoiMix, targetMidpointWorld, secondTargetPositionWorld);
        
        // run the tracking targets through our tracker for smoothing
        _firstTracker->setTarget(Viewport::look(firstLookWorld, dvec2(0,1), _scale));
        _secondTracker->setTarget(Viewport::look(secondLookWorld, dvec2(0,1), _scale));
        auto firstLook = _firstTracker->apply(_firstViewport->getLook(), time);
        auto secondLook = _secondTracker->apply(_secondViewport->getLook(), time);
        _firstViewport->setLook(firstLook);
        _secondViewport->setLook(secondLook);
        
        
        // compute the look center offset. when:
        // voronoiMix == 0, look center offset is zero, making viewports look directly at the look.world position, which will be the midpoint of the two targets
        // voronoiMix == 1, track a position offset perpendicularly from the split
        const double fudge = 1.1;
        const dvec2 centerOffset = -dir * maxEllipticalDistScreen * 0.5 * fudge;
        _firstViewport->setLookCenterOffset(centerOffset * curvedVoronoiMix);
        _secondViewport->setLookCenterOffset(-centerOffset * curvedVoronoiMix);
        
        // update the shader
        // shadow effect fased in as voronoi mix goes to 1
        _vspc->setSplitSideDir(dir);
        _vspc->setShadowIntensity(util::easing::quad_ease_out(voronoiMix));
    }
    
} // end namespace elements
