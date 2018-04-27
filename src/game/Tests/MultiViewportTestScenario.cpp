//
//  MultiViewportTestScenario.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/19/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "MultiViewportTestScenario.hpp"

#include "App.hpp"
#include "DevComponents.hpp"
#include "ImageProcessing.hpp"
#include "GlslProgLoader.hpp"
#include "Easing.hpp"
#include "Tracking.hpp"

#include <cinder/gl/scoped.h>

using namespace ci;
using namespace core;

namespace {
    
    class ImageDrawer : public core::DrawComponent {
    public:
        
        ImageDrawer(gl::Texture2dRef image, dvec2 topLeft):
        _image(image),
        _topLeft(topLeft)
        {}
        
        void draw(const render_state &renderState) override {
            gl::ScopedBlendAlpha sba;
            gl::ScopedColor sc(ColorA(1,1,1,1));
            
            const auto bounds = _image->getBounds();
            const auto dest = Rectf(_topLeft.x, _topLeft.y, _topLeft.x + bounds.getWidth(), _topLeft.y - bounds.getHeight());
            gl::draw(_image, bounds, dest);
        }
        
        VisibilityDetermination::style getVisibilityDetermination() const override {
            return VisibilityDetermination::ALWAYS_DRAW;
        }
        
        int getLayer() const override {
            return 1;
        }
        
    private:
        
        ci::gl::Texture2dRef _image;
        dvec2 _topLeft;
        
    };
    
    class CharacterState : public core::Component, public core::Trackable {
    public:
        CharacterState(ColorA color, dvec2 startPosition, double radius, double speed = 4 ):
        _position(startPosition),
        _radius(radius),
        _speed(speed),
        _color(color)
        {}
        
        void setPosition(dvec2 position) { _position = position; notifyMoved(); }
        dvec2 getPosition() const override { return _position; }
        
        void setRadius(double r) { _radius = r; notifyMoved(); }
        double getRadius() const { return _radius; }
        
        void setColor(ColorA color) { _color = color; }
        ColorA getColor() const { return _color; }
        
        void setSpeed(double s) { _speed = s; }
        double getSpeed() const { return _speed; }
        
    private:
        dvec2 _position;
        double _radius, _speed;
        ColorA _color;
    };
    
    class CharacterDrawComponent : public core::DrawComponent {
    public:
        CharacterDrawComponent(){}
        
        void onReady(ObjectRef parent, StageRef stage) override {
            DrawComponent::onReady(parent, stage);
            _state = getSibling<CharacterState>();
        }
        
        cpBB getBB() const override {
            auto state = _state.lock();
            return cpBBNewForCircle(cpv(state->getPosition()), state->getRadius());
        }
        
        void draw(const render_state &renderState) override {
            auto state = _state.lock();
            gl::ScopedBlendAlpha sba;
            gl::ScopedColor sc(state->getColor());
            gl::drawSolidCircle(state->getPosition(), state->getRadius());
        }
        
        VisibilityDetermination::style getVisibilityDetermination() const override {
            return VisibilityDetermination::FRUSTUM_CULLING;
        }
        
        int getLayer() const override {
            return 100;
        }
        
    protected:
        
        weak_ptr<CharacterState> _state;
        
    };
    
    enum Player {
        A,
        B
    };
    
    class CharacterControlComponent : public core::InputComponent {
    public:
        
        CharacterControlComponent(Player player) {
            switch(player) {
                case Player::A:
                    monitorKeys({ app::KeyEvent::KEY_w, app::KeyEvent::KEY_a, app::KeyEvent::KEY_s, app::KeyEvent::KEY_d });
                    break;
                case Player::B:
                    monitorKeys({ app::KeyEvent::KEY_LEFT, app::KeyEvent::KEY_UP, app::KeyEvent::KEY_RIGHT, app::KeyEvent::KEY_DOWN });
                    break;
            }
        }
        
        void onReady(ObjectRef parent, StageRef stage) override {
            InputComponent::onReady(parent, stage);
            _state = getSibling<CharacterState>();
        }
        
        void update(const time_state &time) override {
            Component::update(time);
            auto state = _state.lock();
            
            if (isMonitoredKeyDown(app::KeyEvent::KEY_w) || isMonitoredKeyDown(app::KeyEvent::KEY_UP)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(0,1));
            }
            
            if (isMonitoredKeyDown(app::KeyEvent::KEY_s) || isMonitoredKeyDown(app::KeyEvent::KEY_DOWN)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(0,-1));
            }
            
            if (isMonitoredKeyDown(app::KeyEvent::KEY_a) || isMonitoredKeyDown(app::KeyEvent::KEY_LEFT)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(-1,0));
            }
            
            if (isMonitoredKeyDown(app::KeyEvent::KEY_d) || isMonitoredKeyDown(app::KeyEvent::KEY_RIGHT)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(1,0));
            }
            
        }
        
    private:
        
        weak_ptr<CharacterState> _state;
        
    };
    
    ObjectRef createCharacter(Player player, dvec2 position, ViewportRef viewport) {
        ColorA color;
        switch (player) {
            case Player::A:
                color = ColorA(1,0,0,1);
                break;
            case Player::B:
                color = ColorA(0,1,1,1);
                break;
        }
        
        auto state = make_shared<CharacterState>(color, position, 10, 10);
        auto drawer = make_shared<CharacterDrawComponent>();
        auto control = make_shared<CharacterControlComponent>(player);
        
        return core::Object::with("Character", { state, drawer, control });
    }
    
    class SplitViewCompositor : public BaseCompositor {
    public:
        
        SplitViewCompositor(const BaseViewportRef &viewportA, const BaseViewportRef &viewportB):
        _viewportA(viewportA),
        _viewportB(viewportB),
        _shader(util::loadGlslAsset("kessler/shaders/split_view_compositor.glsl")),
        _batch(gl::Batch::create(geom::Rect().rect(Rectf(0, 0, 1, 1)), _shader)),
        _side(0,1)
        {
        }
        
        void composite(int width, int height) override {
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
            _shader->uniform("ShadowWidth", static_cast<float>(_shadowWidth));
            _shader->uniform("ShadowColor", _shadowColor);
            _shader->uniform("Side", _side);
            _batch->draw();
        }
        
        void update(const time_state &time) override {
        }
        
        void setSplitSideDir(dvec2 dir) {
            _side = dir;
        }
        
        dvec2 getSplitSideDir() const { return _side; }
        
        void setShadowColor(ColorA color) { _shadowColor = color; }
        ColorA getShadowColor() const { return _shadowColor; }
        
        void setShadowWidth(double width) { _shadowWidth = max<double>(width, 0); }
        double getShadowWidth() const { return _shadowWidth; }
        
    private:
        
        BaseViewportRef _viewportA, _viewportB;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;
        vec2 _side;
        ColorA _shadowColor;
        double _shadowWidth;
        
    };
    
    /*
     int _width, _height;
     TrackableRef _firstTarget, _secondTarget;
     TrackerRef _firstTracker, _secondTracker;
     ViewportRef _firstViewport, _secondViewport;
     shared_ptr<SplitViewCompositor> _splitViewCompositor;
     double _scale;
     */
    
    class SplitViewComposer : public ViewportComposer {
    public:
        
        SplitViewComposer(const TrackableRef &firstTarget, const TrackableRef &secondTarget, const ViewportRef &firstViewport, const ViewportRef &secondViewport, const TrackerRef &firstTracker = nullptr, const TrackerRef &secondTracker = nullptr):
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
            _compositor = _splitViewCompositor = make_shared<SplitViewCompositor>(_viewports[0], _viewports[1]);
        }
        
        void setFirstTarget(const TrackableRef &target) { _firstTarget = target; }
        const TrackableRef &getFirstTarget() const { return _firstTarget; }

        void setSecondTarget(const TrackableRef &target) { _secondTarget = target; }
        const TrackableRef &getSecondTarget() const { return _secondTarget; }
        
        const ViewportRef &getFirstViewport() const { return _firstViewport; }
        const ViewportRef &getSecondViewport() const { return _secondViewport; }
        
        void setFirstTracker(const TrackerRef &tracker) { _firstTracker = tracker; }
        const TrackerRef &getFirstTracker() const { return _firstTracker; }

        void setSecondTracker(const TrackerRef &tracker) { _secondTracker = tracker; }
        const TrackerRef &getSecondTracker() const { return _secondTracker; }

        void setScale(double scale) {
            _scale = max(scale, 0.0);
        }
        
        double getScale() const { return _scale; }
        
        void onScenarioResized(int width, int height) override {
            _width = width;
            _height = height;
            
            _firstViewport->setSize(width, height);
            _secondViewport->setSize(width, height);
        }
        
        void update(const time_state &time) override {
            ViewportComposer::update(time);
            
            
            // first compute the split, in world space
            const dvec2 screenCenter(_width/2, _height/2);
            const dvec2 a = _firstTarget->getPosition();
            const dvec2 b = _secondTarget->getPosition();
            const double worldDistance = length(b-a);
            const dvec2 dir = (b-a) / worldDistance;
            

            // compute the voronoi mix component. when:
            // voronoiMix == 0, then both characters can be rendered on screen without a split
            // voronoiMix == 1, we have full separation
            const double screenDistance = worldDistance * _scale;
            const double maxEllipticalDistScreen = length(dvec2(dir.x * _width/2, dir.y * _height/2));
            const double minEllipticalDistScreen = maxEllipticalDistScreen * 0.5;
            const double voronoiMix = saturate(((screenDistance / 2)  - minEllipticalDistScreen) / (maxEllipticalDistScreen - minEllipticalDistScreen));

            // compute the tracking targets. when:
            // voronoiMix == 0, viewports both track the midpoint of the two targets
            // voronoiMix == 1, viewports track the targets directly
            const dvec2 firstTargetPositionWorld = _firstTarget->getPosition();
            const dvec2 secondTargetPositionWorld = _secondTarget->getPosition();
            const dvec2 targetMidpointWorld = (firstTargetPositionWorld + secondTargetPositionWorld) * 0.5;
            const double lookMix = util::easing::quad_ease_in_out(voronoiMix);
            const dvec2 firstLookWorld = lrp(lookMix, targetMidpointWorld, firstTargetPositionWorld);
            const dvec2 secondLookWorld = lrp(lookMix, targetMidpointWorld, secondTargetPositionWorld);
            
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
            const double centerOffsetMix = util::easing::quad_ease_in_out(voronoiMix);
            _firstViewport->setLookCenterOffset(centerOffset * centerOffsetMix);
            _secondViewport->setLookCenterOffset(-centerOffset * centerOffsetMix);
            
            // update the shader
            // shadow effect fased in as voronoi mix goes to 1
            _splitViewCompositor->setSplitSideDir(dir);
            _splitViewCompositor->setShadowColor(ColorA(0,0,0, voronoiMix * 0.5));
            _splitViewCompositor->setShadowWidth(50);

        }
        
    protected:
        
        int _width, _height;
        TrackableRef _firstTarget, _secondTarget;
        TrackerRef _firstTracker, _secondTracker;
        ViewportRef _firstViewport, _secondViewport;
        shared_ptr<SplitViewCompositor> _splitViewCompositor;
        double _scale;
        
    };
    
}

/*
 double _scale;
*/

MultiViewportTestScenario::MultiViewportTestScenario():
    _scale(1)
{}

MultiViewportTestScenario::~MultiViewportTestScenario()
{}

void MultiViewportTestScenario::setup()
{
    setStage(make_shared<Stage>("Multi-Viewport Test"));
    
    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.1));
    grid->setAxisColor(ColorA(0.2,1,1,1));
    getStage()->addObject(Object::with("Grid", {grid}));
    
    auto viewportA = make_shared<Viewport>();
    auto viewportB = make_shared<Viewport>();
    auto playerA = createCharacter(Player::A, dvec2(10,10), viewportA);
    auto playerB = createCharacter(Player::B, dvec2(100,100), viewportB);
    getStage()->addObject(playerA);
    getStage()->addObject(playerB);
    
    TrackableRef trackableA = playerA->getComponent<CharacterState>();
    TrackableRef trackableB = playerB->getComponent<CharacterState>();
    auto svc = make_shared<SplitViewComposer>(trackableA, trackableB, viewportA, viewportB);
    setViewportComposer(svc);
    
    getStage()->addObject(Object::with("Zelda", {
        make_shared<ImageDrawer>(gl::Texture2d::create(loadImage(app::loadAsset("tests/zelda.png"))), dvec2(0,0))
    }));
    
    // track 'r' for resetting scenario
    getStage()->addObject(Object::with("InputDelegation",{
        KeyboardDelegateComponent::create(0,{ cinder::app::KeyEvent::KEY_r })->onPress([&](int keyCode){
            this->reset();
        }),
        MouseDelegateComponent::create(0)->onWheel([&](dvec2 screen, dvec2 world, double deltaWheel, const ci::app::MouseEvent &event)->bool{
            this->_scale += deltaWheel * 0.1;
            return true;
        })
    }));
    
}

void MultiViewportTestScenario::cleanup()
{
    setStage(nullptr);
}

void MultiViewportTestScenario::update(const time_state &time) {
    Scenario::update(time);
    
    auto svc = static_pointer_cast<SplitViewComposer>(getViewportComposer());
    double scale = svc->getScale();
    scale = lrp<double>(0.25, scale, _scale);
    svc->setScale(scale);
}

void MultiViewportTestScenario::draw(const render_state &state) {
    Scenario::draw(state);
}

void MultiViewportTestScenario::drawScreen(const render_state &state)
{
    stringstream ss;
    ss
    << setprecision(2)
    << "fps: " << core::App::get()->getAverageFps()
    << " sps: " << core::App::get()->getAverageSps()
    << " visible: (" << getStage()->getDrawDispatcher()->visible().size() << "/" << getStage()->getDrawDispatcher()->all().size() << ")";
    
    
    gl::drawString(ss.str(), vec2(10, 10), Color(1, 1, 1));
}

void MultiViewportTestScenario::reset()
{
    CI_LOG_D("reset");
    cleanup();
    setup();
}

