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
    
    class CharacterState : public core::Component {
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
    
    ObjectRef createCharacter(Player player, dvec2 position) {
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

MultiViewportTestScenario::MultiViewportTestScenario()
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
    
    getStage()->addObject( createCharacter(Player::A, dvec2(10,10)));
    getStage()->addObject( createCharacter(Player::B, dvec2(100,100)));
    
    getStage()->addObject(Object::with("Zelda", {
        make_shared<ImageDrawer>(gl::Texture2d::create(loadImage(app::loadAsset("tests/zelda.png"))), dvec2(0,0))
    }));

    // track 'r' for resetting scenario
    getStage()->addObject(Object::with("KeyboardDelegate",{
        KeyboardDelegateComponent::create(0,{ cinder::app::KeyEvent::KEY_r })
        ->onPress([&](int keyCode){
            this->reset();
        })
    }));

}

void MultiViewportTestScenario::cleanup()
{
    setStage(nullptr);
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

