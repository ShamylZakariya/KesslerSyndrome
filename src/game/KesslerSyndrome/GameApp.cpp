//
//  main.cpp
//  Milestone3
//
//  Created by Shamyl Zakariya on 2/2/17.
//
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "GameScenario.hpp"

class KesslerSyndromeApp : public core::App {
public:

    static void prepareSettings(Settings *settings) {
        App::prepareSettings(settings);
        settings->setTitle("Kessler Syndrome");
    }

public:

    KesslerSyndromeApp() {
    }

    virtual void setup() override {
        App::setup();
        setScenario(make_shared<game::GameScenario>("kessler/stages/0.xml"));
    }

};

CINDER_APP(KesslerSyndromeApp, app::RendererGl, KesslerSyndromeApp::prepareSettings)
