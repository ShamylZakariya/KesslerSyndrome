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
    }

    void GameScenario::cleanup() {
        setStage(nullptr);
    }

    void GameScenario::resize(ivec2 size) {
    }

    void GameScenario::step(const time_state &time) {
    }

    void GameScenario::update(const time_state &time) {
    }

    void GameScenario::drawScreen(const render_state &state) {

        //
        // NOTE: we're in screen space, with coordinate system origin at top left
        //

        float fps = core::App::get()->getAverageFps();
        float sps = core::App::get()->getAverageSps();
        string info = core::strings::format("%.1f %.1f", fps, sps);
        gl::drawString(info, vec2(10, 10), Color(1, 1, 1));

        stringstream ss;
        Viewport::look look = getViewport()->getLook();
        double scale = getViewport()->getScale();

        ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y << ") scale: " << scale
           << " up: (" << look.up.x << ", " << look.up.y << ")";
        gl::drawString(ss.str(), vec2(10, 40), Color(1, 1, 1));
    }

    bool GameScenario::keyDown(const ci::app::KeyEvent &event) {
        if (event.getChar() == 'r') {
            reset();
            return true;
        } else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
            setRenderMode(RenderMode::mode((int(getRenderMode()) + 1) % RenderMode::COUNT));
        }
        return false;
    }

    void GameScenario::reset() {
        cleanup();
        setup();
    }

}
