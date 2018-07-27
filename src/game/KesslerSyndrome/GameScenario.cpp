//
//  SurfacerScenario.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#include "game/KesslerSyndrome/GameScenario.hpp"

#include "elements/Components/DevComponents.hpp"
#include "game/KesslerSyndrome/GameStage.hpp"



using namespace core;
namespace game {

    GameScenario::GameScenario(string stageXmlFile) :
            _stageXmlFile(stageXmlFile) {
    }

    GameScenario::~GameScenario() {
    }

    void GameScenario::setup() {
        GameStageRef stage = make_shared<GameStage>();
        setStage(stage);

        stage->load(app::loadAsset(_stageXmlFile));
        
        getStage()->addObject(Object::with("Screen UI", {
            make_shared<elements::PerformanceDisplayComponent>(0, dvec2(10,10), ColorA(1,1,1,1))
        }));
        
        getStage()->addObject(Object::with("InputDelegation",{
            elements::KeyboardDelegateComponent::create(0)->onPress([&](int keyCode)->bool{
                switch(keyCode) {
                        
                    case app::KeyEvent::KEY_r:
                        this->reset();
                        return true;
                        
                    default:
                        return false;
                }
            })
        }));
    }

    void GameScenario::cleanup() {
        setStage(nullptr);
    }

    void GameScenario::reset() {
        cleanup();
        setup();
    }

}
