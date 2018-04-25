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

#include <cinder/gl/scoped.h>

using namespace ci;
using namespace core;

namespace {
    
    struct LineSegment {
    public:
        
        LineSegment(dvec2 a, dvec2 b):
        a(a),
        b(b),
        dir(normalize(b-a))
        {}
        
        dvec2 getA() const { return a; }
        dvec2 getB() const { return b; }
        dvec2 getDir() const { return dir; }
        
        bool intersect(const LineSegment &other, dvec2 &intersection, bool bounded = true) const {
            
            //
            // http://jeffreythompson.org/collision-detection/line-line.php
            //
            
            // early exit for parallel lines
            const double epsilon = 1e-7;
            if(abs(dot(dir, other.dir)) > 1 - epsilon) {
                return false;
            }
            
            const double x1 = a.x, y1 = a.y, x2 = b.x, y2 = b.y;
            const double x3 = other.a.x, y3 = other.a.y, x4 = other.b.x, y4 = other.b.y;
            
            const double uA = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
            const double uB = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
            
            if (!bounded || (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1)) {
                intersection.x = x1 + (uA * (x2 - x1));
                intersection.y = y1 + (uA * (y2 - y1));
                return true;
            }
            
            return false;
        }
        
        bool intersects(const LineSegment &other, bool bounded = true) const {
            dvec2 _;
            return intersect(other, _, bounded);
        }
        
    private:

        dvec2 a, b, dir;
        
    };
    
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
    
    SMART_PTR(Trackable);

    class Trackable {
    public:
        virtual ~Trackable(){}
        virtual dvec2 getPosition() const = 0;
    };
    
    class CharacterState : public core::Component, public Trackable {
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
            _shader->uniform("Side", _side);
            _batch->draw();
        }
        
        void update(const time_state &time) override {
        }
        
        void setSplitSideDir(dvec2 dir) {
            _side = dir;
        }
        
        dvec2 getSplitSideDir() const { return _side; }
        
    private:
        
        BaseViewportRef _viewportA, _viewportB;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;
        vec2 _side;
        
    };
    
    class SplitViewComposer : public ViewportComposer {
    public:
        
        SplitViewComposer(const TrackableRef &firstTarget, const TrackableRef &secondTarget, const ViewportRef &firstViewport, const ViewportRef &secondViewport):
                _width(0),
                _height(0),
                _firstTarget(firstTarget),
                _secondTarget(secondTarget),
                _firstViewport(firstViewport),
                _secondViewport(secondViewport),
                _scale(1)
        {
            _viewports = { _firstViewport, _secondViewport };
            _compositor = _splitViewCompositor = make_shared<SplitViewCompositor>(_viewports[0], _viewports[1]);
        }
        
        const ViewportRef &getFirstViewport() const { return _firstViewport; }
        const ViewportRef &getSecondViewport() const { return _secondViewport; }
        
        void setScale(double scale) {
            _scale = max(scale, 0.0);
        }
        
        double getScale() const { return _scale; }
        
        void onScenarioResized(int width, int height) override {
            _width = width;
            _height = height;
            
            _firstViewport->setSize(width, height);
            _secondViewport->setSize(width, height);
            
            _firstViewport->setLookCenterOffset(dvec2(-width/4.0, -300));
            _secondViewport->setLookCenterOffset(dvec2(width/4.0, 50));
            
            _viewportEdgesScreenSpace = {
                LineSegment(dvec2(0,0), dvec2(width, 0)),
                LineSegment(dvec2(width,0), dvec2(width, height)),
                LineSegment(dvec2(width,height), dvec2(0, height)),
                LineSegment(dvec2(0,height), dvec2(0, 0)),
            };
        }
        
        void update(const time_state &time) override {
            ViewportComposer::update(time);
            
            
            // first compute the split, in world space
            const dvec2 screenCenter(_width/2, _height/2);
            const dvec2 a = _firstTarget->getPosition();
            const dvec2 b = _secondTarget->getPosition();
            const double worldDistance = length(b-a);
            const dvec2 dir = (b-a) / worldDistance;
            const dvec2 splitDir = rotateCCW(dir);
            
            _splitViewCompositor->setSplitSideDir(dir);

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
            const dvec2 firstLookWorld = lrp(voronoiMix, targetMidpointWorld, firstTargetPositionWorld);
            const dvec2 secondLookWorld = lrp(voronoiMix, targetMidpointWorld, secondTargetPositionWorld);
            _firstViewport->setLook(firstLookWorld, dvec2(0,1), _scale);
            _secondViewport->setLook(secondLookWorld, dvec2(0,1), _scale);


            // compute the look center offset. when:
            // voronoiMix == 0, look center offset is zero, making viewports look directly at the look.world position, which will be the midpoint of the two targets
            // voronoiMix == 1, track a position offset perpendicularly from the split
            const dvec2 center = (a + b) * 0.5;
            const double segmentLength = max(_width, _height) * 1.41421356237;
            const LineSegment splitSegment(center + splitDir * segmentLength * 0.5, center - splitDir * segmentLength * 0.5);
            const dvec2 anchor = -dir * length(dvec2(dir.x * _width/4, dir.y * _height/4));
            _firstViewport->setLookCenterOffset(anchor * voronoiMix);
            _secondViewport->setLookCenterOffset(-anchor * voronoiMix);
        }
        
    protected:
        
        
        
    private:
        
        int _width, _height;
        TrackableRef _firstTarget, _secondTarget;
        ViewportRef _firstViewport, _secondViewport;
        shared_ptr<SplitViewCompositor> _splitViewCompositor;
        vector<LineSegment> _viewportEdgesScreenSpace;
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
    auto characterA = createCharacter(Player::A, dvec2(10,10), viewportA);
    auto characterB = createCharacter(Player::B, dvec2(100,100), viewportB);
    getStage()->addObject(characterA);
    getStage()->addObject(characterB);
    
    TrackableRef trackableA = characterA->getComponent<CharacterState>();
    TrackableRef trackableB = characterB->getComponent<CharacterState>();
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

