//
//  TestsApp.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/9/17.
//

#include <cinder/app/RendererGl.h>
#include "core/Core.hpp"

#include "game/Tests/TerrainTestScenario.hpp"
#include "game/Tests/TerrainAttachmentsTestScenario.hpp"
#include "game/Tests/SvgTestScenario.hpp"
#include "game/Tests/PerlinWorldTestScenario.hpp"
#include "game/Tests/IPTestsScenario.hpp"
#include "game/Tests/EasingTestScenario.hpp"
#include "game/Tests/ParticleSystemTestScenario.hpp"
#include "game/Tests/MultiViewportTestScenario.hpp"
#include "game/Tests/FilterStackTestScenario.hpp"
#include "game/Tests/GamepadTestScenario.hpp"
#include "game/Tests/CharacterTestScenario.hpp"

#define SCENARIO_FACTORY(s) [](){ return make_pair(#s, make_shared<s>()); }

class TestsApp : public core::App {
public:

    static void prepareSettings(Settings *settings) {
        App::prepareSettings(settings);
    }

public:

    TestsApp():
            _index(0) {
    }

    void setup() override {
        App::setup();
        
        _scenarioFactories = {
            SCENARIO_FACTORY(CharacterTestScenario),
            SCENARIO_FACTORY(TerrainTestScenario),
            SCENARIO_FACTORY(TerrainAttachmentsTestScenario),
            SCENARIO_FACTORY(ParticleSystemTestScenario),
            SCENARIO_FACTORY(PerlinWorldTestScenario),
            SCENARIO_FACTORY(FilterStackTestScenario),
            SCENARIO_FACTORY(MultiViewportTestScenario),
            SCENARIO_FACTORY(SvgTestScenario),
            SCENARIO_FACTORY(IPTestsScenario),
            SCENARIO_FACTORY(EasingTestScenario),
            SCENARIO_FACTORY(GamepadTestScenario),
        };
        
        _index = 0;
        loadScenario();
    }
    
    //
    // command+shift+[left or right bracket] to switch through test scenarios
    //

    void keyDown( app::KeyEvent event ) override {
        if (event.isShiftDown() && event.isMetaDown()) {
            switch(event.getCode()) {
                case app::KeyEvent::KEY_LEFTBRACKET:
                    if (_index > 0) {
                        _index--;
                    } else {
                        _index = _scenarioFactories.size() - 1;
                    }
                    loadScenario();
                    break;
                    
                case app::KeyEvent::KEY_RIGHTBRACKET:
                    _index = (_index+1) % _scenarioFactories.size();
                    loadScenario();
                    break;
            }
        }
    }
    
private:
    
    void loadScenario() {
        auto r = _scenarioFactories[_index]();
        getWindow()->setTitle("Test - " + str(_index) + " (" + r.first + ")");
        setScenario(r.second);
    }
    
    size_t _index;
    vector<function<pair<string,ScenarioRef>()>> _scenarioFactories;

};

CINDER_APP(TestsApp, app::RendererGl, TestsApp::prepareSettings)
