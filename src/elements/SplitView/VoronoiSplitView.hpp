//
//  VoronoiSplitView.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/30/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef VoronoiSplitView_hpp
#define VoronoiSplitView_hpp

#include "Core.hpp"
#include "Tracking.hpp"

#include <cinder/gl/scoped.h>

namespace vsv {
    
    SMART_PTR(VoronoiSplitViewCompositor);
    SMART_PTR(VoronoiSplitViewComposer);

    /**
     Implements the compositor for VoronoiSplitViewComposer.
     You will not interact with this directly, it's created by VoronoiSplitViewComposer.
     */
    class VoronoiSplitViewCompositor : public core::BaseCompositor {
    public:
        
        VoronoiSplitViewCompositor(const core::BaseViewportRef &viewportA, const core::BaseViewportRef &viewportB);
        
        void composite(int width, int height) override;
        
        void setSplitSideDir(dvec2 dir) {
            _side = dir;
        }
        
        dvec2 getSplitSideDir() const { return _side; }
        
        void setShadowColor(ColorA color) { _shadowColor = color; }
        ColorA getShadowColor() const { return _shadowColor; }
        
        /**
         ShadowWidth is expressed in terms of [0,1] where 0 means no width, 1 means shadow extends all the way across the split.
         */
        void setShadowWidth(double width) { _shadowWidth = saturate<float>(width); }
        double getShadowWidth() const { return _shadowWidth; }
        
        void setShadowIntensity(double intensity) { _shadowIntensity = saturate<float>(intensity); }
        double getShadowIntensity() const { return _shadowIntensity; }
        
    private:
        
        core::BaseViewportRef _viewportA, _viewportB;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;
        vec2 _side;
        ColorA _shadowColor;
        float _shadowWidth, _shadowIntensity;
        
    };
    
    /**
     Implements a simplified, 2-player "voronoi" split view, per:
     https://www.gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing
     This is not an arbitrary N-player split, just two.
     
     Usage is simple - VoronoiSplitViewComposer needs two "Trackable" things (e.g. player state) and two viewports.
     Construct it, and set it as the Scenario's ViewportComposer.
     
     TrackableRef trackableA = playerA->getComponent<CharacterState>();
     TrackableRef trackableB = playerB->getComponent<CharacterState>();
     auto svc = make_shared<vsv::VoronoiSplitViewComposer>(trackableA, trackableB, viewportA, viewportB);
     scenario->setViewportComposer(svc);

     */
    class VoronoiSplitViewComposer : public core::ViewportComposer {
    public:
        
        VoronoiSplitViewComposer(const core::TrackableRef &firstTarget, const core::TrackableRef &secondTarget,
                                 const core::ViewportRef &firstViewport, const core::ViewportRef &secondViewport,
                                 const core::TrackerRef &firstTracker = nullptr, const core::TrackerRef &secondTracker = nullptr);
        
        // get access to the VoronoiSplitViewCompositor; you can use it to set shadow width, color, etc.
        const VoronoiSplitViewCompositorRef getVoronoiSplitViewCompositor() const { return _vspc; }
        
        void setFirstTarget(const core::TrackableRef &target) { _firstTarget = target; }
        const core::TrackableRef &getFirstTarget() const { return _firstTarget; }
        
        void setSecondTarget(const core::TrackableRef &target) { _secondTarget = target; }
        const core::TrackableRef &getSecondTarget() const { return _secondTarget; }
        
        const core::ViewportRef &getFirstViewport() const { return _firstViewport; }
        const core::ViewportRef &getSecondViewport() const { return _secondViewport; }
        
        void setFirstTracker(const core::TrackerRef &tracker) { _firstTracker = tracker; }
        const core::TrackerRef &getFirstTracker() const { return _firstTracker; }
        
        void setSecondTracker(const core::TrackerRef &tracker) { _secondTracker = tracker; }
        const core::TrackerRef &getSecondTracker() const { return _secondTracker; }
        
        void setScale(double scale) {
            _scale = max(scale, 0.0);
        }
        
        double getScale() const { return _scale; }
        
        void onScenarioResized(int width, int height) override;
        
        void update(const core::time_state &time) override;
        
    protected:
        
        int _width, _height;
        core::TrackableRef _firstTarget, _secondTarget;
        core::TrackerRef _firstTracker, _secondTracker;
        core::ViewportRef _firstViewport, _secondViewport;
        VoronoiSplitViewCompositorRef _vspc;
        double _scale;
        
    };
    
} // end namespace vsv

#endif /* VoronoiSplitView_hpp */
