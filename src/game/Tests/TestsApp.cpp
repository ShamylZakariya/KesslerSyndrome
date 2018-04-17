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
            [](){ return make_pair("ParticleSystemTestScenario", make_shared<ParticleSystemTestScenario>()); },
            [](){ return make_pair("PerlinWorldTestScenario", make_shared<PerlinWorldTestScenario>()); },
            [](){ return make_pair("SvgTestScenario", make_shared<SvgTestScenario>()); },
            [](){ return make_pair("TerrainAttachmentsTestScenario", make_shared<TerrainAttachmentsTestScenario>()); },
            [](){ return make_pair("IPTestsScenario", make_shared<IPTestsScenario>()); },
            [](){ return make_pair("EasingTestScenario", make_shared<EasingTestScenario>()); },
            [](){ return make_pair("TerrainTestScenario", make_shared<TerrainTestScenario>()); },
        };
        
        _index = 0;
        loadScenario();
    }
    
    //
    // command+shift+[left or right bracket] to switch through test scenarios
    //

    void keyDown( ci::app::KeyEvent event ) override {
        if (event.isShiftDown() && event.isMetaDown()) {
            switch(event.getCode()) {
                case ci::app::KeyEvent::KEY_LEFTBRACKET:
                    if (_index > 0) {
                        _index--;
                    } else {
                        _index = _scenarioFactories.size() - 1;
                    }
                    loadScenario();
                    break;
                    
                case ci::app::KeyEvent::KEY_RIGHTBRACKET:
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

CINDER_APP(TestsApp, ci::app::RendererGl, TestsApp::prepareSettings)
