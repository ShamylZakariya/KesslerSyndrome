//
//  TestsApp.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/9/17.
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "TerrainTestScenario.hpp"
#include "TerrainAttachmentsTestScenario.hpp"
#include "SvgTestScenario.hpp"
#include "PerlinWorldTestScenario.hpp"
#include "IPTestsScenario.hpp"
#include "EasingTestScenario.hpp"
#include "ParticleSystemTestScenario.hpp"
#include "MultiViewportTestScenario.hpp"

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
            SCENARIO_FACTORY(MultiViewportTestScenario),
            SCENARIO_FACTORY(ParticleSystemTestScenario),
            SCENARIO_FACTORY(PerlinWorldTestScenario),
            SCENARIO_FACTORY(SvgTestScenario),
            SCENARIO_FACTORY(TerrainAttachmentsTestScenario),
            SCENARIO_FACTORY(IPTestsScenario),
            SCENARIO_FACTORY(EasingTestScenario),
            SCENARIO_FACTORY(TerrainTestScenario),
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
