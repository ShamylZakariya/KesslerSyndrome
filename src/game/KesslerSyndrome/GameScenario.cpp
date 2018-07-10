//
//  SurfacerScenario.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#include "GameScenario.hpp"

#include "App.hpp"
#include "GameStage.hpp"

#include "DevComponents.hpp"


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
    }

    void GameScenario::cleanup() {
        setStage(nullptr);
    }

    bool GameScenario::keyDown(const app::KeyEvent &event) {
        if (event.getChar() == 'r') {
            reset();
            return true;
        }
        return false;
    }

    void GameScenario::reset() {
        cleanup();
        setup();
    }

}
