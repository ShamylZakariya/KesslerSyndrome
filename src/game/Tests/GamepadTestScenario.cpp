//
//  GamepadTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/1/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "GamepadTestScenario.hpp"
#include "elements/Components/DevComponents.hpp"
#include "core/util/Svg.hpp"

using namespace core;
using namespace elements;

namespace {
    
    class GamepadRenderer : public util::svg::SvgDrawComponent {
    public:
        GamepadRenderer(GamepadRef gamepad, dvec2 pos):
                SvgDrawComponent(util::svg::Group::loadSvgDocument(app::loadAsset("tests/gamepad.svg"), 1)),
                _gamepad(gamepad)
        {
            auto svg = getSvg();
            svg->setPosition(pos);
            
            // get our shapes
            _leftStickKnobGroup = svg->findGroupById("left_stick_knob");
            _leftStickKnob = _leftStickKnobGroup->getChildShapes().front();
            _rightStickKnobGroup = svg->findGroupById("right_stick_knob");
            _rightStickKnob = _rightStickKnobGroup->getChildShapes().front();
            
            _defaultPositions[_leftStickKnobGroup] = _leftStickKnobGroup->getPosition();
            _defaultPositions[_rightStickKnobGroup] = _rightStickKnobGroup->getPosition();

            _leftStickBase = svg->findGroupById("left_stick_base")->getChildShapes().front();
            _rightStickBase = svg->findGroupById("right_stick_base")->getChildShapes().front();

            _buttonABase = svg->findGroupById("button_a_base")->getChildShapes().front();
            _buttonBBase = svg->findGroupById("button_b_base")->getChildShapes().front();
            _buttonXBase = svg->findGroupById("button_x_base")->getChildShapes().front();
            _buttonYBase = svg->findGroupById("button_y_base")->getChildShapes().front();

            _buttonR1Base = svg->findGroupById("button_r1_base")->getChildShapes().front();
            _buttonR2Base = svg->findGroupById("button_r2_base")->getChildShapes().front();
            _buttonL1Base = svg->findGroupById("button_l1_base")->getChildShapes().front();
            _buttonL2Base = svg->findGroupById("button_l2_base")->getChildShapes().front();

            _buttonStartBase = svg->findGroupById("button_start_base")->getChildShapes().front();
            _buttonSelectBase = svg->findGroupById("button_select_base")->getChildShapes().front();
            
            _dPadUp = svg->findGroupById("d_pad_up")->getChildShapes().front();
            _dPadRight = svg->findGroupById("d_pad_right")->getChildShapes().front();
            _dPadDown = svg->findGroupById("d_pad_down")->getChildShapes().front();
            _dPadLeft = svg->findGroupById("d_pad_left")->getChildShapes().front();

            saveDefaults(_leftStickKnob);
            saveDefaults(_leftStickBase);
            saveDefaults(_rightStickKnob);
            saveDefaults(_rightStickBase);
            saveDefaults(_buttonABase);
            saveDefaults(_buttonBBase);
            saveDefaults(_buttonXBase);
            saveDefaults(_buttonYBase);
            saveDefaults(_buttonR1Base);
            saveDefaults(_buttonR2Base);
            saveDefaults(_buttonL1Base);
            saveDefaults(_buttonL2Base);
            saveDefaults(_buttonStartBase);
            saveDefaults(_buttonSelectBase);
        }
        
        void update(const time_state &time) override {
            SvgDrawComponent::update(time);
            
            if (_gamepad) {
                const auto activateColor = ColorA(0,1,1,1);
                activate(_buttonABase, activateColor, _gamepad->getAButton() ? 1 : 0);
                activate(_buttonBBase, activateColor, _gamepad->getBButton() ? 1 : 0);
                activate(_buttonXBase, activateColor, _gamepad->getXButton() ? 1 : 0);
                activate(_buttonYBase, activateColor, _gamepad->getYButton() ? 1 : 0);
                activate(_buttonStartBase, activateColor, _gamepad->getStartButton() ? 1 : 0);
                activate(_buttonSelectBase, activateColor, _gamepad->getSelectButton() ? 1 : 0);
                activate(_buttonL1Base, activateColor, _gamepad->getLeftShoulderButton() ? 1 : 0);
                activate(_buttonR1Base, activateColor, _gamepad->getRightShoulderButton() ? 1 : 0);
                activate(_buttonL2Base, activateColor, _gamepad->getLeftTrigger());
                activate(_buttonR2Base, activateColor, _gamepad->getRightTrigger());
                
                const auto leftStickBB = _leftStickBase->getLocalBB();
                const auto leftStickRange = dvec2(cpBBWidth(leftStickBB),cpBBHeight(leftStickBB)) * 0.4;
                const auto rightStickBB = _rightStickBase->getLocalBB();
                const auto rightStickRange = dvec2(cpBBWidth(rightStickBB),cpBBHeight(rightStickBB)) * 0.4;
                const auto leftStickDefaultPosition = _defaultPositions[_leftStickKnobGroup];
                const auto rightStickDefaultPosition = _defaultPositions[_rightStickKnobGroup];
                
                const auto leftStickPosition = leftStickDefaultPosition + _gamepad->getLeftStick() * leftStickRange;
                const auto rightStickPosition = rightStickDefaultPosition + _gamepad->getRightStick() * rightStickRange;

                _leftStickKnobGroup->setPosition(leftStickPosition);
                _rightStickKnobGroup->setPosition(rightStickPosition);
            }
        }

    protected:
        
        void saveDefaults(util::svg::ShapeRef shape) {
            _defaultColors[shape] = shape->getAppearance()->getFillColor();
        }
        
        void activate(util::svg::ShapeRef shape, ColorA activateColor, double amount) {
            shape->getAppearance()->setFillColor(_defaultColors[shape].lerp(amount, activateColor));
        }
        
    protected:
        
        GamepadRef _gamepad;
        util::svg::GroupRef _leftStickKnobGroup, _rightStickKnobGroup;
        util::svg::ShapeRef _leftStickKnob, _leftStickBase, _rightStickKnob, _rightStickBase, _buttonABase, _buttonBBase, _buttonXBase, _buttonYBase, _buttonR1Base, _buttonR2Base, _buttonL1Base, _buttonL2Base, _buttonStartBase, _buttonSelectBase, _dPadUp, _dPadRight, _dPadDown, _dPadLeft;
        
        map<util::svg::ShapeRef, ColorA> _defaultColors;
        map<util::svg::GroupRef, dvec2> _defaultPositions;
        
    };
    
    ObjectRef GamepadObject(dvec2 pos, GamepadRef gamepad) {
        return Object::with("Gamepad", {make_shared<GamepadRenderer>(gamepad, pos)});
    }
    
}

GamepadTestScenario::GamepadTestScenario()
{
}

GamepadTestScenario::~GamepadTestScenario() {
}

void GamepadTestScenario::setup() {
    setStage(make_shared<Stage>("Gamepad Test"));
    
    auto viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());
    auto stage = getStage();
    auto input = InputDispatcher::get();

    stage->addObject(Object::with("ViewportControlComponent", {
        viewportController,
        make_shared<MouseViewportControlComponent>(viewportController)
    }));
    
    // build background grid
    auto grid = elements::WorldCartesianGridDrawComponent::create(1);
    grid->setGridColor(ColorA(0.2,0.2,0.7,0.5));
    grid->setAxisColor(ColorA(0.2,0.2,0.7,1));
    grid->setAxisIntensity(1);
    grid->setFillColor(ColorA(0.95,0.95,0.96,1));
    stage->addObject(Object::with("Grid", { grid }));
    
    if (!input->getGamepads().empty()) {
        double x = 0;
        for (auto gamepad : input->getGamepads()) {
            auto obj = GamepadObject(dvec2(x,0), gamepad);
            stage->addObject(obj);
            
            auto dc = obj->getComponent<GamepadRenderer>();
            x += cpBBWidth(dc->getBB()) + 50;
        }
    } else {
        CI_LOG_D("Found no gamepads");
    }
}

void GamepadTestScenario::cleanup() {
    setStage(nullptr);
}

void GamepadTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void GamepadTestScenario::draw(const render_state &state) {
    Scenario::draw(state);
}

void GamepadTestScenario::drawScreen(const render_state &state) {
    Scenario::drawScreen(state);
}

void GamepadTestScenario::reset() {
    cleanup();
    setup();
}
