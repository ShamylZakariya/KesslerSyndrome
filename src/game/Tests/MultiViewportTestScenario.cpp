//
//  MultiViewportTestScenario.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/19/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include <cinder/gl/scoped.h>

#include "game/Tests/MultiViewportTestScenario.hpp"
#include "core/util/GlslProgLoader.hpp"
#include "core/Tracking.hpp"
#include "elements/Components/DevComponents.hpp"
#include "elements/SplitView/VoronoiSplitView.hpp"

using namespace core;
using namespace elements;

namespace {
    
    class ImageDrawer : public core::DrawComponent {
    public:
        
        ImageDrawer(gl::Texture2dRef image, dvec2 topLeft):
        DrawComponent(1, VisibilityDetermination::ALWAYS_DRAW),
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
        
    private:
        
        gl::Texture2dRef _image;
        dvec2 _topLeft;
        
    };
    
    class ScreenBlendingTestComponent : public core::ScreenDrawComponent {
    public:
        
        ScreenBlendingTestComponent(int layer, dvec2 topLeft):
                ScreenDrawComponent(layer),
                _topLeft(topLeft)
        {}
        
        void drawScreen(const render_state &renderState) override {
            
            gl::ScopedModelMatrix smm;
            gl::translate(_topLeft);
            
            int count = 8;
            double radius = 20;

            {
                gl::ScopedBlendAlpha sb;
                
                for (int i = 0; i < count; i++) {
                    double hue = static_cast<double>(i) / static_cast<double>(count);
                    ColorA color = ColorA(ColorModel::CM_HSV, hue, 1, 1, 0.5);
                    double x = i * radius * 1.5;
                    
                    gl::ScopedColor sc(color);
                    gl::drawSolidCircle(vec2(_topLeft.x + x + radius, _topLeft.y + radius), radius);
                }
            }
        }
        
    private:
        
        dvec2 _topLeft;
        
    };
    
#pragma mark - Character
    
    class CharacterState : public core::Component, public core::Trackable {
    public:
        CharacterState(ColorA color, dvec2 startPosition, double radius, double speed = 4 ):
        _position(startPosition),
        _radius(radius),
        _speed(speed),
        _color(color)
        {}
        
        void setPosition(dvec2 position) { _position = position; notifyMoved(); }
        dvec2 getPosition() const { return _position; }
        
        void setRadius(double r) { _radius = r; notifyMoved(); }
        double getRadius() const { return _radius; }
        
        void setColor(ColorA color) { _color = color; }
        ColorA getColor() const { return _color; }
        
        void setSpeed(double s) { _speed = s; }
        double getSpeed() const { return _speed; }
        
        // Trackable
        dvec2 getTrackingPosition() const override { return _position; }
        
    private:
        dvec2 _position;
        double _radius, _speed;
        ColorA _color;
    };
    
    class CharacterDrawComponent : public core::DrawComponent {
    public:
        CharacterDrawComponent():
        DrawComponent(100, VisibilityDetermination::FRUSTUM_CULLING)
        {}
        
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
            
    protected:
        
        weak_ptr<CharacterState> _state;
        
    };
    
    enum Player {
        A,
        B
    };
    
    class CharacterControlComponent : public core::InputComponent {
    public:
        
        CharacterControlComponent(Player player):
            InputComponent(0),
            _player(player)
        {
            switch(player) {
                case Player::A:
                    _upKeyCode = app::KeyEvent::KEY_w;
                    _downKeyCode = app::KeyEvent::KEY_s;
                    _leftKeyCode = app::KeyEvent::KEY_a;
                    _rightKeyCode = app::KeyEvent::KEY_d;
                    break;
                case Player::B:
                    _upKeyCode = app::KeyEvent::KEY_UP;
                    _downKeyCode = app::KeyEvent::KEY_DOWN;
                    _leftKeyCode = app::KeyEvent::KEY_LEFT;
                    _rightKeyCode = app::KeyEvent::KEY_RIGHT;
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
            
            if (isKeyDown(_upKeyCode)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(0,1));
            }
            
            if (isKeyDown(_downKeyCode)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(0,-1));
            }
            
            if (isKeyDown(_leftKeyCode)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(-1,0));
            }
            
            if (isKeyDown(_rightKeyCode)) {
                state->setPosition(state->getPosition() + state->getSpeed() * dvec2(1,0));
            }
        }
        
    private:
        
        Player _player;
        int _upKeyCode, _downKeyCode, _rightKeyCode, _leftKeyCode;
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
    
}

#pragma mark - MultiViewportTestScenario

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
    auto svc = make_shared<VoronoiSplitViewComposer>(trackableA, trackableB, viewportA, viewportB);
    svc->getVoronoiSplitViewCompositor()->setShadowColor(ColorA(0.0,0.0,0.05,0.5));
    svc->getVoronoiSplitViewCompositor()->setShadowWidth(0.5);
    setViewportComposer(svc);
    
     getStage()->addObject(Object::with("Zelda", {
        make_shared<ImageDrawer>(gl::Texture2d::create(loadImage(app::loadAsset("tests/zelda.png"))), dvec2(0,0))
    }));
    
    // track 'r' for resetting scenario
    getStage()->addObject(Object::with("InputDelegation",{
        KeyboardDelegateComponent::create(0)->onPress([&](int keyCode)->bool{
            switch(keyCode) {
                case app::KeyEvent::KEY_r:
                    this->reset();
                    return true;
                default:
                    return false;
            }
        }),
        MouseDelegateComponent::create(0)->onWheel([&](dvec2 screen, dvec2 world, double deltaWheel, const app::MouseEvent &event)->bool{
            this->_scale += deltaWheel * 0.1;
            return true;
        })
    }));
    
    getStage()->addObject(Object::with("Screen UI", {
        make_shared<elements::PerformanceDisplayComponent>(0, dvec2(10,10), ColorA(1,1,1,1)),
        make_shared<ScreenBlendingTestComponent>(1, dvec2(10,25))
    }));
}

void MultiViewportTestScenario::cleanup()
{
    setStage(nullptr);
}

void MultiViewportTestScenario::update(const time_state &time) {
    Scenario::update(time);
    
    auto svc = static_pointer_cast<VoronoiSplitViewComposer>(getViewportComposer());
    double scale = svc->getScale();
    scale = lrp<double>(0.25, scale, _scale);
    svc->setScale(scale);
}

void MultiViewportTestScenario::reset()
{
    CI_LOG_D("reset");
    cleanup();
    setup();
}

