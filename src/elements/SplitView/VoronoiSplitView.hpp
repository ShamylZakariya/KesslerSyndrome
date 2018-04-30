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
        void setShadowWidth(double width) { _shadowWidth = saturate<float>(static_cast<float>(width)); }
        double getShadowWidth() const { return _shadowWidth; }
        
    private:
        
        core::BaseViewportRef _viewportA, _viewportB;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;
        vec2 _side;
        ColorA _shadowColor;
        float _shadowWidth;
        
    };
    
    
    class VoronoiSplitViewComposer : public core::ViewportComposer {
    public:
        
        VoronoiSplitViewComposer(const core::TrackableRef &firstTarget, const core::TrackableRef &secondTarget,
                                 const core::ViewportRef &firstViewport, const core::ViewportRef &secondViewport,
                                 const core::TrackerRef &firstTracker = nullptr, const core::TrackerRef &secondTracker = nullptr);
        
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
        shared_ptr<VoronoiSplitViewCompositor> _vspc;
        double _scale;
        
    };
    
} // end namespace vsv

#endif /* VoronoiSplitView_hpp */
